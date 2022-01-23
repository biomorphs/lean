#pragma once
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
	struct TextureHandle;
	using DefaultTextures = std::map<std::string, TextureHandle>;
	uint32_t ApplyMaterial(Render::Device& d, Render::ShaderProgram& shader, const Render::Material& m, DefaultTextures* defaults = nullptr, uint32_t textureUnit=0);
}
