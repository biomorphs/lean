#pragma once
#include "render/shader_program.h"
#include <stdint.h>
#include <string>
#include <vector>
#include <memory>

namespace Engine
{
	struct ShaderHandle
	{
		uint16_t m_index = -1;
		static ShaderHandle Invalid() { return { (uint16_t)-1 }; };
	};

	class ShaderManager
	{
	public:
		ShaderManager() = default;
		~ShaderManager() = default;
		ShaderManager(const ShaderManager&) = delete;
		ShaderManager(ShaderManager&&) = delete;

		std::vector<ShaderHandle> AllShaders() const;
		ShaderHandle LoadShader(const char* name, const char* vsPath, const char* fsPath);
		Render::ShaderProgram* GetShader(const ShaderHandle& h);
		bool GetShaderPaths(const ShaderHandle& h, std::string& vs, std::string& fs);

		void ReloadAll();

	private:
		struct ShaderDesc {
			std::unique_ptr<Render::ShaderProgram> m_shader;
			std::string m_name;
			std::string m_vsPath;
			std::string m_fsPath;
		};
		std::vector<ShaderDesc> m_shaders;
	};
}