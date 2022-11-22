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
#include "engine/2d_render_context.h"
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
#include "engine/components/component_model_part_materials.h"
#include "engine/components/component_environment_settings.h"

Engine::MenuBar g_graphicsMenu;
bool g_enableShadowUpdate = true;

class RenderPass2D : public Render::RenderPass
{
public:
	void Reset()
	{
		if (m_rc2d)
		{
			m_rc2d->Reset(m_windowDimensions);
		}
	}
	void RenderAll(Render::Device& d)
	{
		if (m_rc2d)
		{
			d.DrawToBackbuffer();
			m_rc2d->Render(d);
		}
	}
	Engine::RenderContext2D* m_rc2d = nullptr;;
	glm::vec2 m_windowDimensions;
} g_rc2d;

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
	m_entitySystem->RegisterInspector<Material>(Material::MakeInspector(*m_debugGui));

	m_entitySystem->RegisterComponentType<ModelPartMaterials>();
	m_entitySystem->RegisterInspector<ModelPartMaterials>(ModelPartMaterials::MakeInspector(*m_debugGui));

	m_entitySystem->RegisterComponentType<Tags>();
	m_entitySystem->RegisterInspector<Tags>(Tags::MakeInspector(*m_debugGui));

	m_entitySystem->RegisterComponentType<Transform>();
	m_entitySystem->RegisterInspector<Transform>(Transform::MakeInspector(*m_debugGui, *m_debugRender));

	m_entitySystem->RegisterComponentType<Light>();
	m_entitySystem->RegisterInspector<Light>(Light::MakeInspector(*m_debugGui, *m_debugRender));

	m_entitySystem->RegisterComponentType<Model>();
	m_entitySystem->RegisterInspector<Model>(Model::MakeInspector(*m_debugGui));

	m_entitySystem->RegisterComponentType<SDFModel>();
	m_entitySystem->RegisterInspector<SDFModel>(SDFModel::MakeInspector(*m_debugGui));

	m_entitySystem->RegisterComponentType<EnvironmentSettings>();
	m_entitySystem->RegisterInspector<EnvironmentSettings>(EnvironmentSettings::MakeInspector());
}

