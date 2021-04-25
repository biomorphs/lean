#include "graphics_system.h"
#include "core/log.h"
#include "engine/system_enumerator.h"
#include "engine/debug_gui_system.h"
#include "engine/debug_gui_menubar.h"
#include "engine/script_system.h"
#include "engine/render_system.h"
#include "engine/job_system.h"
#include "engine/input_system.h"
#include "engine/debug_camera.h"
#include "engine/arcball_camera.h"
#include "engine/texture_manager.h"
#include "engine/model_manager.h"
#include "engine/shader_manager.h"
#include "engine/renderer.h"
#include "engine/debug_render.h"
#include "engine/frustum.h"
#include "render/render_pass.h"
#include "engine/file_picker_dialog.h"
#include "render/window.h"
#include "render/camera.h"
#include "core/profiler.h"
#include "core/timer.h"
#include "engine/arcball_camera.h"
#include "entity/entity_system.h"
#include "engine/components/component_light.h"
#include "engine/components/component_transform.h"
#include "engine/components/component_model.h"
#include "engine/components/component_sdf_model.h"

Engine::MenuBar g_graphicsMenu;
bool g_showTextureGui = false;
bool g_showModelGui = false;
bool g_useArcballCam = false;
bool g_showCameraInfo = false;
bool g_enableShadowUpdate = true;

struct SDFDebugDraw : public Engine::SDFMeshBuilder::Debug
{
	Engine::DebugRender* dbg;
	Engine::DebugGuiSystem* gui;
	glm::vec3 cellSize;
	glm::mat4 transform;
	float alpha = 0.5f;
	void OnQuad(glm::vec3 v0, glm::vec3 v1, glm::vec3 v2, glm::vec3 v3, glm::vec3 n0, glm::vec3 n1, glm::vec3 n2, glm::vec3 n3)
	{
		// push the vertices out a tiny amount so we dont have z-fighting
		float c_bias = 0.01f;	
		glm::vec4 v[] = {
			transform * glm::vec4(v0 + (n0 * c_bias),1.0f),
			transform * glm::vec4(v1 + (n1 * c_bias),1.0f),
			transform * glm::vec4(v1 + (n1 * c_bias),1.0f),
			transform * glm::vec4(v2 + (n2 * c_bias),1.0f),
			transform * glm::vec4(v2 + (n2 * c_bias),1.0f),
			transform * glm::vec4(v3 + (n3 * c_bias),1.0f),
			transform * glm::vec4(v3 + (n3 * c_bias),1.0f),
			transform * glm::vec4(v0 + (n0 * c_bias),1.0f),
		};
		glm::vec4 c[] = {
			glm::vec4(0.0f,0.8f,1.0f,alpha),
			glm::vec4(0.0f,0.8f,1.0f,alpha),
			glm::vec4(0.0f,0.8f,1.0f,alpha),
			glm::vec4(0.0f,0.8f,1.0f,alpha),
			glm::vec4(0.0f,0.8f,1.0f,alpha),
			glm::vec4(0.0f,0.8f,1.0f,alpha),
			glm::vec4(0.0f,0.8f,1.0f,alpha),
			glm::vec4(0.0f,0.8f,1.0f,alpha),
		};
		dbg->AddLines(v, c, 4);
	}

	void OnCellVertex(glm::vec3 p, glm::vec3 n)
	{
		dbg->AddBox(glm::vec3(transform * glm::vec4(p, 1.0f)), cellSize * 0.2f, glm::vec4(1.0f, 0.0f, 0.0f, alpha));
		glm::vec4 p4 = transform * glm::vec4(p, 1.0f);
		glm::vec3 normal = glm::mat3(transform) * n;
		glm::vec4 v[] = {
			p4,
			p4 + glm::vec4(normal * 0.05f,1.0f)
		};
		glm::vec4 c[] = { glm::vec4(1.0f,1.0f,0.0f,alpha), glm::vec4(1.0f,1.0f,0.0f,alpha) };
		dbg->AddLines(v, c, 1);
	}
};

GraphicsSystem::GraphicsSystem()
{
}

GraphicsSystem::~GraphicsSystem()
{
}

