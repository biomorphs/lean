#pragma once
#include "texture_manager.h"
#include <stdint.h>
#include <map>
#include <string>

namespace Render
{
	class Device;
	class ShaderProgram;
	class Material;
}

namespace Engine
{
	using DefaultTextures = std::map<std::string, TextureHandle>;
	uint32_t ApplyMaterial(Render::Device& d, Render::ShaderProgram& shader, const Render::Material& m, DefaultTextures* defaults, uint32_t textureUnit);

	uint32_t ApplyMaterial(Render::Device& d, Render::ShaderProgram& shader, const Render::Material& m);
}
