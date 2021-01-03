#pragma once

namespace sol
{
	class state;
}

namespace Engine
{
	class DebugGuiSystem;
}

namespace DebugGuiScriptBinding
{
	void Go(Engine::DebugGuiSystem* system, sol::state& context);
}