bool GraphicsSystem::PreInit(Engine::SystemEnumerator& systemEnumerator)
{
	SDE_PROF_EVENT();

	m_scriptSystem = (Engine::ScriptSystem*)systemEnumerator.GetSystem("Script");
	m_renderSystem = (Engine::RenderSystem*)systemEnumerator.GetSystem("Render");
	m_inputSystem = (Engine::InputSystem*)systemEnumerator.GetSystem("Input");
	m_jobSystem = (Engine::JobSystem*)systemEnumerator.GetSystem("Jobs");
	m_debugGui = (Engine::DebugGuiSystem*)systemEnumerator.GetSystem("DebugGui");
	m_entitySystem = (EntitySystem*)systemEnumerator.GetSystem("Entities");

	m_shaders = std::make_unique<Engine::ShaderManager>();
	m_textures = std::make_unique<Engine::TextureManager>(m_jobSystem);
	m_models = std::make_unique<Engine::ModelManager>(m_textures.get(), m_jobSystem);
	m_mainRenderCamera = std::make_unique<Render::Camera>();

	return true;
}

bool GraphicsSystem::Initialise()
{
	SDE_PROF_EVENT();

	const auto& windowProps = m_renderSystem->GetWindow()->GetProperties();
	m_windowSize = glm::ivec2(windowProps.m_sizeX, windowProps.m_sizeY);

	m_entitySystem->RegisterComponentType<Transform>();
	m_entitySystem->RegisterComponentUi<Transform>([](ComponentStorage& cs, EntityHandle e, Engine::DebugGuiSystem& dbg) {
		auto& t = *static_cast<Transform::StorageType&>(cs).Find(e);
		t.SetPosition(dbg.DragVector("Position", t.GetPosition(), 0.25f, -100000.0f, 100000.0f));
		t.SetRotationDegrees(dbg.DragVector("Rotation", t.GetRotationDegrees(), 0.1f));
		t.SetScale(dbg.DragVector("Scale", t.GetScale(), 0.05f, 0.0f));
	});

	m_entitySystem->RegisterComponentType<Light>();
	m_entitySystem->RegisterComponentUi<Light>([this](ComponentStorage& cs, EntityHandle e, Engine::DebugGuiSystem& dbg) {
		auto& l = *static_cast<Light::StorageType&>(cs).Find(e);
		int typeIndex = static_cast<int>(l.GetLightType());
		const char* types[] = { "Directional", "Point", "Spot" };
		if (dbg.ComboBox("Type", types, 3, typeIndex))
		{
			l.SetType(static_cast<Light::Type>(typeIndex));
		}
		l.SetColour(glm::vec3(dbg.ColourEdit("Colour", glm::vec4(l.GetColour(), 1.0f), false)));
		l.SetBrightness(dbg.DragFloat("Brightness", l.GetBrightness(), 0.001f, 0.0f, 10000.0f));
		l.SetAmbient(dbg.DragFloat("Ambient", l.GetAmbient(), 0.001f, 0.0f, 1.0f));
		l.SetDistance(dbg.DragFloat("Distance", l.GetDistance(), 0.1f, 0.0f, 3250.0f));
		if (l.GetLightType() != Light::Type::Directional)
		{
			l.SetAttenuation(dbg.DragFloat("Attenuation", l.GetAttenuation(), 0.1f, 0.0001f, 1000.0f));
		}
		if (l.GetLightType() == Light::Type::Spot)
		{
			auto angles = l.GetSpotAngles();
			angles.y = dbg.DragFloat("Outer Angle", angles.y, 0.01f, angles.x, 1.0f);
			angles.x = dbg.DragFloat("Inner Angle", angles.x, 0.01f, 0.0f, angles.y);
			l.SetSpotAngles(angles.x, angles.y);
		}
		l.SetCastsShadows(dbg.Checkbox("Cast Shadows", l.CastsShadows()));
		if (l.CastsShadows())
		{
			if (l.GetLightType() == Light::Type::Directional)
			{
				l.SetShadowOrthoScale(dbg.DragFloat("Ortho Scale", l.GetShadowOrthoScale(), 0.1f, 0.1f, 10000000.0f));
			}
			l.SetShadowBias(dbg.DragFloat("Shadow Bias", l.GetShadowBias(), 0.001f, 0.0f, 10.0f));
			if (!l.IsPointLight() && l.GetShadowMap() != nullptr && !l.GetShadowMap()->IsCubemap())
			{
				dbg.Image(*l.GetShadowMap()->GetDepthStencil(), glm::vec2(256.0f));
			}
		}
		if (!l.IsPointLight())
		{
			Engine::Frustum f(l.GetShadowMatrix());
			m_debugRender->DrawFrustum(f, glm::vec4(l.GetColour(), 1.0f));
		}
	});

	m_entitySystem->RegisterComponentType<Model>();
	m_entitySystem->RegisterComponentUi<Model>([this](ComponentStorage& cs, EntityHandle e, Engine::DebugGuiSystem& dbg) {
		auto& m = *static_cast<Model::StorageType&>(cs).Find(e);
		std::string modelPath = m_models->GetModelPath(m.GetModel());
		if (dbg.Button(modelPath.c_str()))
		{
			std::string newFile = Engine::ShowFilePicker("Select Model", "", "Model Files (.fbx)\0*.fbx\0(.obj)\0*.obj\0");
			if (newFile != "")
			{
				auto loadedModel = m_models->LoadModel(newFile.c_str());
				m.SetModel(loadedModel);
			}
		}
		auto allShaders = m_shaders->AllShaders();
		std::vector<std::string> shaders;
		std::string vs, fs;
		for (auto it : allShaders)
		{
			if (m_shaders->GetShaderPaths(it, vs, fs))
			{
				char shaderPathText[1024];
				sprintf(shaderPathText, "%s, %s", vs.c_str(), fs.c_str());
				shaders.push_back(shaderPathText);
			}
		}
		shaders.push_back("None");
		int shaderIndex = m.GetShader().m_index != (uint32_t)-1 ? m.GetShader().m_index : shaders.size() - 1;
		if (m_debugGui->ComboBox("Shader", shaders, shaderIndex))
		{
			m.SetShader({ (uint32_t)(shaderIndex == (shaders.size() - 1) ? (uint32_t)-1 : shaderIndex) });
		}
	});

	m_entitySystem->RegisterComponentType<SDFModel>();
	m_entitySystem->RegisterComponentUi<SDFModel>([this](ComponentStorage& cs, EntityHandle e, Engine::DebugGuiSystem& dbg) {
		auto& m = *static_cast<SDFModel::StorageType&>(cs).Find(e);
		int typeIndex = static_cast<int>(m.GetMeshMode());
		const char* types[] = { "Blocky", "Surface Net", "Dual Contour" };
		if (dbg.ComboBox("Mesh Type", types, 3, typeIndex))
		{
			m.SetMeshMode(static_cast<Engine::SDFMeshBuilder::MeshMode>(typeIndex));
			m.Remesh();
		}
		auto bMin = m.GetBoundsMin();
		auto bMax = m.GetBoundsMax();
		bMin = dbg.DragVector("BoundsMin", bMin, 0.1f);
		bMax = dbg.DragVector("BoundsMax", bMax, 0.1f);
		m.SetBounds(bMin, bMax);
		auto r = m.GetResolution();
		r.x = dbg.DragInt("ResX", r.x, 1, 1);
		r.y = dbg.DragInt("ResY", r.y, 1, 1);
		r.z = dbg.DragInt("ResZ", r.z, 1, 1);
		m.SetResolution(r.x, r.y, r.z);
		m.SetNormalSmoothness(dbg.DragFloat("Normal Smooth", m.GetNormalSmoothness(), 0.01f, 0.0f));
		std::string texturePath = m_textures->GetTexturePath(m.GetDiffuseTexture());
		texturePath = "Diffuse: " + texturePath;
		if (dbg.Button(texturePath.c_str()))
		{
			std::string newFile = Engine::ShowFilePicker("Select Texture", "", "JPG (.jpg)\0*.jpg\0PNG (.png)\0*.png\0BMP (.bmp)\0*.bmp\0");
			if (newFile != "")
			{
				auto loadedTexture = m_textures->LoadTexture(newFile);
				m.SetDiffuseTexture(loadedTexture);
			}
		}
		if (dbg.Button("Remesh Now"))
		{
			m.Remesh();
		}
		m.SetDebugEnabled(dbg.Checkbox("Debug Render", m.GetDebugEnabled()));
	});
	
	//// add our renderer to the global passes
	m_renderer = std::make_unique<Engine::Renderer>(m_textures.get(), m_models.get(), m_shaders.get(), m_jobSystem, m_windowSize);
	m_renderSystem->AddPass(*m_renderer);

	// expose types to lua
	m_scriptSystem->Globals().new_usertype<Engine::TextureHandle>("TextureHandle",sol::constructors<Engine::TextureHandle()>());
	m_scriptSystem->Globals().new_usertype<Engine::ModelHandle>("ModelHandle",sol::constructors<Engine::ModelHandle()>());
	m_scriptSystem->Globals().new_usertype<Engine::ShaderHandle>("ShaderHandle", sol::constructors<Engine::ShaderHandle()>());
	m_scriptSystem->Globals().new_usertype<glm::vec3>("vec3", sol::constructors<glm::vec3(), glm::vec3(float,float,float)>(),
		"x", &glm::vec3::x,
		"y", &glm::vec3::y,
		"z", &glm::vec3::z);

	//// expose Graphics script functions
	auto graphics = m_scriptSystem->Globals()["Graphics"].get_or_create<sol::table>();
	graphics["SetClearColour"] = [this](float r, float g, float b) {
		m_renderer->SetClearColour(glm::vec4(r, g, b, 1.0f));
	};
	graphics["LoadTexture"] = [this](const char* path) -> Engine::TextureHandle {
		return m_textures->LoadTexture(path);
	};
	graphics["LoadModel"] = [this](const char* path) -> Engine::ModelHandle {
		return m_models->LoadModel(path);
	};
	graphics["LoadShader"] = [this](const char* name, const char* vsPath, const char* fsPath) -> Engine::ShaderHandle {
		return m_shaders->LoadShader(name, vsPath, fsPath);
	};
	graphics["SetShadowShader"] = [this](Engine::ShaderHandle lightingShander, Engine::ShaderHandle shadowShader) {
		m_shaders->SetShadowsShader(lightingShander, shadowShader);
	};
	graphics["DrawModel"] = [this](float px, float py, float pz, float scale, Engine::ModelHandle h, Engine::ShaderHandle sh) {
		auto transform = glm::scale(glm::translate(glm::identity<glm::mat4>(), glm::vec3(px, py, pz)), glm::vec3(scale));
		m_renderer->SubmitInstance(transform, h, sh);
	};
	graphics["PointLight"] = [this](float px, float py, float pz, float r, float g, float b, float ambient, float distance, float atten) {
		m_renderer->SetLight(glm::vec4(px, py, pz,1.0f), glm::vec3(0.0f), glm::vec3(r, g, b), ambient, distance, atten);
	};
	graphics["DirectionalLight"] = [this](float dx, float dy, float dz, float r, float g, float b, float ambient) {
		m_renderer->SetLight(glm::vec4(0.0f), { dx,dy,dz }, glm::vec3(r, g, b), ambient, 0.0f, 0.0f);
	};
	graphics["DebugDrawAxis"] = [this](float px, float py, float pz, float size) {
		m_debugRender->AddAxisAtPoint({ px,py,pz,1.0f }, size);
	};
	graphics["DebugDrawBox"] = [this](float px, float py, float pz, float size, float r, float g, float b, float a) {
		m_debugRender->AddBox({ px,py,pz }, { size,size,size }, { r, g, b, a });
	};
	graphics["DebugDrawLine"] = [this](float p0x, float p0y, float p0z, float p1x, float p1y, float p1z, float p0r, float p0g, float p0b, float p0a, float p1r, float p1g, float p1b, float p1a) {
		glm::vec4 positions[] = {
			{p0x,p0y,p0z,0.0f}, {p1x,p1y,p1z,0.0f}
		};
		glm::vec4 colours[] = {
			{p0r,p0g,p0b,p0a},{p1r,p1g,p1b,p1a}
		};
		m_debugRender->AddLines(positions, colours, 1);
	};
	m_debugRender = std::make_unique<Engine::DebugRender>(m_shaders.get());

	auto windowSize = m_renderSystem->GetWindow()->GetSize();
	m_debugCamera = std::make_unique<Engine::DebugCamera>();
	m_debugCamera->SetPosition({0.f,0.0f,20.0f});
	m_debugCamera->SetPitch(0.0f);
	m_debugCamera->SetYaw(0.0f);

	m_arcballCamera = std::make_unique<Engine::ArcballCamera>();
	m_arcballCamera->SetWindowSize(m_renderSystem->GetWindow()->GetSize());
	m_arcballCamera->SetPosition({ 7.1f,8.0f,15.0f });
	m_arcballCamera->SetTarget({ 0.0f,5.0f,0.0f });
	m_arcballCamera->SetUp({ 0.0f,1.0f,0.0f });
	
	float aspect = (float)windowSize.x / (float)windowSize.y;
	m_mainRenderCamera->SetProjection(70.0f, aspect, 0.1f, 1000.0f);
 
	auto& gMenu = g_graphicsMenu.AddSubmenu(ICON_FK_TELEVISION " Graphics");
	gMenu.AddItem("Reload Shaders", [this]() { m_renderer->Reset(); m_shaders->ReloadAll(); });
	gMenu.AddItem("Reload Textures", [this]() { m_renderer->Reset(); m_textures->ReloadAll(); });
	gMenu.AddItem("Reload Models", [this]() { m_renderer->Reset(); m_models->ReloadAll(); });
	gMenu.AddItem("TextureManager", [this]() { g_showTextureGui = true; });
	gMenu.AddItem("ModelManager", [this]() { g_showModelGui = true; });
	auto& camMenu = g_graphicsMenu.AddSubmenu(ICON_FK_CAMERA " Camera (Flycam)");
	camMenu.AddItem("Toggle Camera Mode", [this,&camMenu]() {
		g_useArcballCam = !g_useArcballCam; 
		if (g_useArcballCam)
		{
			camMenu.m_label = ICON_FK_CAMERA " Camera (Arcball)";
		}
		else
		{
			camMenu.m_label = ICON_FK_CAMERA " Camera (Flycam)";
		}
	});
	camMenu.AddItem("Show Camera Info", [this]() {g_showCameraInfo = true; });

	return true;
}

