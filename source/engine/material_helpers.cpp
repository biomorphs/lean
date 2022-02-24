#include "material_helpers.h"
#include "render/device.h"
#include "render/shader_program.h"
#include "render/material.h"
#include "engine/system_manager.h"
#include "core/profiler.h"
#include "engine/texture_manager.h"

namespace Engine
{
	void ApplyMaterial(Render::Device& d, Render::ShaderProgram& shader, const Render::Material& m)
	{
		SDE_PROF_EVENT();

		static Engine::TextureManager* s_tm = Engine::GetSystem<Engine::TextureManager>("Textures");
		const auto& uniforms = m.GetUniforms();
		uniforms.Apply(d, shader);

		const auto& samplers = m.GetSamplers();
		for (const auto& s : samplers)
		{
			uint32_t uniformHandle = shader.GetUniformHandle(s.second.m_name.c_str());
			if (uniformHandle != -1)
			{
				// todo - remove indirection here, samplers should be a list of texture handle NOT ints
				TextureHandle texHandle = { s.second.m_handle };
				const auto theTexture = s_tm->GetTexture({ texHandle });
				if (theTexture)
				{
					d.SetSampler(uniformHandle, theTexture->GetResidentHandle());
				}
			}
		}
	}

	void ApplyMaterial(Render::Device& d, Render::ShaderProgram& shader, const Render::Material& m, DefaultTextures* defaults)
	{
		SDE_PROF_EVENT();

		static Engine::TextureManager* s_tm = Engine::GetSystem<Engine::TextureManager>("Textures");
		const auto& uniforms = m.GetUniforms();
		uniforms.Apply(d, shader);

		const auto& samplers = m.GetSamplers();
		for (const auto& s : samplers)
		{
			uint32_t uniformHandle = shader.GetUniformHandle(s.second.m_name.c_str());
			if (uniformHandle != -1)
			{
				// todo - remove indirection here, samplers should be a list of texture handle NOT ints
				TextureHandle texHandle = { s.second.m_handle };	
				const auto theTexture = s_tm->GetTexture({ texHandle });
				if (theTexture)
				{
					d.SetSampler(uniformHandle, theTexture->GetResidentHandle());
				}
				else if(defaults)
				{
					// set default if one exists
					auto foundDefault = defaults->find(s.second.m_name.c_str());
					if (foundDefault != defaults->end())
					{
						const auto defaultTexture = s_tm->GetTexture({ foundDefault->second });
						if (defaultTexture != nullptr)
						{
							d.SetSampler(uniformHandle, defaultTexture->GetResidentHandle());
						}
					}
				}
			}
		}
	}
}
