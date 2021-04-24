#pragma once

#include "entity/component.h"
#include "core/glm_headers.h"
#include "engine/shader_manager.h"
#include "engine/model_manager.h"

class Model
{
public:
	COMPONENT(Model);

	void SetShader(Engine::ShaderHandle s) { m_shader = s; }
	Engine::ShaderHandle GetShader() const { return m_shader; }

	void SetModel(Engine::ModelHandle s) { m_model = s; }
	Engine::ModelHandle GetModel() const { return m_model; }

private:
	// how do we serialise this?!
	Engine::ShaderHandle m_shader;
	Engine::ModelHandle m_model;
};