void GraphicsSystem::DrawModelBounds(const Engine::Model& m, glm::mat4 transform)
{
	SDE_PROF_EVENT();
	for (const auto& part : m.Parts())
	{
		const auto bmin = part.m_boundsMin;
		const auto bmax = part.m_boundsMax;
		m_debugRender->DrawBox(bmin, bmax, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), transform);
	}
	m_debugRender->DrawBox(m.BoundsMin(), m.BoundsMax(), glm::vec4(1.0f, 0.0f, 0.0f, 1.0f), transform);
}

void GraphicsSystem::ProcessLight(Light& l, const Transform* transform)
{
	SDE_PROF_EVENT();
	const glm::vec3 position = transform ? transform->GetPosition() : glm::vec3(0.0f);
	const glm::vec4 posAndType = { position, static_cast<float>(l.GetLightType()) };
	const float distance = l.GetDistance();
	const float attenuation = l.GetAttenuation();
	glm::vec3 direction = { 0.0f,-1.0f,0.0f };
	if (!l.IsPointLight())
	{
		// direction is based on entity transform, default is (0,-1,0)
		auto transformRot = glm::mat3(transform->GetMatrix());
		direction = glm::normalize(transformRot * direction);
	}
	if (l.CastsShadows())
	{
		bool recreateShadowmap = l.GetShadowMap() == nullptr || l.GetShadowMap()->IsCubemap() != l.IsPointLight();
		if (recreateShadowmap)
		{
			auto& sm = l.GetShadowMap();
			sm = std::make_unique<Render::FrameBuffer>(l.GetShadowmapSize());
			if (l.IsPointLight())
				sm->AddDepthCube();
			else
				sm->AddDepth();
			if (!sm->Create())
			{
				SDE_LOG("Failed to create shadow depth buffer");
			}
		}
		bool updateShadowmap = l.GetShadowMap() != nullptr;
		glm::mat4 shadowMatrix = l.GetShadowMatrix();
		if (updateShadowmap)
		{
			shadowMatrix = l.UpdateShadowMatrix(position, direction);
		}

		if (l.GetLightType() == Light::Type::Spot)
		{
			m_renderer->SpotLight(position, direction, l.GetColour() * l.GetBrightness(), l.GetAmbient(), distance, attenuation, l.GetSpotAngles(), *l.GetShadowMap(), l.GetShadowBias(), shadowMatrix, updateShadowmap);
		}
		else
		{
			// the renderer keeps a reference to the shadow map here for 1 frame, do not delete lights that are in use!
			m_renderer->SetLight(posAndType, direction, l.GetColour() * l.GetBrightness(), l.GetAmbient(), distance, attenuation, *l.GetShadowMap(), l.GetShadowBias(), shadowMatrix, updateShadowmap);
		}
	}
	else
	{
		l.GetShadowMap() = nullptr;	// this is a safe spot to destroy unused shadow maps
		if (l.GetLightType() == Light::Type::Spot)
		{
			m_renderer->SpotLight(position, direction, l.GetColour() * l.GetBrightness(), l.GetAmbient(), distance, attenuation, l.GetSpotAngles());
		}
		else
		{
			m_renderer->SetLight(posAndType, direction, l.GetColour() * l.GetBrightness(), l.GetAmbient(), distance, attenuation);
		}
	}
};

