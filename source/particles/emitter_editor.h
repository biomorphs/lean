#pragma once 
#include "engine/system.h"
#include "engine/debug_gui_menubar.h"
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <type_traits>

namespace Engine
{
	class DebugGuiSystem;
}

namespace Particles
{
	class EmitterDescriptor;
	class EmissionBehaviour;
	class GeneratorBehaviour;
	class UpdateBehaviour;
	class RenderBehaviour;
	class EmitterEditor : public Engine::System
	{
	public:
		EmitterEditor();
		virtual ~EmitterEditor();

		template<class Behaviour>
		void RegisterBehaviour(std::unique_ptr<Behaviour>&& b);

		int NewEmitter();	// returns ID
		bool SaveEmitter(int ID, std::string_view path);
		int LoadEmitter(std::string_view path);	// returns -1 on failure, ID otherwise

		enum BehaviourType {
			Emitter, Generator, Updater, Renderer
		};
		template<class Behaviour>
		void AddBehaviourToEmitter(int emitterID, std::unique_ptr<Behaviour>&& b);
		void DeleteBehaviour(BehaviourType type, int emitterID, int behaviourIndex);

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
		bool m_refreshPlayingEmitter = false;
		uint64_t m_currentPlayingEmitter = -1;
		ActiveEmitter* FindEmitter(int id);
		int m_nextActiveEmitterId = 0;
		std::vector<ActiveEmitter> m_activeEmitters;
		std::unordered_map<std::string, std::unique_ptr<EmissionBehaviour>> m_emissionBehaviours;
		std::unordered_map<std::string, std::unique_ptr<GeneratorBehaviour>> m_generatorBehaviours;
		std::unordered_map<std::string, std::unique_ptr<UpdateBehaviour>> m_updateBehaviours;
		std::unordered_map<std::string, std::unique_ptr<RenderBehaviour>> m_renderBehaviours;
		
		int m_currentEmitterID = -1;		// references m_activeEmitters
	};

	template<class Behaviour>
	void EmitterEditor::AddBehaviourToEmitter(int emitterID, std::unique_ptr<Behaviour>&& b)
	{
		auto em = FindEmitter(emitterID);
		if (em)
		{
			if constexpr (std::is_base_of<EmissionBehaviour, Behaviour>::value)
			{
				em->m_emitter->GetEmissionBehaviours().emplace_back(std::move(b));
			}
			else if constexpr (std::is_base_of<GeneratorBehaviour, Behaviour>::value)
			{
				em->m_emitter->GetGenerators().emplace_back(std::move(b));
			}
			else if constexpr (std::is_base_of<UpdateBehaviour, Behaviour>::value)
			{
				em->m_emitter->GetUpdaters().emplace_back(std::move(b));
			}
			else if constexpr (std::is_base_of<RenderBehaviour, Behaviour>::value)
			{
				em->m_emitter->GetRenderers().emplace_back(std::move(b));
			}
			else
			{
				static_assert(false);
			}
		}
	}

	template<class Behaviour>
	void EmitterEditor::RegisterBehaviour(std::unique_ptr<Behaviour>&& b)
	{
		if constexpr (std::is_base_of<EmissionBehaviour, Behaviour>::value)
		{
			m_emissionBehaviours[std::string(b->GetName())] = std::move(b);
		}
		else if constexpr (std::is_base_of<GeneratorBehaviour, Behaviour>::value)
		{
			m_generatorBehaviours[std::string(b->GetName())] = std::move(b);
		}
		else if constexpr (std::is_base_of<UpdateBehaviour, Behaviour>::value)
		{
			m_updateBehaviours[std::string(b->GetName())] = std::move(b);
		}
		else if constexpr (std::is_base_of<RenderBehaviour, Behaviour>::value)
		{
			m_renderBehaviours[std::string(b->GetName())] = std::move(b);
		}
		else
		{
			static_assert(false);
		}
	}
}