void GraphicsSystem::RegisterScripts()
{
	m_scriptSystem->Globals().new_usertype<glm::ivec2>("ivec2", sol::constructors<glm::ivec2(), glm::ivec2(int, int)>(),
		"x", &glm::ivec2::x,
		"y", &glm::ivec2::y);
	m_scriptSystem->Globals().new_usertype<glm::vec2>("vec2", sol::constructors<glm::vec2(), glm::vec2(float, float)>(),
		"x", &glm::vec2::x,
		"y", &glm::vec2::y);
	m_scriptSystem->Globals().new_usertype<glm::vec3>("vec3", sol::constructors<glm::vec3(), glm::vec3(float, float, float)>(),
		"x", &glm::vec3::x,
		"y", &glm::vec3::y,
		"z", &glm::vec3::z);
	m_scriptSystem->Globals().new_usertype<glm::vec4>("vec4", sol::constructors<glm::vec4(), glm::vec4(float, float, float, float)>(),
		"x", &glm::vec4::x,
		"y", &glm::vec4::y,
		"z", &glm::vec4::z,
		"w", &glm::vec4::w);

	m_scriptSystem->Globals().new_usertype<Engine::TextureHandle>("TextureHandle", sol::constructors<Engine::TextureHandle()>());
	m_scriptSystem->Globals().new_usertype<Engine::ModelHandle>("ModelHandle", sol::constructors<Engine::ModelHandle()>());
	m_scriptSystem->Globals().new_usertype<Engine::ShaderHandle>("ShaderHandle", sol::constructors<Engine::ShaderHandle()>());
	
	//// expose Graphics script functions
	auto graphics = m_scriptSystem->Globals()["Graphics"].get_or_create<sol::table>();
	graphics["SetClearColour"] = [this](float r, float g, float b) {
		m_renderer->SetClearColour(glm::vec4(r, g, b, 1.0f));
	};
	graphics["LoadTexture"] = [this](const char* path) -> Engine::TextureHandle {
		return Engine::GetSystem<Engine::TextureManager>("Textures")->LoadTexture(path);
	};
	graphics["LoadModel"] = [this](const char* path) -> Engine::ModelHandle {
		return Engine::GetSystem<Engine::ModelManager>("Models")->LoadModel(path);
	};
	graphics["LoadShader"] = [this](const char* name, const char* vsPath, const char* fsPath) -> Engine::ShaderHandle {
		return Engine::GetSystem<Engine::ShaderManager>("Shaders")->LoadShader(name, vsPath, fsPath);
	};
	graphics["LoadComputeShader"] = [this](const char* name, const char* csPath) -> Engine::ShaderHandle {
		return Engine::GetSystem<Engine::ShaderManager>("Shaders")->LoadComputeShader(name, csPath);
	};
	graphics["SetShadowShader"] = [this](Engine::ShaderHandle lightingShander, Engine::ShaderHandle shadowShader) {
		Engine::GetSystem<Engine::ShaderManager>("Shaders")->SetShadowsShader(lightingShander, shadowShader);
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
	auto graphics2d = m_scriptSystem->Globals()["Graphics2D"].get_or_create<sol::table>();
	graphics2d["DrawSolidQuad"] = [this](float p0x, float p0y, int zIndex, float width, float height, float r, float g, float b, float a) {
		auto white = Engine::GetSystem<Engine::TextureManager>("Textures")->LoadTexture("white.bmp");
		m_render2D->DrawQuad({ p0x, p0y }, zIndex, { width,height }, { 0.0,0.0 }, { 1.0,1.0 }, { r,g,b,a }, white);
	};
	graphics2d["DrawTexturedQuad"] = [this](float p0x, float p0y, int zIndex, float width, float height,
		float uv0x, float uv0y, float uv1x, float uv1y,
		float r, float g, float b, float a, Engine::TextureHandle t)
	{
		m_render2D->DrawQuad({ p0x, p0y }, zIndex, { width, height }, { uv0x, uv0y }, { uv1x, uv1y }, { r,g,b,a }, t);
	};
	graphics2d["GetDimensions"] = [this]() -> glm::vec2 {
		return m_render2D->GetDimensions();
	};
}

bool GraphicsSystem::PreInit()
{
	SDE_PROF_EVENT();

	m_scriptSystem = Engine::GetSystem<Engine::ScriptSystem>("Script");
	m_renderSystem = Engine::GetSystem<Engine::RenderSystem>("Render");
	m_inputSystem = Engine::GetSystem<Engine::InputSystem>("Input");
	m_jobSystem = Engine::GetSystem<Engine::JobSystem>("Jobs");
	m_debugGui = Engine::GetSystem<Engine::DebugGuiSystem>("DebugGui");
	m_entitySystem = Engine::GetSystem<EntitySystem>("Entities");

	return true;
}

bool GraphicsSystem::PostInit()
{
	SDE_PROF_EVENT();

	const auto& windowProps = m_renderSystem->GetWindow()->GetProperties();
	m_windowSize = glm::ivec2(windowProps.m_sizeX, windowProps.m_sizeY);

	//// add our renderer to the global passes
	m_renderer = std::make_unique<Engine::Renderer>(m_jobSystem, m_windowSize);
	m_renderSystem->AddPass(*m_renderer);
	m_debugRender = std::make_unique<Engine::DebugRender>();
	m_render2D = std::make_unique<Engine::RenderContext2D>();
	g_rc2d.m_rc2d = m_render2D.get();
	g_rc2d.m_windowDimensions = m_windowSize;
	m_renderSystem->AddPass(g_rc2d);

	RegisterComponents();
	RegisterScripts();
 
	auto& gMenu = g_graphicsMenu.AddSubmenu(ICON_FK_TELEVISION " Graphics");
	gMenu.AddItem("Toggle Render Stats", [this]() {m_showStats = !m_showStats; });

	return true;
}

void GraphicsSystem::DrawModelBounds(const Engine::Model& m, glm::mat4 transform, glm::vec4 mainColour, glm::vec4 partsColour)
{
	SDE_PROF_EVENT();
	if (partsColour != glm::vec4(0.0f))
	{
		for (const auto& part : m.MeshParts())
		{
			const auto bmin = part.m_boundsMin;
			const auto bmax = part.m_boundsMax;
			m_debugRender->DrawBox(bmin, bmax, partsColour, transform * part.m_transform);
		}
	}
	m_debugRender->DrawBox(m.BoundsMin(), m.BoundsMax(), mainColour, transform);
}

void GraphicsSystem::ProcessLight(Light& l, Transform* transform)
{
	SDE_PROF_EVENT();
	const glm::vec3 position = glm::vec3(transform->GetWorldspaceMatrix()[3]);
	const glm::vec4 posAndType = { position, static_cast<float>(l.GetLightType()) };
	const float distance = l.GetDistance();
	const float attenuation = l.GetAttenuation();
	glm::vec3 direction = { 0.0f,-1.0f,0.0f };
	if (!l.IsPointLight())
	{
		// direction is based on entity transform, default is (0,-1,0)
		auto transformRot = glm::mat3(transform->GetWorldspaceMatrix());
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
	SDE_PROF_EVENT("SubmitLights");
	static auto lightIterator = world->MakeIterator<Light, Transform>();
	lightIterator.ForEach([this](Light& l, Transform& t, EntityHandle h) {
		ProcessLight(l, &t);
	});

	// submit all models
	SDE_PROF_EVENT("SubmitEntities");
	static auto modelIterator = world->MakeIterator<Model, Transform>();
	modelIterator.ForEach([this, &materials](Model& m, Transform& t, EntityHandle h) {
		if (m.GetModel().m_index != -1 && m.GetShader().m_index != -1)
		{
			ModelPartMaterials* partOverrides = m.GetPartMaterialsComponent();
			if (partOverrides == nullptr)
			{
				m_renderer->SubmitInstance(t.GetWorldspaceMatrix(), m.GetModel(), m.GetShader());
			}
			else
			{
				m_renderer->SubmitInstance(t.GetWorldspaceMatrix(), m.GetModel(), m.GetShader(), partOverrides->Materials().data(), partOverrides->Materials().size());
			}
		}
	});

	if(m_showBounds)
	{
		SDE_PROF_EVENT("ShowBounds");
		auto* models = Engine::GetSystem<Engine::ModelManager>("Models");

		static World::EntityIterator iterator = world->MakeIterator<Model, Transform>();
		iterator.ForEach([this, models](Model& m, Transform& t, EntityHandle h) {
			const auto renderModel = models->GetModel(m.GetModel());
			if (renderModel)
			{
				DrawModelBounds(*renderModel, t.GetWorldspaceMatrix(), glm::vec4(1.0f), glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
			}
		});
	}

	// SDF Models
	{
		SDE_PROF_EVENT("ProcessSDFModels");
		static World::EntityIterator iterator = world->MakeIterator<SDFModel, Transform>();
		iterator.ForEach([this,&materials](SDFModel& m, Transform& t, EntityHandle h) {
			if (m_showBounds)
			{
				auto colour = m.IsRemeshing() ? glm::vec4(0.5f, 0.5f, 0.0f, 0.5f) : glm::vec4(0.0f, 0.5f, 0.0f, 0.5f);
				m_debugRender->DrawBox(m.GetBoundsMin(), m.GetBoundsMax(), colour, t.GetWorldspaceMatrix());
			}

			if (m.GetDebugEnabled())
			{
				SDFDebugDraw debug;
				debug.cellSize = (m.GetBoundsMax() - m.GetBoundsMin()) / glm::vec3(m.GetResolution());
				debug.dbg = m_debugRender.get();
				debug.gui = m_debugGui;
				debug.transform = t.GetWorldspaceMatrix();
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
				m_renderer->SubmitInstance(t.GetWorldspaceMatrix(), *m.GetMesh(), m.GetShader(), m.GetBoundsMin(), m.GetBoundsMax(), instanceMaterial);
			}
		});
	}

	world->ForEachComponent<EnvironmentSettings>([this](EnvironmentSettings& s, EntityHandle owner) {
		m_renderer->SetClearColour(glm::vec4(s.GetClearColour(), 1.0f));
	});
}

void GraphicsSystem::ShowGui(int framesPerSecond)
{
	m_debugGui->MainMenuBar(g_graphicsMenu);

	if (m_showStats)
	{
		const auto& fs = m_renderer->GetStats();
		char statText[1024] = { '\0' };
		m_debugGui->BeginWindow(m_showStats, "Render Stats");
		m_renderer->SetUseDrawIndirect(m_debugGui->Checkbox("Use draw indirect", m_renderer->GetUseDrawIndirect()));
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

	return true;
}

void GraphicsSystem::Shutdown()
{
	SDE_PROF_EVENT();

	m_scriptSystem->Globals()["Graphics"] = nullptr;
	m_debugRender = nullptr;
	m_renderer = nullptr;
	g_rc2d.m_rc2d = nullptr;
	m_render2D = nullptr;
}