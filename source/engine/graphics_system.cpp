#include "graphics_system.h"
#include "core/log.h"
#include "engine/system_manager.h"
#include "engine/debug_gui_system.h"
#include "engine/debug_gui_menubar.h"
#include "engine/script_system.h"
#include "engine/render_system.h"
#include "engine/job_system.h"
#include "engine/input_system.h"
#include "engine/texture_manager.h"
#include "engine/model_manager.h"
#include "engine/shader_manager.h"
#include "engine/renderer.h"
#include "engine/debug_render.h"
#include "engine/frustum.h"
#include "render/render_pass.h"
#include "engine/file_picker_dialog.h"
#include "render/window.h"
#include "core/profiler.h"
#include "core/timer.h"
#include "entity/entity_system.h"
#include "engine/components/component_light.h"
#include "engine/components/component_transform.h"
#include "engine/components/component_model.h"
#include "engine/components/component_sdf_model.h"
#include "engine/components/component_tags.h"
#include "engine/components/component_material.h"

Engine::MenuBar g_graphicsMenu;
bool g_showTextureGui = false;
bool g_showModelGui = false;
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

void GraphicsSystem::RegisterComponents()
{
	m_entitySystem->RegisterComponentType<Material>();
	m_entitySystem->RegisterInspector<Material>(Material::MakeInspector(*m_debugGui, *m_textures));

	m_entitySystem->RegisterComponentType<Tags>();
	m_entitySystem->RegisterInspector<Tags>(Tags::MakeInspector(*m_debugGui));

	m_entitySystem->RegisterComponentType<Transform>();
	m_entitySystem->RegisterInspector<Transform>(Transform::MakeInspector(*m_debugGui, *m_debugRender));

	m_entitySystem->RegisterComponentType<Light>();
	m_entitySystem->RegisterInspector<Light>(Light::MakeInspector(*m_debugGui, *m_debugRender));

	m_entitySystem->RegisterComponentType<Model>();
	m_entitySystem->RegisterInspector<Model>(Model::MakeInspector(*m_debugGui, *m_models, *m_shaders));

	m_entitySystem->RegisterComponentType<SDFModel>();
	m_entitySystem->RegisterInspector<SDFModel>(SDFModel::MakeInspector(*m_debugGui, *m_textures));
}

