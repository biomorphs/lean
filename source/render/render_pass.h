/*
SDLEngine
Matt Hoyle
*/
#pragma once

namespace Render
{
	class RenderPass
	{
	public:
		RenderPass() = default;
		RenderPass(RenderPass&&) = default;
		virtual ~RenderPass() = default;
		virtual void Reset() { }
		virtual void RenderAll(class Device&) { }

	private:
		RenderPass(const RenderPass&);
	};
}