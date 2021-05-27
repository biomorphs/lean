#pragma once

#include "uniform_buffer.h"
#include <stdint.h>
#include <memory>
#include <atomic>

namespace Render
{
	class ShaderProgram;

	class Material
	{
	public:
		Material();
		~Material();

		bool GetCastsShadows() const { return m_castsShadows; }
		void SetCastsShadows(bool s) { m_castsShadows = s; }
		bool GetIsTransparent() const { return m_isTransparent; }
		void SetIsTransparent(bool m) { m_isTransparent = m; }
		inline UniformBuffer& GetUniforms()							{ return m_uniforms; }
		inline const UniformBuffer& GetUniforms() const				{ return m_uniforms; }

		struct Sampler {
			std::string m_name;
			uint32_t m_handle = -1;		// can be anything really
		};
		using Samplers = std::unordered_map<uint32_t, Sampler>;
		void SetSampler(std::string name, uint32_t handle);
		const Samplers& GetSamplers() const { return m_samplers; }
		Samplers& GetSamplers() { return m_samplers; }
	private:
		Samplers m_samplers;
		UniformBuffer m_uniforms;
		bool m_isTransparent = false;
		bool m_castsShadows = true;
	};
}