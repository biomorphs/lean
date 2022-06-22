#include "shader_manager.h"
#include "system_manager.h"
#include "debug_gui_system.h"
#include "debug_gui_menubar.h"
#include "render/shader_program.h"
#include "render/shader_binary.h"
#include "core/profiler.h"
#include "core/log.h"
#include "core/file_io.h"

namespace Engine
{
	SERIALISE_BEGIN(ShaderHandle)
		static ShaderManager* sm = GetSystem<ShaderManager>("Shaders");
		std::string name;
		std::string vs, fs;
		if (op == Engine::SerialiseType::Write)
		{
			name = sm->GetShaderName(*this);
			sm->GetShaderPaths(*this, vs, fs);
			Engine::ToJson("Name", name, json);
			if (vs.length() > 0)
			{
				Engine::ToJson("VSCS", vs, json);
			}
			if (fs.length() > 0)
			{
				Engine::ToJson("FS", fs, json);
			}
		}
		else
		{
			Engine::FromJson("Name", name, json);
			Engine::FromJson("VSCS", vs, json);
			Engine::FromJson("FS", fs, json);
			if (vs.length() > 0 && fs.length() == 0 && vs.find(".cs") != -1)
			{
				*this = sm->LoadComputeShader(name.c_str(), vs.c_str());
			}
			else if (vs.length() > 0 && fs.length() > 0)
			{
				*this = sm->LoadShader(name.c_str(), vs.c_str(), fs.c_str());
			}
		}
	SERIALISE_END()

	ShaderHandle ShaderManager::GetShadowsShader(ShaderHandle lightingShader)
	{
		const auto& foundShadowShader = m_shadowShaders.find(lightingShader.m_index);
		if (foundShadowShader != m_shadowShaders.end())
		{
			return foundShadowShader->second;
		}
		else
		{
			return ShaderHandle::Invalid();
		}
	}

	void ShaderManager::SetShadowsShader(ShaderHandle lightingShader, ShaderHandle shadowShader)
	{
		m_shadowShaders[lightingShader.m_index] = shadowShader;
	}

	std::vector<ShaderHandle> ShaderManager::AllShaders() const
	{
		std::vector<ShaderHandle> results;
		results.reserve(m_shaders.size());
		for (int s=0; s < m_shaders.size(); ++s)
		{
			results.push_back({ (uint32_t)s });
		}
		return results;
	}

	void ShaderManager::DoReloadAll()
	{
		SDE_PROF_EVENT();
		auto currentShaders = std::move(m_shaders);
		for (auto &s : currentShaders)
		{
			ShaderHandle newHandle;
			if (s.m_fsPath.size() > 0)
			{
				newHandle = LoadShader(s.m_name.c_str(), s.m_vsPath.c_str(), s.m_fsPath.c_str());
			}
			else
			{
				newHandle = LoadComputeShader(s.m_name.c_str(), s.m_vsPath.c_str());
			}
			if (newHandle.m_index == ShaderHandle::Invalid().m_index)	// if compilation failed use the old shader
			{
				m_shaders.emplace_back(std::move(s));
			}
		}
	}

	bool ShaderManager::HotReloader::Tick(float timeDelta)
	{
		ShaderManager* sm = GetSystem<ShaderManager>("Shaders");
		if (sm->m_shouldReloadAll)
		{
			sm->DoReloadAll();
			sm->m_shouldReloadAll = false;
		}
		return true;
	}

	ShaderHandle ShaderManager::LoadComputeShader(const char* name, const char* path, const CustomDefines& customDefines)
	{
		SDE_PROF_EVENT();
		for (uint64_t i = 0; i < m_shaders.size(); ++i)
		{
			if (m_shaders[i].m_name == name)
			{
				return { static_cast<uint32_t>(i) };
			}
		}

		std::string shaderSource;
		if (!Core::LoadTextFromFile(path, shaderSource))
		{
			SDE_LOG("Failed to load shader source from %s", path);
			return ShaderHandle::Invalid();
		}

		for (const auto& it : customDefines)
		{
			size_t foundPos = shaderSource.find(std::get<0>(it), 0);
			while (foundPos != std::string::npos)
			{
				shaderSource.replace(foundPos, std::get<0>(it).length(), std::get<1>(it));
				foundPos = shaderSource.find(std::get<0>(it), foundPos);
			}
		}

		std::string errorText;
		auto computeShader = std::make_unique<Render::ShaderBinary>();
		if (!computeShader->CompileFromBuffer(Render::ShaderType::ComputeShader, shaderSource, errorText))
		{
			SDE_LOG("Compute shader compilation failed - \n%s\n%s", shaderSource.c_str(), errorText.c_str());
			return ShaderHandle::Invalid();
		}

		auto shader = std::make_unique<Render::ShaderProgram>();
		if (!shader->Create(*computeShader, errorText))
		{
			SDE_LOG("Shader linkage failed - %s", errorText.c_str());
			return ShaderHandle::Invalid();
		}

		m_shaders.push_back({ std::move(shader), name, path, "" });
		return ShaderHandle{ static_cast<uint32_t>(m_shaders.size() - 1) };
	}

