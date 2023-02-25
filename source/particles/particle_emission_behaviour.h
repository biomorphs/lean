#pragma once

#include "engine/serialisation.h"

namespace Particles
{
	class EditorValueInspector;
	class EmissionBehaviour
	{
	public:
		virtual SERIALISED_CLASS() {}
		virtual std::unique_ptr<EmissionBehaviour> MakeNew() = 0;
		virtual std::string_view GetName() = 0;
		virtual void Inspect(EditorValueInspector&) = 0;
	};
}