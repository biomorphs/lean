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
	void ApplyMaterial(Render::Device& d, Render::ShaderProgram& shader, const Render::Material& m, DefaultTextures* defaults);
	void ApplyMaterial(Render::Device& d, Render::ShaderProgram& shader, const Render::Material& m);
}
