/*
SDLEngine
Matt Hoyle
*/
#include "material.h"
#include "core/string_hashing.h"

namespace Render
{
	Material::Material()
	{

	}

	Material::~Material()
	{

	}

	void Material::SetSampler(std::string name, uint32_t handle)
	{
		const uint32_t hash = Core::StringHashing::GetHash(name.c_str());
		m_samplers[hash] = { name, handle };
	}
}