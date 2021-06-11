#include "component_material.h"
#include "engine/file_picker_dialog.h"
#include "engine/texture_manager.h"
#include "engine/debug_gui_system.h"
#include "entity/entity_handle.h"
#include "render/material.h"

COMPONENT_SCRIPTS(Material,
	"SetFloat", &Material::SetFloat,
	"SetVec4", &Material::SetVec4,
	"SetMat4", &Material::SetMat4,
	"SetInt32", &Material::SetInt32,
	"SetSampler", &Material::SetSampler,
	"SetIsTransparent", &Material::SetIsTransparent,
	"SetCastShadows", &Material::SetCastShadows
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

void Material::SetCastShadows(bool s)
{
	m_material->SetCastsShadows(s);
}

COMPONENT_INSPECTOR_IMPL(Material, Engine::DebugGuiSystem& gui, Engine::TextureManager& textures)
{
	auto fn = [&gui, &textures](ComponentStorage& cs, const EntityHandle& e)
	{
		auto& m = *static_cast<StorageType&>(cs).Find(e);
		auto& rmat = m.GetRenderMaterial();
		auto& uniforms = rmat.GetUniforms();
		auto& samplers = rmat.GetSamplers();
		char text[1024] = { '\0' };
		m.SetIsTransparent(gui.Checkbox("Transparent", m.GetRenderMaterial().GetIsTransparent()));
		m.SetCastShadows(gui.Checkbox("Cast Shadows", m.GetRenderMaterial().GetCastsShadows()));
		for (auto& v : uniforms.FloatValues())
		{
			sprintf_s(text, "%s", v.second.m_name.c_str());
			v.second.m_value = gui.DragFloat(text, v.second.m_value, 0.01f);
		}
		for (auto& v : uniforms.Vec4Values())
		{
			sprintf_s(text, "%s", v.second.m_name.c_str());
			v.second.m_value = gui.DragVector(text, v.second.m_value, 0.01f);
		}
		for (auto& v : uniforms.IntValues())
		{
			sprintf_s(text, "%s", v.second.m_name.c_str());
			v.second.m_value = gui.DragInt(text, v.second.m_value);
		}
		for (auto& t : samplers)
		{
			sprintf_s(text, "%s", t.second.m_name.c_str());
			if (t.second.m_handle != 0 && gui.TreeNode(text))
			{
				auto texture = textures.GetTexture({ t.second.m_handle });
				auto path = textures.GetTexturePath({ t.second.m_handle });
				if (texture)
				{
					gui.Image(*texture, { 256,256 });
				}
				sprintf_s(text, "%s", t.second.m_name.c_str());
				if (gui.Button(text))
				{
					std::string newFile = Engine::ShowFilePicker("Select Texture", "", "JPG (.jpg)\0*.jpg\0PNG (.png)\0*.png\0BMP (.bmp)\0*.bmp\0");
					if (newFile != "")
					{
						auto loadedTexture = textures.LoadTexture(newFile.c_str());
						t.second.m_handle = loadedTexture.m_index;
					}
				}
			}
		}
	};
	return fn;
}