void GraphicsSystem::RegisterScripts()
{
	m_scriptSystem->Globals().new_usertype<glm::vec3>("vec3", sol::constructors<glm::vec3(), glm::vec3(float, float, float)>(),
		"x", &glm::vec3::x,
		"y", &glm::vec3::y,
		"z", &glm::vec3::z);
	m_scriptSystem->Globals().new_usertype<glm::vec4>("vec4", sol::constructors<glm::vec4(), glm::vec4(float, float, float, float)>(),
		"x", &glm::vec4::x,
		"y", &glm::vec4::y,
		"z", &glm::vec4::z,
		"w", &glm::vec4::z);

	m_scriptSystem->Globals().new_usertype<Engine::TextureHandle>("TextureHandle", sol::constructors<Engine::TextureHandle()>());
	m_scriptSystem->Globals().new_usertype<Engine::ModelHandle>("ModelHandle", sol::constructors<Engine::ModelHandle()>());
	m_scriptSystem->Globals().new_usertype<Engine::ShaderHandle>("ShaderHandle", sol::constructors<Engine::ShaderHandle()>());
	
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
	graphics["LoadComputeShader"] = [this](const char* name, const char* csPath) -> Engine::ShaderHandle {
		return m_shaders->LoadComputeShader(name, csPath);
	};
	graphics["SetShadowShader"] = [this](Engine::ShaderHandle lightingShander, Engine::ShaderHandle shadowShader) {
		m_shaders->SetShadowsShader(lightingShander, shadowShader);
	};
	graphics["DrawModel"] = [this](float px, float py, float pz, float scale, Engine::ModelHandle h, Engine::ShaderHandle sh) {
		auto transform = glm::scale(glm::translate(glm::identity<glm::mat4>(), glm::vec3(px, py, pz)), glm::vec3(scale));
		m_renderer->SubmitInstance(transform, h, sh);
	};
	graphics["PointLight"] = [this](float px, float py, float pz, float r, float g, float b, float ambient, float distance, float atten) {
		m_renderer->SetLight(glm::vec4(px, py, pz, 1.0f), glm::vec3(0.0f), glm::vec3(r, g, b), ambient, distance, atten);
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
}

bool GraphicsSystem::PreInit(Engine::SystemManager& manager)
{
	SDE_PROF_EVENT();

	m_scriptSystem = (Engine::ScriptSystem*)manager.GetSystem("Script");
	m_renderSystem = (Engine::RenderSystem*)manager.GetSystem("Render");
	m_inputSystem = (Engine::InputSystem*)manager.GetSystem("Input");
	m_jobSystem = (Engine::JobSystem*)manager.GetSystem("Jobs");
	m_debugGui = (Engine::DebugGuiSystem*)manager.GetSystem("DebugGui");
	m_entitySystem = (EntitySystem*)manager.GetSystem("Entities");

	m_shaders = std::make_unique<Engine::ShaderManager>();
	m_textures = std::make_unique<Engine::TextureManager>(m_jobSystem);
	m_models = std::make_unique<Engine::ModelManager>(m_textures.get(), m_jobSystem);

	return true;
}

bool GraphicsSystem::Initialise()
{
	SDE_PROF_EVENT();

	const auto& windowProps = m_renderSystem->GetWindow()->GetProperties();
	m_windowSize = glm::ivec2(windowProps.m_sizeX, windowProps.m_sizeY);

	//// add our renderer to the global passes
	m_renderer = std::make_unique<Engine::Renderer>(m_textures.get(), m_models.get(), m_shaders.get(), m_jobSystem, m_windowSize);
	m_renderSystem->AddPass(*m_renderer);
	m_debugRender = std::make_unique<Engine::DebugRender>(m_shaders.get());

	RegisterComponents();
	RegisterScripts();
 
	auto& gMenu = g_graphicsMenu.AddSubmenu(ICON_FK_TELEVISION " Graphics");
	gMenu.AddItem("Reload Shaders", [this]() { m_renderer->Reset(); m_shaders->ReloadAll(); });
	gMenu.AddItem("Reload Textures", [this]() { m_renderer->Reset(); m_textures->ReloadAll(); });
	gMenu.AddItem("Reload Models", [this]() { m_renderer->Reset(); m_models->ReloadAll(); });
	gMenu.AddItem("TextureManager", [this]() { g_showTextureGui = true; });
	gMenu.AddItem("ModelManager", [this]() { g_showModelGui = true; });
	gMenu.AddItem("Toggle Render Stats", [this]() {m_showStats = !m_showStats; });

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
	auto materials = world->GetAllComponents<Material>();

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
		world->ForEachComponent<Model>([this, &world, &transforms, &materials](Model& model, EntityHandle owner) {
			const Transform* transform = transforms->Find(owner);
			if (transform && model.GetModel().m_index != -1 && model.GetShader().m_index != -1)
			{
				Render::Material* instanceMaterial = nullptr;
				if (model.GetMaterialEntity().GetID() != -1)
				{
					auto matComponent = materials->Find(model.GetMaterialEntity());
					if (matComponent != nullptr)
					{
						instanceMaterial = &matComponent->GetRenderMaterial();
					}
				}
				m_renderer->SubmitInstance(transform->GetMatrix(), model.GetModel(), model.GetShader(), instanceMaterial);
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
		world->ForEachComponent<SDFModel>([this, &world, &transforms, &materials](SDFModel& m, EntityHandle owner) {
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
				Render::Material* instanceMaterial = nullptr;
				if (m.GetMaterialEntity().GetID() != -1)
				{
					auto matComponent = materials->Find(m.GetMaterialEntity());
					if (matComponent != nullptr)
					{
						instanceMaterial = &matComponent->GetRenderMaterial();
					}
				}
				m_renderer->SubmitInstance(transform->GetMatrix(), *m.GetMesh(), m.GetShader(), m.GetBoundsMin(), m.GetBoundsMax(), instanceMaterial);
			}
		});
	}
}

void GraphicsSystem::ShowGui(int framesPerSecond)
{
	m_debugGui->MainMenuBar(g_graphicsMenu);
	
	if (g_showTextureGui)
	{
		g_showTextureGui = m_textures->ShowGui(*m_debugGui);
	}

	if (g_showModelGui)
	{
		g_showModelGui = m_models->ShowGui(*m_debugGui);
	}

	if (m_showStats)
	{
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
		m_renderer->SetWireframeMode(m_debugGui->Checkbox("Wireframe", m_renderer->GetWireframeMode()));
		m_showBounds = m_debugGui->Checkbox("Draw Bounds", m_showBounds);
		m_renderer->SetExposure(m_debugGui->DragFloat("Exposure", m_renderer->GetExposure(), 0.01f, 0.0f, 100.0f));
		m_renderer->SetBloomThreshold(m_debugGui->DragFloat("Bloom Threshold", m_renderer->GetBloomThreshold(), 0.01f, 0.0f, 0.0f));
		m_renderer->SetBloomMultiplier(m_debugGui->DragFloat("Bloom Multiplier", m_renderer->GetBloomMultiplier(), 0.01f, 0.0f, 0.0f));
		m_renderer->SetCullingEnabled(m_debugGui->Checkbox("Culling Enabled", m_renderer->IsCullingEnabled()));
		m_debugGui->EndWindow();
	}
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