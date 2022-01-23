#pragma once

#include "entity/component.h"
#include "entity/entity_handle.h"
#include "core/glm_headers.h"
#include "engine/shader_manager.h"
#include "engine/model_manager.h"

class Model
{
public:
	COMPONENT(Model);
	COMPONENT_INSPECTOR(Engine::DebugGuiSystem& gui);

	// set this to a entity with a MaterialComponent for per-entity materials
	void SetMaterialEntity(EntityHandle e) { m_materialEntity = e; }
	EntityHandle GetMaterialEntity() { return m_materialEntity; }

	void SetShader(Engine::ShaderHandle s) { m_shader = s; }
	Engine::ShaderHandle GetShader() const { return m_shader; }

	void SetModel(Engine::ModelHandle s) { m_model = s; }
	Engine::ModelHandle GetModel() const { return m_model; }

private:
	// how do we serialise this?!
	EntityHandle m_materialEntity;	
	Engine::ShaderHandle m_shader;
	Engine::ModelHandle m_model;
};