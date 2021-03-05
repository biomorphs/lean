#pragma once

#include "core/glm_headers.h"
#include <unordered_map>
#include <string>

namespace Render
{
	class Texture;
	class Device;
	class ShaderProgram;

	// Contains a set of name -> value pairs for a set of uniforms + samplers
	class UniformBuffer
	{
	public:
		UniformBuffer() = default;
		~UniformBuffer() = default;
		UniformBuffer(const UniformBuffer& other) = default;
		UniformBuffer(UniformBuffer&& other) = default;
		void Apply(Render::Device& d, Render::ShaderProgram& p) const;
		void Apply(Render::Device& d, Render::ShaderProgram& p, const UniformBuffer& defaults) const;

		template <class T>
		struct Uniform {
			std::string m_name;
			T m_value = {};
		};
		using FloatUniforms = std::unordered_map<uint32_t, Uniform<float>>;
		using Vec4Uniforms = std::unordered_map<uint32_t, Uniform<glm::vec4>>;
		using Mat4Uniforms = std::unordered_map<uint32_t, Uniform<glm::mat4>>;
		using IntUniforms = std::unordered_map<uint32_t, Uniform<int32_t>>;
		void SetValue(std::string name, float value);
		void SetValue(std::string name, const glm::vec4& value);
		void SetValue(std::string name, const glm::mat4& value);
		void SetValue(std::string name, int32_t value);
		const FloatUniforms& FloatValues() const { return m_floatValues; }
		const Vec4Uniforms& Vec4Values() const { return m_vec4Values; }
		const Mat4Uniforms& Mat4Values() const { return m_mat4Values; }
		const IntUniforms& IntValues() const { return m_intValues; }
		FloatUniforms& FloatValues() { return m_floatValues; }
		Vec4Uniforms& Vec4Values() { return m_vec4Values; }
		Mat4Uniforms& Mat4Values() { return m_mat4Values; }
		IntUniforms& IntValues() { return m_intValues; }

	private:
		FloatUniforms m_floatValues;
		Vec4Uniforms m_vec4Values;
		Mat4Uniforms m_mat4Values;
		IntUniforms m_intValues;
	};
}