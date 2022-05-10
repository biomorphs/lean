#pragma once
#include "engine/system.h"
#include "entity/entity_handle.h"
#include "core/glm_headers.h"

namespace Engine
{
	class CharacterControllerSystem : public Engine::System
	{
	public:
		CharacterControllerSystem();
		virtual ~CharacterControllerSystem();
		virtual bool PreInit();
		virtual bool Initialise();
		virtual bool Tick(float timeDelta);
		virtual void Shutdown();
	private:
		void RegisterScripts();
		void RegisterComponents();
		bool m_enableDebug = false;
	};
}