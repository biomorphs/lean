#pragma once 
#include "engine/system.h"
#include "engine/debug_gui_menubar.h"
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

namespace Engine
{
	class DebugGuiSystem;
}

namespace Particles
{
	class EmitterDescriptor;
	class EmissionBehaviour;
	class EmitterEditor : public Engine::System
	{
	public:
		EmitterEditor();
		virtual ~EmitterEditor();

		void RegisterEmissionBehaviour(std::unique_ptr<EmissionBehaviour>&& b);

		int NewEmitter();	// returns ID
		bool SaveEmitter(int ID, std::string_view path);
		int LoadEmitter(std::string_view path);	// returns -1 on failure, ID otherwise

		void AddEmissionBehaviour(int emitterID, std::unique_ptr<EmissionBehaviour>&& b);
		void DeleteEmissionBehaviour(int emitterID, int behaviourIndex);

		virtual bool Tick(float timeDelta);
	private:
		void ShowCurrentEmitter(Engine::DebugGuiSystem& gui);
		void DrawTabButtons(Engine::DebugGuiSystem& gui);
		void DoOnClose();
		bool m_windowOpen = true;

		Engine::MenuBar m_menuBar;

		struct ActiveEmitter	// emitter being editted
		{
			int m_id;
			std::unique_ptr<Particles::EmitterDescriptor> m_emitter;
			std::string m_documentPath;
		};
		ActiveEmitter* FindEmitter(int id);
		int m_nextActiveEmitterId = 0;
		std::vector<ActiveEmitter> m_activeEmitters;
		std::unordered_map<std::string, std::unique_ptr<EmissionBehaviour>> m_emissionBehaviours;
		
		int m_currentEmitterID = -1;		// references m_activeEmitters
	};
}