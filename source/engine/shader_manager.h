#pragma once
#include "render/shader_program.h"
#include "robin_hood.h"
#include <stdint.h>
#include <string>
#include <vector>
#include <memory>

namespace Engine
{
	struct ShaderHandle
	{
		uint32_t m_index = -1;
		static ShaderHandle Invalid() { return { (uint32_t)-1 }; };
	};

	class ShaderManager
	{
	public:
		ShaderManager() = default;
		~ShaderManager() = default;
		ShaderManager(const ShaderManager&) = delete;
		ShaderManager(ShaderManager&&) = delete;

		std::vector<ShaderHandle> AllShaders() const;
		ShaderHandle LoadComputeShader(const char* name, const char* path);
		ShaderHandle LoadShader(const char* name, const char* vsPath, const char* fsPath);
		Render::ShaderProgram* GetShader(const ShaderHandle& h);
		bool GetShaderPaths(const ShaderHandle& h, std::string& vs, std::string& fs);
		void SetShadowsShader(ShaderHandle lightingShader, ShaderHandle shadowShader);
		ShaderHandle GetShadowsShader(ShaderHandle lightingShader);

		void ReloadAll();

	private:
		struct ShaderDesc {
			std::unique_ptr<Render::ShaderProgram> m_shader;
			std::string m_name;
			std::string m_vsPath;	// may also be compute shader path
			std::string m_fsPath;
		};
		std::vector<ShaderDesc> m_shaders;
		robin_hood::unordered_map<uint32_t, ShaderHandle> m_shadowShaders;	// map of lighting shader handle index -> shadow shader
	};
}