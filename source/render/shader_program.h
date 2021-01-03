/*
SDLEngine
Matt Hoyle
*/
#pragma once

#include <stdint.h>
#include <string>
#include <unordered_map>

namespace Render
{
	class ShaderBinary;

	// This represents a 'linked' set of shaders, which are used to render stuff
	class ShaderProgram
	{
	public:
		ShaderProgram();
		~ShaderProgram();

		bool Create(const ShaderBinary& vertexShader, const ShaderBinary& fragmentShader, std::string& result);
		void Destroy();

		void AddUniform(const char* uniformName);
		uint32_t GetUniformHandle(const char* uniformName, uint32_t nameHash);	// lazily updates cache
		uint32_t GetUniformHandle(const char* uniformName);
		uint32_t GetUniformBufferBlockIndex(const char* bufferName) const;

		inline uint32_t GetHandle() const { return m_handle; }

	private:
		uint32_t m_handle;
		std::unordered_map<uint32_t, uint32_t> m_uniformHandles;	// map of uniform name hash -> uniform handle
	};
}