void GraphicsSystem::ProcessEntities()
{
	SDE_PROF_EVENT();

	auto world = m_entitySystem->GetWorld();
	auto transforms = world->GetAllComponents<Transform>();

	// submit all lights
	{
		SDE_PROF_EVENT("SubmitLights");
		world->ForEachComponent<Light>([this, &world, &transforms](Light& light, EntityHandle owner) {
			const Transform* transform = transforms->Find(owner);
			if (transform)
			{
				ProcessLight(light, transform);
			}
		});
	}

	// submit all models
	{
		SDE_PROF_EVENT("SubmitEntities");
		world->ForEachComponent<Model>([this, &world, &transforms](Model& model, EntityHandle owner) {
			const Transform* transform = transforms->Find(owner);
			if (transform && model.GetModel().m_index != -1 && model.GetShader().m_index != -1)
			{
				m_renderer->SubmitInstance(transform->GetMatrix(), model.GetModel(), model.GetShader());
			}
		});
	}

	if(m_showBounds)
	{
		SDE_PROF_EVENT("ShowBounds");
		world->ForEachComponent<Model>([this, &world, &transforms](Model& m, EntityHandle owner) {
			const auto renderModel = m_models->GetModel(m.GetModel());
			const Transform* transform = transforms->Find(owner);
			if (transform && renderModel)
			{
				DrawModelBounds(*renderModel, transform->GetMatrix());
			}
		});
	}

	// SDF Models
	{
		SDE_PROF_EVENT("ProcessSDFModels");
		Engine::Frustum viewFrustum(m_mainRenderCamera->ProjectionMatrix() * m_mainRenderCamera->ViewMatrix());
		world->ForEachComponent<SDFModel>([this, &world, &transforms, &viewFrustum](SDFModel& m, EntityHandle owner) {
			const Transform* transform = transforms->Find(owner);
			if (!transform)
				return;

			if (m_showBounds)
			{
				auto colour = m.IsRemeshing() ? glm::vec4(0.5f, 0.5f, 0.0f, 0.5f) : glm::vec4(0.0f, 0.5f, 0.0f, 0.5f);
				m_debugRender->DrawBox(m.GetBoundsMin(), m.GetBoundsMax(), colour, transform->GetMatrix());
			}

			if (m.GetDebugEnabled())
			{
				SDFDebugDraw debug;
				debug.cellSize = (m.GetBoundsMax() - m.GetBoundsMin()) / glm::vec3(m.GetResolution());
				debug.dbg = m_debugRender.get();
				debug.gui = m_debugGui;
				debug.transform = transform->GetMatrix();
				m.UpdateMesh(m_jobSystem, debug);
			}
			else
			{
				m.UpdateMesh(m_jobSystem);
			}

			if (m.GetMesh() && m.GetShader().m_index != -1)
			{
				m_renderer->SubmitInstance(transform->GetMatrix(), *m.GetMesh(), m.GetShader(), m.GetBoundsMin(), m.GetBoundsMax());
			}
		});
	}
}

