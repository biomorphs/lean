#include "graphics.h"
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
#include "render/render_pass.h"
#include "render/window.h"
#include "render/camera.h"
#include "core/profiler.h"
#include "core/timer.h"
#include "engine/arcball_camera.h"
#include "engine/entity/entity_system.h"
#include "components/light.h"
#include "components/transform.h"
#include "components/model.h"
#include <sol.hpp>

Graphics::Graphics()
{
}

Graphics::~Graphics()
{
}

bool Graphics::PreInit(Engine::SystemEnumerator& systemEnumerator)
{
	SDE_PROF_EVENT();

	m_scriptSystem = (Engine::ScriptSystem*)systemEnumerator.GetSystem("Script");
	m_renderSystem = (Engine::RenderSystem*)systemEnumerator.GetSystem("Render");
	m_inputSystem = (Engine::InputSystem*)systemEnumerator.GetSystem("Input");
	m_jobSystem = (Engine::JobSystem*)systemEnumerator.GetSystem("Jobs");
	m_debugGui = (Engine::DebugGuiSystem*)systemEnumerator.GetSystem("DebugGui");
	m_entitySystem = (EntitySystem*)systemEnumerator.GetSystem("Entities");

	// Create managers
	m_shaders = std::make_unique<Engine::ShaderManager>();
	m_textures = std::make_unique<Engine::TextureManager>(m_jobSystem);
	m_models = std::make_unique<Engine::ModelManager>(m_textures.get(), m_jobSystem);

	return true;
}

Engine::MenuBar g_graphicsMenu;
bool g_showTextureGui = false;
bool g_showModelGui = false;
bool g_useArcballCam = false;
bool g_showCameraInfo = false;
bool g_enableShadowUpdate = true;

