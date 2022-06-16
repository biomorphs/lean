#pragma once

#include "entity/component.h"
#include "entity/component_handle.h"
#include "entity/entity_handle.h"
#include "core/glm_headers.h"
#include "engine/shader_manager.h"
#include "engine/model_manager.h"
#include "engine/components/component_material.h"

class Model
{
public:
	COMPONENT(Model);
	COMPONENT_INSPECTOR(Engine::DebugGuiSystem& gui);

	// set this to a entity with a MaterialComponent for per-entity materials
	void SetMaterialEntity(EntityHandle e) { m_material = e; }
	EntityHandle GetMaterialEntity() { return m_material.GetEntity(); }
	Material* GetMaterialComponent() { return m_material.GetComponent(); }

	void SetShader(Engine::ShaderHandle s) { m_shader = s; }
	Engine::ShaderHandle GetShader() const { return m_shader; }

	void SetModel(Engine::ModelHandle s) { m_model = s; }
	Engine::ModelHandle GetModel() const { return m_model; }

private:
	ComponentHandle<Material> m_material;
	Engine::ShaderHandle m_shader;
	Engine::ModelHandle m_model;
};