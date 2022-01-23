#include "component_material.h"
#include "engine/system_manager.h"
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

SERIALISE_BEGIN(Material)
if (op == Engine::SerialiseType::Write)
{
	bool castingShadows = m_material->GetCastsShadows();
	Engine::ToJson("CastShadow", castingShadows, json);
	bool isTransparent = m_material->GetIsTransparent();
	Engine::ToJson("IsTransparent", isTransparent, json);
	
	auto writeUniformsToJson = [](const char* name, nlohmann::json& target, auto& uniforms) {
		std::vector<nlohmann::json> uniformsJson;
		uniformsJson.reserve(uniforms.size());
		for (auto& v : uniforms)
		{
			nlohmann::json& uniformJson = uniformsJson.emplace_back();
			Engine::ToJson("Name", v.second.m_name, uniformJson);
			Engine::ToJson("Value", v.second.m_value, uniformJson);
		}
		target[name] = std::move(uniformsJson);
	};
	writeUniformsToJson("Float", json, m_material->GetUniforms().FloatValues());
	writeUniformsToJson("Vec4", json, m_material->GetUniforms().Vec4Values());
	writeUniformsToJson("Mat4", json, m_material->GetUniforms().Mat4Values());
	writeUniformsToJson("Int", json, m_material->GetUniforms().IntValues());
	
	std::vector<nlohmann::json> samplersJson;
	for (const auto& sampler : m_material->GetSamplers())
	{
		nlohmann::json& thisSamplerData = samplersJson.emplace_back();
		Engine::ToJson("Name", sampler.second.m_name, thisSamplerData);
		auto& valueJson = thisSamplerData["Texture"];
		Engine::TextureHandle texHandle{ sampler.second.m_handle };
		Engine::ToJson(texHandle, valueJson);
	}
	json["Samplers"] = samplersJson;
}
else
{
	bool castShadow = false;
	Engine::FromJson("CastShadow", castShadow, json);
	bool isTransparent = false;
	Engine::FromJson("IsTransparent", isTransparent, json);

	auto readUniforms = [this](const char* name, nlohmann::json& src, auto& uniforms)
	{
		nlohmann::json& uniformsJson = src[name];
		int count = uniformsJson.size();
		for (int i = 0; i < count; ++i)
		{
			std::decay<decltype(uniforms)>::type::mapped_type newUniform;
			Engine::FromJson("Name", newUniform.m_name, uniformsJson[i]);
			Engine::FromJson("Value", newUniform.m_value, uniformsJson[i]);
			m_material->GetUniforms().SetValue(newUniform.m_name, newUniform.m_value);
		}
	};
	readUniforms("Float", json, m_material->GetUniforms().FloatValues());
	readUniforms("Vec4", json, m_material->GetUniforms().Vec4Values());
	readUniforms("Mat4", json, m_material->GetUniforms().Mat4Values());
	readUniforms("Int", json, m_material->GetUniforms().IntValues());

	auto& samplersJson = json["Samplers"];
	for (int index = 0; index < samplersJson.size(); ++index)
	{
		std::string samplerName = "";
		Engine::TextureHandle texture;
		Engine::FromJson("Name", samplerName, samplersJson[index]);
		Engine::FromJson("Texture", texture, samplersJson[index]);
		SetSampler(samplerName.c_str(), texture);
	}
}
SERIALISE_END()

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

COMPONENT_INSPECTOR_IMPL(Material, Engine::DebugGuiSystem& gui)
{
	auto fn = [&gui](ComponentStorage& cs, const EntityHandle& e)
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
				auto& textures = *Engine::GetSystem<Engine::TextureManager>("Textures");
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
				gui.TreePop();
			}
		}
	};
	return fn;
}