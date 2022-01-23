#pragma once
#include "serialisation.h"
#include "system.h"
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
		SERIALISED_CLASS();
		uint32_t m_index = -1;
		static ShaderHandle Invalid() { return { (uint32_t)-1 }; };
	};

	class ShaderManager : public System
	{
	public:
		using CustomDefines = std::vector<std::tuple<std::string, std::string>>;	// name, value (optional)
		std::vector<ShaderHandle> AllShaders() const;
		ShaderHandle LoadComputeShader(const char* name, const char* path, const CustomDefines& customDefines);
		ShaderHandle LoadComputeShader(const char* name, const char* path);
		ShaderHandle LoadShader(const char* name, const char* vsPath, const char* fsPath);
		Render::ShaderProgram* GetShader(const ShaderHandle& h);
		std::string GetShaderName(const ShaderHandle& h) const;
		bool GetShaderPaths(const ShaderHandle& h, std::string& vs, std::string& fs);
		void SetShadowsShader(ShaderHandle lightingShader, ShaderHandle shadowShader);
		ShaderHandle GetShadowsShader(ShaderHandle lightingShader);

		virtual void Shutdown();

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