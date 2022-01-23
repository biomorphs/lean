#pragma once

#include "core/glm_headers.h"
#include "entity/component.h"
#include <memory>

namespace Render
{
	class Material;
}
namespace Engine
{
	class TextureHandle;
	class DebugGuiSystem;
}

// A material component can (and should!) be shared across entities for better batching
class Material
{
public:
	Material();
	COMPONENT(Material);
	COMPONENT_INSPECTOR(Engine::DebugGuiSystem& gui);

	const Render::Material& GetRenderMaterial() const { return *m_material; }
	Render::Material& GetRenderMaterial() { return *m_material; }

	// Mainly helpers for scripts
	void SetFloat(const char* name, float v); 
	void SetVec4(const char* name, glm::vec4 v); 
	void SetMat4(const char* name, glm::mat4 v); 
	void SetInt32(const char* name, int32_t v);
	void SetSampler(const char* name, const Engine::TextureHandle& v); 
	void SetIsTransparent(bool t);
	void SetCastShadows(bool s);

private:
	std::unique_ptr<Render::Material> m_material;
};