bool Graphics::Initialise()
{
	SDE_PROF_EVENT();

	const auto& windowProps = m_renderSystem->GetWindow()->GetProperties();
	m_windowSize = glm::ivec2(windowProps.m_sizeX, windowProps.m_sizeY);

	m_entitySystem->RegisterComponentType<Transform>("Transform");
	m_entitySystem->RegisterComponentType<Light>("Light");
	m_entitySystem->RegisterComponentType<Model>("Model");
	
	//// add our renderer to the global passes
	m_renderer = std::make_unique<Engine::Renderer>(m_textures.get(), m_models.get(), m_shaders.get(), m_windowSize);
	m_renderSystem->AddPass(*m_renderer);

	// expose TextureHandle to lua
	m_scriptSystem->Globals().new_usertype<Engine::TextureHandle>("TextureHandle",sol::constructors<Engine::TextureHandle()>());

	// expose ModelHandle to lua
	m_scriptSystem->Globals().new_usertype<Engine::ModelHandle>("ModelHandle",sol::constructors<Engine::ModelHandle()>());

	// expose ShaderHandle to lua
	m_scriptSystem->Globals().new_usertype<Engine::ShaderHandle>("ShaderHandle", sol::constructors<Engine::ShaderHandle()>());

	//// expose Graphics namespace functions
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
		m_renderer->SetShadowsShader(lightingShander, shadowShader);
	};
	graphics["DrawModel"] = [this](float px, float py, float pz, float r, float g, float b, float a, float scale, Engine::ModelHandle h, Engine::ShaderHandle sh) {
		auto transform = glm::scale(glm::translate(glm::identity<glm::mat4>(), glm::vec3(px, py, pz)), glm::vec3(scale));
		m_renderer->SubmitInstance(transform, glm::vec4(r,g,b,a), h, sh);
	};
	graphics["PointLight"] = [this](float px, float py, float pz, float r, float g, float b, float ambient, float attenConst, float attenLinear, float attenQuad) {
		m_renderer->SetLight(glm::vec4(px, py, pz,1.0f), glm::vec3(r, g, b), ambient, { attenConst , attenLinear , attenQuad });
	};
	graphics["DirectionalLight"] = [this](float dx, float dy, float dz, float r, float g, float b, float ambient) {
		m_renderer->SetLight(glm::vec4(dx, dy, dz, 0.0f), glm::vec3(r, g, b), ambient, { 0.0f,0.0f,0.0f });
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

	m_debugCamera = std::make_unique<Engine::DebugCamera>();
	m_debugCamera->SetPosition({99.f,47.0f,2.4f});
	m_debugCamera->SetPitch(-0.185f);
	m_debugCamera->SetYaw(1.49f);

	m_arcballCamera = std::make_unique<Engine::ArcballCamera>();
	m_arcballCamera->SetWindowSize(m_renderSystem->GetWindow()->GetSize());
	m_arcballCamera->SetPosition({ 7.1f,8.0f,15.0f });
	m_arcballCamera->SetTarget({ 0.0f,5.0f,0.0f });
	m_arcballCamera->SetUp({ 0.0f,1.0f,0.0f });
 
	auto& gMenu = g_graphicsMenu.AddSubmenu(ICON_FK_TELEVISION " Graphics");
	gMenu.AddItem("Reload Shaders", [this]() { m_renderer->Reset(); m_shaders->ReloadAll(); });
	gMenu.AddItem("Reload Textures", [this]() { m_renderer->Reset(); m_textures->ReloadAll(); });
	gMenu.AddItem("Reload Models", [this]() { m_renderer->Reset(); m_models->ReloadAll(); });
	gMenu.AddItem("TextureManager", [this]() { g_showTextureGui = true; });
	gMenu.AddItem("ModelManager", [this]() { g_showModelGui = true; });
	gMenu.AddItem("Toggle shadow update", [this]() { 
		g_enableShadowUpdate = !g_enableShadowUpdate; 
		m_renderer->SetShadowUpdateEnabled(g_enableShadowUpdate);
	});
	auto& camMenu = g_graphicsMenu.AddSubmenu(ICON_FK_CAMERA " Camera (Arcball)");
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

void Graphics::RenderEntities()
{
	SDE_PROF_EVENT();

	auto world = m_entitySystem->GetWorld();

	// submit all lights
	{
		SDE_PROF_EVENT("SubmitLights");
		world->ForEachComponent<Light>("Light", [this, &world](Component& c, EntityHandle owner) {
			const auto& light = static_cast<const Light&>(c);
			const Transform* transform = (Transform*)world->GetComponent(owner, "Transform");
			const glm::vec3 position = transform ? transform->GetPosition() : glm::vec3(0.0f);
			const glm::vec4 posAndType = { position, light.IsPointLight() ? 1.0f : 0.0f };
			const glm::vec3 attenuation = light.GetAttenuation();
			m_renderer->SetLight(posAndType, light.GetColour(), light.GetAmbient(), attenuation);
		});
	}

	// submit all models
	{
		SDE_PROF_EVENT("SubmitEntities");
		world->ForEachComponent<Model>("Model", [this, &world](Component& c, EntityHandle owner) {
			const auto& model = static_cast<Model&>(c);
			const Transform* transform = (Transform*)world->GetComponent(owner, "Transform");
			if (transform && model.GetModel().m_index != -1 && model.GetShader().m_index != -1)
			{
				m_renderer->SubmitInstance(transform->GetMatrix(), glm::vec4(1.0f), model.GetModel(), model.GetShader());
			}
		});
	}

	if(m_showBounds)
	{
		SDE_PROF_EVENT("ShowBounds");
		world->ForEachComponent<Model>("Model", [this, &world](Component& c, EntityHandle owner) {
			const auto& model = static_cast<Model&>(c);
			const Transform* transform = (Transform*)world->GetComponent(owner, "Transform");
			if (transform && model.GetModel().m_index != -1 && model.GetShader().m_index != -1)
			{
				const auto renderModel = m_models->GetModel(model.GetModel());
				if (renderModel != nullptr)
				{
					for (const auto& part : renderModel->Parts())
					{
						const auto bmin = part.m_boundsMin;
						const auto bmax = part.m_boundsMax;
						glm::vec4 v[] = { 
							{bmin.x,bmin.y,bmin.z,1.0f},
							{bmax.x,bmin.y,bmin.z,1.0f},

							{bmax.x,bmin.y,bmin.z,1.0f},
							{bmax.x,bmin.y,bmax.z,1.0f},

							{bmax.x,bmin.y,bmax.z,1.0f},
							{bmin.x,bmin.y,bmax.z,1.0f},

							{bmin.x,bmin.y,bmax.z,1.0f},
							{bmin.x,bmin.y,bmin.z,1.0f},

							{bmin.x,bmax.y,bmin.z,1.0f},
							{bmax.x,bmax.y,bmin.z,1.0f},

							{bmax.x,bmax.y,bmin.z,1.0f},
							{bmax.x,bmax.y,bmax.z,1.0f},

							{bmax.x,bmax.y,bmax.z,1.0f},
							{bmin.x,bmax.y,bmax.z,1.0f},

							{bmin.x,bmax.y,bmax.z,1.0f},
							{bmin.x,bmax.y,bmin.z,1.0f},

							{bmin.x,bmin.y,bmin.z,1.0f},
							{bmin.x,bmax.y,bmin.z,1.0f},

							{bmax.x,bmin.y,bmin.z,1.0f},
							{bmax.x,bmax.y,bmin.z,1.0f},

							{bmax.x,bmin.y,bmax.z,1.0f},
							{bmax.x,bmax.y,bmax.z,1.0f},

							{bmin.x,bmin.y,bmax.z,1.0f},
							{bmin.x,bmax.y,bmax.z,1.0f},
						};
						const glm::vec4 c[] = {
							{1.0f,1.0f,1.0f,1.0f},{1.0f,1.0f,1.0f,1.0f},
							{1.0f,1.0f,1.0f,1.0f},{1.0f,1.0f,1.0f,1.0f},
							{1.0f,1.0f,1.0f,1.0f},{1.0f,1.0f,1.0f,1.0f},
							{1.0f,1.0f,1.0f,1.0f},{1.0f,1.0f,1.0f,1.0f},
							{1.0f,1.0f,1.0f,1.0f},{1.0f,1.0f,1.0f,1.0f},
							{1.0f,1.0f,1.0f,1.0f},{1.0f,1.0f,1.0f,1.0f},
							{1.0f,1.0f,1.0f,1.0f},{1.0f,1.0f,1.0f,1.0f},
							{1.0f,1.0f,1.0f,1.0f},{1.0f,1.0f,1.0f,1.0f},
							{1.0f,1.0f,1.0f,1.0f},{1.0f,1.0f,1.0f,1.0f},
							{1.0f,1.0f,1.0f,1.0f},{1.0f,1.0f,1.0f,1.0f},
							{1.0f,1.0f,1.0f,1.0f},{1.0f,1.0f,1.0f,1.0f},
							{1.0f,1.0f,1.0f,1.0f},{1.0f,1.0f,1.0f,1.0f},
						};

						auto finalTransform = transform->GetMatrix();
						for (auto& vert : v)
						{
							vert = finalTransform * vert;
						}

						m_debugRender->AddLines(v, c, 12);
					}
				}
			}
		});
	}
}

bool Graphics::Tick()
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

	double renderEntitiesStart = timer.GetSeconds();
	RenderEntities();
	double renderEntitiesTime = timer.GetSeconds() - renderEntitiesStart;

	if (g_showCameraInfo)
	{
		m_debugGui->BeginWindow(g_showCameraInfo, "Camera");
		glm::vec3 posVec = g_useArcballCam ? m_arcballCamera->GetPosition() : m_debugCamera->GetPosition();
		m_debugGui->DragVector("Position", posVec);
		m_debugGui->EndWindow();
	}

	Render::Camera c;
	if (g_useArcballCam)
	{
		if (!m_debugGui->IsCapturingMouse())
		{
			m_arcballCamera->Update(m_inputSystem->GetMouseState(), 0.066f);
		}
		c.LookAt(m_arcballCamera->GetPosition(), m_arcballCamera->GetTarget(), m_arcballCamera->GetUp());
	}
	else
	{
		m_debugCamera->Update(m_inputSystem->ControllerState(0), 0.016f);
		if (!m_debugGui->IsCapturingMouse())
		{
			m_debugCamera->Update(m_inputSystem->GetMouseState(), 0.016f);
		}
		if (!m_debugGui->IsCapturingKeyboard())
		{
			m_debugCamera->Update(m_inputSystem->GetKeyboardState(), 0.016f);
		}
		m_debugCamera->ApplyToCamera(c);
	}	
	m_renderer->SetCamera(c);

	m_debugRender->PushToRenderer(*m_renderer);
	m_debugGui->MainMenuBar(g_graphicsMenu);
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
	m_debugGui->BeginWindow(forceOpen,"Render Stats");
	sprintf_s(statText, "RenderEntities time: %.2fms", renderEntitiesTime * 1000.0f);	m_debugGui->Text(statText);
	sprintf_s(statText, "Total Instances: %zu", fs.m_instancesSubmitted);	m_debugGui->Text(statText);
	sprintf_s(statText, "Active Lights: %zu", fs.m_activeLights);	m_debugGui->Text(statText);
	sprintf_s(statText, "Shader Binds: %zu", fs.m_shaderBinds);	m_debugGui->Text(statText);
	sprintf_s(statText, "VA Binds: %zu", fs.m_vertexArrayBinds);	m_debugGui->Text(statText);
	sprintf_s(statText, "Batches Drawn: %zu", fs.m_batchesDrawn);	m_debugGui->Text(statText);
	sprintf_s(statText, "Draw calls: %zu", fs.m_drawCalls);	m_debugGui->Text(statText);
	sprintf_s(statText, "Total Tris: %zu", fs.m_totalVertices / 3);	m_debugGui->Text(statText);
	sprintf_s(statText, "FPS: %d", framesPerSecond);	m_debugGui->Text(statText);
	m_debugGui->Checkbox("Draw Bounds", &m_showBounds);
	m_debugGui->DragFloat("Exposure", m_renderer->GetExposure(), 0.01f, 0.0f, 100.0f);
	m_debugGui->DragFloat("Shadow Bias", m_renderer->GetShadowBias(), 0.00001f, 0.0000001f, 1.0f);
	m_debugGui->DragFloat("Cube Shadow Bias", m_renderer->GetCubeShadowBias(), 0.1f, 0.1f, 5.0f);
	m_debugGui->EndWindow();

	// Process loaded data on main thread
	m_textures->ProcessLoadedTextures();
	m_models->ProcessLoadedModels();

	return true;
}

void Graphics::Shutdown()
{
	SDE_PROF_EVENT();

	m_scriptSystem->Globals()["Graphics"] = nullptr;
	m_debugRender = nullptr;
	m_renderer = nullptr;
	m_models = nullptr;
	m_textures = nullptr;
	m_shaders = nullptr;
}