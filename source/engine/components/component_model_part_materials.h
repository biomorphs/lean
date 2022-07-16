#pragma once

#include "entity/component.h"
#include "engine/model.h"
#include "core/glm_headers.h"

class ModelPartMaterialOverride : public Engine::Model::MeshPart::DrawData
{
public:
	SERIALISED_CLASS();
};

class ModelPartMaterials
{
public:
	COMPONENT(ModelPartMaterials);
	COMPONENT_INSPECTOR(Engine::DebugGuiSystem& gui);

	std::vector<ModelPartMaterialOverride>& Materials() { return m_partMaterials; }
	const std::vector<ModelPartMaterialOverride>& Materials() const { return m_partMaterials; }

private:
	std::vector<ModelPartMaterialOverride> m_partMaterials;
};