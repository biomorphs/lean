#include "component_material.h"
#include "engine/texture_manager.h"
#include "render/material.h"

COMPONENT_SCRIPTS(Material,
	"SetFloat", &Material::SetFloat,
	"SetVec4", &Material::SetVec4,
	"SetMat4", &Material::SetMat4,
	"SetInt32", &Material::SetInt32,
	"SetSampler", &Material::SetSampler,
	"SetIsTransparent", &Material::SetIsTransparent
)


Material::Material()
{
	m_material = std::make_unique<Render::Material>();
}

void Material::SetFloat(const char* name, float v)
{ 
	m_material->GetUniforms().SetValue(name, v); 
}

void Material::SetVec4(const char* name, glm::vec4 v)
{ 
	m_material->GetUniforms().SetValue(name, v);
}

void Material::SetMat4(const char* name, glm::mat4 v)
{ 
	m_material->GetUniforms().SetValue(name, v);
}

void Material::SetInt32(const char* name, int32_t v)
{ 
	m_material->GetUniforms().SetValue(name, v);
}

void Material::SetSampler(const char* name, const Engine::TextureHandle& v)
{ 
	m_material->SetSampler(name, v.m_index);
}

void Material::SetIsTransparent(bool t)
{ 
	m_material->SetIsTransparent(t);
}