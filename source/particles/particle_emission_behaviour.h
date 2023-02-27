#pragma once

#include "engine/serialisation.h"

namespace Particles
{
	class EditorValueInspector;
	class ParticleContainer;

	// This is responsible for determining if a playing emitter should be stopped
	class EmitterLifetimeBehaviour
	{
	public:
		virtual SERIALISED_CLASS() {}
		virtual bool ShouldStop(double emitterAge, float deltaTime, ParticleContainer& container) = 0;
		virtual std::unique_ptr<EmitterLifetimeBehaviour> MakeNew() = 0;
		virtual std::string_view GetName() = 0;
		virtual void Inspect(EditorValueInspector&) = 0;
	};

	// This is responsible for determining WHEN particles spawn
	class EmissionBehaviour
	{
	public:
		virtual SERIALISED_CLASS() {}
		virtual int Emit(double emitterAge, float deltaTime) = 0;	// returns # particles to generate
		virtual std::unique_ptr<EmissionBehaviour> MakeNew() = 0;
		virtual std::string_view GetName() = 0;
		virtual void Inspect(EditorValueInspector&) = 0;
	};

	// This is responsible for initialising particles when they are spawned
	class GeneratorBehaviour
	{
	public:
		virtual SERIALISED_CLASS() {}
		virtual void Generate(glm::vec3 emitterPos, glm::quat orientation, double emitterAge, float deltaTime, ParticleContainer& container, uint32_t startIndex, uint32_t endIndex) = 0;
		virtual std::unique_ptr<GeneratorBehaviour> MakeNew() = 0;
		virtual std::string_view GetName() = 0;
		virtual void Inspect(EditorValueInspector&) = 0;
	};

	// This is responsible for updating particles each frame
	class UpdateBehaviour
	{
	public:
		virtual void Update(glm::vec3 emitterPos, glm::quat orientation, double emitterAge, float deltaTime, ParticleContainer& container) = 0;
		virtual SERIALISED_CLASS() {}
		virtual std::unique_ptr<UpdateBehaviour> MakeNew() = 0;
		virtual std::string_view GetName() = 0;
		virtual void Inspect(EditorValueInspector&) = 0;
	};

	// Finally this determines how they are drawn
	class RenderBehaviour
	{
	public:
		virtual SERIALISED_CLASS() {}
		virtual void Draw(glm::vec3 emitterPos, glm::quat orientation, double emitterAge, float deltaTime, ParticleContainer& container) = 0;
		virtual std::unique_ptr<RenderBehaviour> MakeNew() = 0;
		virtual std::string_view GetName() = 0;
		virtual void Inspect(EditorValueInspector&) = 0;
	};
}