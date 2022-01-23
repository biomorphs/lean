#include "material_helpers.h"
#include "render/device.h"
#include "render/shader_program.h"
#include "render/material.h"
#include "engine/system_manager.h"
#include "texture_manager.h"

namespace Engine
{
	uint32_t ApplyMaterial(Render::Device& d, Render::ShaderProgram& shader, const Render::Material& m, DefaultTextures* defaults, uint32_t textureUnit)
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
				TextureHandle texHandle = { s.second.m_handle, s_tm };	// sketchy!
				const auto theTexture = s_tm->GetTexture({ texHandle });
				if (theTexture)
				{
					d.SetSampler(uniformHandle, theTexture->GetHandle(), textureUnit++);
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
							d.SetSampler(uniformHandle, defaultTexture->GetHandle(), textureUnit++);
						}
					}
				}
			}
		}
		return textureUnit;
	}
}