void GraphicsSystem::ShowGui(int framesPerSecond)
{
	m_debugGui->MainMenuBar(g_graphicsMenu);

	if (g_showCameraInfo)
	{
		m_debugGui->BeginWindow(g_showCameraInfo, "Camera");
		glm::vec3 posVec = m_mainRenderCamera->Position();
		m_debugGui->DragVector("Position", posVec);
		m_debugGui->EndWindow();
	}
	
	if (g_showTextureGui)
	{
		g_showTextureGui = m_textures->ShowGui(*m_debugGui);
	}

	if (g_showModelGui)
	{
		g_showModelGui = m_models->ShowGui(*m_debugGui);
	}

	const auto& fs = m_renderer->GetStats();
	char statText[1024] = { '\0' };
	bool forceOpen = true;
	m_debugGui->BeginWindow(forceOpen, "Render Stats");
	sprintf_s(statText, "Total Instances Submitted: %zu", fs.m_instancesSubmitted);	m_debugGui->Text(statText);
	sprintf_s(statText, "\tOpaques: %zu (%zu visible)", fs.m_totalOpaqueInstances, fs.m_renderedOpaqueInstances);	m_debugGui->Text(statText);
	sprintf_s(statText, "\tTransparents: %zu (%zu visible)", fs.m_totalTransparentInstances, fs.m_renderedTransparentInstances);	m_debugGui->Text(statText);
	sprintf_s(statText, "Shadowmaps updated: %zu", fs.m_shadowMapUpdates);	m_debugGui->Text(statText);
	sprintf_s(statText, "\tCasters: %zu (%zu visible)", fs.m_totalShadowInstances, fs.m_renderedShadowInstances);	m_debugGui->Text(statText);
	sprintf_s(statText, "Active Lights: %zu (%zu visible)", fs.m_activeLights, fs.m_visibleLights);	m_debugGui->Text(statText);
	sprintf_s(statText, "Shader Binds: %zu", fs.m_shaderBinds);	m_debugGui->Text(statText);
	sprintf_s(statText, "VA Binds: %zu", fs.m_vertexArrayBinds);	m_debugGui->Text(statText);
	sprintf_s(statText, "Batches Drawn: %zu", fs.m_batchesDrawn);	m_debugGui->Text(statText);
	sprintf_s(statText, "Draw calls: %zu", fs.m_drawCalls);	m_debugGui->Text(statText);
	sprintf_s(statText, "Total Tris: %zu", fs.m_totalVertices / 3);	m_debugGui->Text(statText);
	sprintf_s(statText, "FPS: %d", framesPerSecond);	m_debugGui->Text(statText);
	m_showBounds = m_debugGui->Checkbox("Draw Bounds", m_showBounds);
	m_renderer->SetExposure(m_debugGui->DragFloat("Exposure", m_renderer->GetExposure(), 0.01f, 0.0f, 100.0f));
	m_renderer->SetBloomThreshold(m_debugGui->DragFloat("Bloom Threshold", m_renderer->GetBloomThreshold(), 0.01f, 0.0f, 0.0f));
	m_renderer->SetBloomMultiplier(m_debugGui->DragFloat("Bloom Multiplier", m_renderer->GetBloomMultiplier(), 0.01f, 0.0f, 0.0f));
	m_renderer->SetCullingEnabled(m_debugGui->Checkbox("Culling Enabled", m_renderer->IsCullingEnabled()));
	m_debugGui->EndWindow();
}

