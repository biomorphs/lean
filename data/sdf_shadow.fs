#version 430 
#pragma sde include "shared.fs"

in vec3 vs_out_position;

uniform int ShadowLightIndex;

void main()
{
	if(AllLights[ShadowLightIndex].Position.w == 0.0 || AllLights[ShadowLightIndex].Position.w == 2.0)
	{
		gl_FragDepth = gl_FragCoord.z;
	}
	else
	{
		// get distance between fragment and light source
		float lightDistance = length(vs_out_position.xyz - AllLights[ShadowLightIndex].Position.xyz);

		// map to [0;1] range by dividing by far_plane
		lightDistance = lightDistance / AllLights[ShadowLightIndex].DistanceAttenuation.x;
    
		// write this as modified depth
		gl_FragDepth = lightDistance;
	}
}