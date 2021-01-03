#include "uniform_buffer.h"
#include "render/device.h"
#include "render/shader_program.h"
#include "core/string_hashing.h"

namespace Render
{
	template<class T>
	void ApplyValues(Render::Device& d, Render::ShaderProgram& p, const T& values)
	{
		for (const auto& it : values)
		{
			auto uniformHandle = p.GetUniformHandle(it.second.m_name.c_str(), it.first);
			if (uniformHandle != -1)
				d.SetUniformValue(uniformHandle, it.second.m_value);
		}
	}

	template<class T>
	void ApplyDefaults(Render::Device& d, Render::ShaderProgram& p, const T& values, const T& defaults)
	{
		for (const auto& it : defaults)
		{
			if (values.find(it.first) == values.end())
			{
				auto uniformHandle = p.GetUniformHandle(it.second.m_name.c_str(), it.first);
				if (uniformHandle != -1)
					d.SetUniformValue(uniformHandle, it.second.m_value);
			}
		}
	}

	void UniformBuffer::Apply(Render::Device& d, Render::ShaderProgram& p, const UniformBuffer& defaults) const
	{
		ApplyValues(d, p, m_floatValues);
		ApplyDefaults(d, p, m_floatValues, defaults.m_floatValues);
		ApplyValues(d, p, m_vec4Values);
		ApplyDefaults(d, p, m_vec4Values, defaults.m_vec4Values);
		ApplyValues(d, p, m_mat4Values);
		ApplyDefaults(d, p, m_mat4Values, defaults.m_mat4Values);
		ApplyValues(d, p, m_intValues);
		ApplyDefaults(d, p, m_intValues, defaults.m_intValues);
	}

	void UniformBuffer::Apply(Render::Device& d, Render::ShaderProgram& p) const
	{
		ApplyValues(d, p, m_floatValues);
		ApplyValues(d, p, m_vec4Values);
		ApplyValues(d, p, m_mat4Values);
		ApplyValues(d, p, m_intValues);
	}

	void UniformBuffer::SetValue(std::string name, int32_t value)
	{
		const uint32_t hash = Core::StringHashing::GetHash(name.c_str());
		m_intValues[hash] = { name, value };
	}

	void UniformBuffer::SetValue(std::string name, float value)
	{
		const uint32_t hash = Core::StringHashing::GetHash(name.c_str());
		m_floatValues[hash] = { name, value };
	}

	void UniformBuffer::SetValue(std::string name, const glm::mat4& value)
	{
		const uint32_t hash = Core::StringHashing::GetHash(name.c_str());
		m_mat4Values[hash] = { name, value };
	}

	void UniformBuffer::SetValue(std::string name, const glm::vec4& value)
	{
		const uint32_t hash = Core::StringHashing::GetHash(name.c_str());
		m_vec4Values[hash] = { name, value };
	}
}