void GraphicsSystem::ProcessCamera(float timeDelta)
{
	if (g_useArcballCam)
	{
		if (!m_debugGui->IsCapturingMouse())
		{
			m_arcballCamera->Update(m_inputSystem->GetMouseState(), timeDelta);
		}
		m_mainRenderCamera->LookAt(m_arcballCamera->GetPosition(), m_arcballCamera->GetTarget(), m_arcballCamera->GetUp());
	}
	else
	{
		m_debugCamera->Update(m_inputSystem->ControllerState(0), timeDelta);
		if (!m_debugGui->IsCapturingMouse())
		{
			m_debugCamera->Update(m_inputSystem->GetMouseState(), timeDelta);
		}
		if (!m_debugGui->IsCapturingKeyboard())
		{
			m_debugCamera->Update(m_inputSystem->GetKeyboardState(), timeDelta);
		}
		m_debugCamera->ApplyToCamera(*m_mainRenderCamera);
	}
	m_renderer->SetCamera(*m_mainRenderCamera);
}

bool GraphicsSystem::Tick(float timeDelta)
{
	SDE_PROF_EVENT();

	static int framesPerSecond = 0;
	static uint32_t framesThisSecond = 0;
	static Core::Timer timer;
	static double startTime = timer.GetSeconds();

	double currentTime = timer.GetSeconds();
	framesThisSecond++;
	if (currentTime - startTime >= 1.0f)
	{
		framesPerSecond = framesThisSecond;
		framesThisSecond = 0;
		startTime = currentTime;
	}

	ProcessCamera(timeDelta);
	ProcessEntities();
	ShowGui(framesPerSecond);

	m_debugRender->PushToRenderer(*m_renderer);

	// Process loaded data on main thread
	m_textures->ProcessLoadedTextures();
	m_models->ProcessLoadedModels();

	return true;
}

void GraphicsSystem::Shutdown()
{
	SDE_PROF_EVENT();

	m_scriptSystem->Globals()["Graphics"] = nullptr;
	m_debugRender = nullptr;
	m_renderer = nullptr;
	m_models = nullptr;
	m_textures = nullptr;
	m_shaders = nullptr;
}