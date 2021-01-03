/*
SDLEngine
Matt Hoyle
*/
#pragma once

#include "uniform_buffer.h"
#include <stdint.h>
#include <memory>

namespace Render
{
	class ShaderProgram;

	class Material
	{
	public:
		Material();
		~Material();

		inline UniformBuffer& GetUniforms()							{ return m_uniforms; }
		inline const UniformBuffer& GetUniforms() const				{ return m_uniforms; }

		struct Sampler {
			std::string m_name;
			uint32_t m_handle;		// can be anything really
		};
		using Samplers = std::unordered_map<uint32_t, Sampler>;
		void SetSampler(std::string name, uint32_t handle);
		const Samplers& GetSamplers() const { return m_samplers; }
	private:
		Samplers m_samplers;
		UniformBuffer m_uniforms;
	};
}