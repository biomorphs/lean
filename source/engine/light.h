#pragma once
#include "core/glm_headers.h"

namespace Engine
{
	class Light						// transient light data kept per frame in the renderer
	{
	public:
		glm::vec4 m_colour;			// w = ambient strength
		glm::vec4 m_position;		// w = type - 0 = dir, 1=point, 2=spot
		glm::vec3 m_direction;
		glm::mat4 m_lightspaceMatrix;	// for shadow calculations
		glm::vec2 m_spotlightAngles;	// inner, outer, cosines
		float m_maxDistance;			// for attenuation
		float m_attenuationCompress;	// controls attenuation fn
		float m_shadowBias;	
		Render::FrameBuffer* m_shadowMap = nullptr;	// if this is not null the light casts shadows
		bool m_updateShadowmap;		// do we need to redraw this frame
	};
}