	ShaderHandle ShaderManager::LoadComputeShader(const char* name, const char* path)
	{
		SDE_PROF_EVENT();
		for (uint64_t i = 0; i < m_shaders.size(); ++i)
		{
			if (m_shaders[i].m_name == name)
			{
				return { static_cast<uint32_t>(i) };
			}
		}

		auto shader = std::make_unique<Render::ShaderProgram>();
		auto computeShader = std::make_unique<Render::ShaderBinary>();
		std::string errorText;
		if (!computeShader->CompileFromFile(Render::ShaderType::ComputeShader, path, errorText))
		{
			SDE_LOG("Compute shader compilation failed - %s\n%s", path, errorText.c_str());
			return ShaderHandle::Invalid();
		}

		if (!shader->Create(*computeShader, errorText))
		{
			SDE_LOG("Shader linkage failed - %s", errorText.c_str());
			return ShaderHandle::Invalid();
		}

		m_shaders.push_back({ std::move(shader), name, path, "" });
		return ShaderHandle{ static_cast<uint32_t>(m_shaders.size() - 1) };
	}

	ShaderHandle ShaderManager::LoadShader(const char* name, const char* vsPath, const char* fsPath)
	{
		SDE_PROF_EVENT();
		for (uint64_t i = 0; i < m_shaders.size(); ++i)
		{
			if (m_shaders[i].m_name == name)
			{
				return { static_cast<uint32_t>(i) };
			}
		}

		auto shader = std::make_unique<Render::ShaderProgram>();
		auto vertexShader = std::make_unique<Render::ShaderBinary>();
		std::string errorText;
		if (!vertexShader->CompileFromFile(Render::ShaderType::VertexShader, vsPath, errorText))
		{
			SDE_LOG("Vertex shader compilation failed - %s\n%s", vsPath,errorText.c_str());
			return ShaderHandle::Invalid();
		}
		auto fragmentShader = std::make_unique<Render::ShaderBinary>();
		if (!fragmentShader->CompileFromFile(Render::ShaderType::FragmentShader, fsPath, errorText))
		{
			SDE_LOG("Fragment shader compilation failed - %s\n%s", fsPath, errorText.c_str());
			return ShaderHandle::Invalid();
		}

		if (!shader->Create(*vertexShader, *fragmentShader, errorText))
		{
			SDE_LOG("Shader linkage failed - %s", errorText.c_str());
			return ShaderHandle::Invalid();
		}

		m_shaders.push_back({ std::move(shader), name, vsPath, fsPath });
		return ShaderHandle{ static_cast<uint32_t>(m_shaders.size() - 1) };
	}

	std::string ShaderManager::GetShaderName(const ShaderHandle& h) const
	{
		if (h.m_index != -1 && h.m_index < m_shaders.size())
		{
			return m_shaders[h.m_index].m_name;
		}
		else
		{
			return {};
		}
	}

	bool ShaderManager::GetShaderPaths(const ShaderHandle& h, std::string& vs, std::string& fs)
	{
		if (h.m_index != -1 && h.m_index < m_shaders.size())
		{
			vs = m_shaders[h.m_index].m_vsPath;
			fs = m_shaders[h.m_index].m_fsPath;
			return true;
		}
		else
		{
			return false;
		}
	}

	Render::ShaderProgram* ShaderManager::GetShader(const ShaderHandle& h)
	{
		if (h.m_index != -1 && h.m_index < m_shaders.size())
		{
			return m_shaders[h.m_index].m_shader.get();
		}
		else
		{
			return nullptr;
		}
	}

	bool ShaderManager::ShowGui()
	{
		static bool s_showWindow = false;

		DebugGuiSystem& gui = *GetSystem<DebugGuiSystem>("DebugGui");

		Engine::MenuBar menuBar;
		auto& fileMenu = menuBar.AddSubmenu(ICON_FK_PAINT_BRUSH " Assets");
		fileMenu.AddItem("Shader Manager", [this]() {
			s_showWindow = !s_showWindow;
			});
		gui.MainMenuBar(menuBar);

		if (s_showWindow && gui.BeginWindow(s_showWindow, "Shader Manager"))
		{
			if (gui.Button("Reload all"))
			{
				ReloadAll();
			}
			char text[1024] = { '\0' };
			if (gui.BeginListbox("Loaded shaders"))
			{
				for (const auto& s : m_shaders)
				{
					sprintf_s(text, "%s: %s%s%s (gl handle %d)", 
						s.m_name.c_str(), 
						s.m_vsPath.c_str(), 
						s.m_fsPath.size() > 0 ? " / " : "", 
						s.m_fsPath.c_str(),
						s.m_shader ? s.m_shader->GetHandle() : -1);
					gui.Text(text);
				}
				gui.EndListbox();
			}
			if (gui.BeginListbox("Shadow shaders"))
			{
				for (const auto& s : m_shadowShaders)
				{
					std::string shaderName = "unknown (-1)";
					std::string shadowName = "unknown (-1)";
					if (s.first >= 0 && s.first < m_shaders.size())
					{
						shaderName = m_shaders[s.first].m_name;
					}
					if (s.second.m_index >= 0 && s.second.m_index < m_shaders.size())
					{
						shadowName = m_shaders[s.second.m_index].m_name;
					}
					sprintf_s(text, "%s: %s", shaderName.c_str(), shadowName.c_str());
					gui.Text(text);
				}
				gui.EndListbox();
			}
			gui.EndWindow();
		}

		return s_showWindow;
	}

	bool ShaderManager::Tick(float timeDelta)
	{
		ShowGui();
		return true;
	}

	void ShaderManager::Shutdown()
	{
		m_shadowShaders.clear();
		m_shaders.clear();
	}
}