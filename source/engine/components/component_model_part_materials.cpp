#include "component_model_part_materials.h"
#include "component_model.h"
#include "entity/entity_system.h"
#include "entity/component_inspector.h"
#include "engine/debug_gui_system.h"
#include "engine/texture_manager.h"
#include "engine/file_picker_dialog.h"
#include "engine/system_manager.h"

COMPONENT_SCRIPTS(ModelPartMaterials)

SERIALISE_BEGIN(ModelPartMaterialOverride)
SERIALISE_PROPERTY("DiffuseOpacity", m_diffuseOpacity)
SERIALISE_PROPERTY("Specular", m_specular)
SERIALISE_PROPERTY("Shininess", m_shininess)
SERIALISE_PROPERTY("DiffuseTexture", m_diffuseTexture)
SERIALISE_PROPERTY("NormalTexture", m_normalsTexture)
SERIALISE_PROPERTY("SpecularTexture", m_specularTexture)
SERIALISE_END()

SERIALISE_BEGIN(ModelPartMaterials)
SERIALISE_PROPERTY("Parts", m_partMaterials)
SERIALISE_END()

COMPONENT_INSPECTOR_IMPL(ModelPartMaterials, Engine::DebugGuiSystem& gui)
{
	auto fn = [&gui](ComponentInspector& i, ComponentStorage& cs, const EntityHandle& e)
	{
		auto& m = *static_cast<ModelPartMaterials::StorageType&>(cs).Find(e);
		auto entities = Engine::GetSystem<EntitySystem>("Entities");
		auto models = Engine::GetSystem<Engine::ModelManager>("Models");
		if (gui.Button("Populate From Mesh"))
		{
			auto modelCmp = entities->GetWorld()->GetComponent<Model>(e);
			if (modelCmp != nullptr && modelCmp->GetModel().m_index != -1)
			{
				auto theModel = models->GetModel(modelCmp->GetModel());
				if (theModel)
				{
					m.Materials().clear();
					for (const auto& part : theModel->MeshParts())
					{
						m.Materials().push_back({ part.m_drawData });
					}
				}
			}
		}
		if (gui.Button("Clear"))
		{
			m.Materials().clear();
		}
		auto& textures = *Engine::GetSystem<Engine::TextureManager>("Textures");
		for (auto& p : m.m_partMaterials)
		{
			p.m_diffuseOpacity = gui.ColourEdit("Diffuse/Opacity", p.m_diffuseOpacity);
			p.m_specular = gui.ColourEdit("Specular Colour/Strength", p.m_specular);
			p.m_shininess.x = gui.DragFloat("Shininess", p.m_shininess.x, 0.01f, 0.0f);

			auto doTexture = [&](std::string name, Engine::TextureHandle& t)
			{
				auto theTexture = textures.GetTexture(t);
				auto path = textures.GetTexturePath(t);
				if (gui.Button(path.length() > 0 ? path.c_str() : name.c_str()))
				{
					std::string newFile = Engine::ShowFilePicker("Select Texture", "", "JPG (.jpg)\0*.jpg\0PNG (.png)\0*.png\0BMP (.bmp)\0*.bmp\0");
					if (newFile != "")
					{
						t = textures.LoadTexture(newFile.c_str());
					}
				}
				if (theTexture)
				{
					gui.Image(*theTexture, { 256,256 });
				}
			};
			doTexture("Diffuse Texture", p.m_diffuseTexture);
			doTexture("Normals Texture", p.m_normalsTexture);
			doTexture("Specular Texture", p.m_specularTexture);
		}
	};
	return fn;
}