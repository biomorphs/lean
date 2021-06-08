#include "shader_manager.h"
#include "render/shader_program.h"
#include "render/shader_binary.h"
#include "core/profiler.h"
#include "core/log.h"

namespace Engine
{
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

	void ShaderManager::ReloadAll()
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
}