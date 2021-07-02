#version 460

layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;
layout(std430, binding = 0) buffer AllRays
{
	float m_allRayData[];		// start[3], end[3]
};
layout(std430, binding = 1) buffer RayIndices
{
	uint m_rayIndices[];
};
layout(std430, binding = 2) buffer OutNormalTPoint
{
	vec4 m_results[];	// write hit normal, t for each ray index
};

uniform uint RayIndexOffset;
uniform uint RayCount;

#pragma sde include "SDF_SHADER_INCLUDE"

void main() 
{
	// get index in global work group
	ivec3 p = ivec3(gl_GlobalInvocationID.xyz);
	if(p.x < RayCount)
	{	
		vec3 rayStart = vec3(m_allRayData[RayIndexOffset],m_allRayData[RayIndexOffset+1],m_allRayData[RayIndexOffset+2]);
		vec3 rayEnd = vec3(m_allRayData[RayIndexOffset+3],m_allRayData[RayIndexOffset+4],m_allRayData[RayIndexOffset+5]);
		
		/// blah blah do raymarch against SDF 
		
		vec3 hitNormal = vec3(rayStart.x,rayEnd.x,0);
		float hitTPoint = 0.5;
		
		m_results[p.x + RayIndexOffset] = vec4(hitNormal, hitTPoint);
	}
}