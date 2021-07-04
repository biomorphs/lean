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
struct Output
{
	vec4 m_normalTPos;
	uint m_entityID;
	uint m_padding[3];
};
layout(std430, binding = 2) buffer OutNormalTPoint
{
	Output m_results[];
};

uniform uint RayIndexOffset;
uniform uint RayCount;
uniform uint EntityID;

#pragma sde include "SDF_SHADER_INCLUDE"

vec3 SampleNormal(vec3 p, float sampleDelta)
{
	float samples[6];
	samples[0] = SDF(p + vec3(sampleDelta,0,0));
	samples[1] = SDF(p + vec3(-sampleDelta,0,0));
	samples[2] = SDF(p + vec3(0,sampleDelta,0));
	samples[3] = SDF(p + vec3(0,-sampleDelta,0));
	samples[4] = SDF(p + vec3(0,0,sampleDelta));
	samples[5] = SDF(p + vec3(0,0,-sampleDelta));
	vec3 normal;
	normal.x = (samples[0] - samples[1]);
	normal.y = (samples[2] - samples[3]);
	normal.z = (samples[4] - samples[5]);
	normal = normalize(normal);
	return normal;
}

void main() 
{
	// get index in global work group
	ivec3 p = ivec3(gl_GlobalInvocationID.xyz);
	if(p.x < RayCount)
	{	
		const uint rayDataIndex = m_rayIndices[p.x + RayIndexOffset] * 6;
		const vec3 rayStart = vec3(m_allRayData[rayDataIndex],m_allRayData[rayDataIndex+1],m_allRayData[rayDataIndex+2]);
		const vec3 rayEnd = vec3(m_allRayData[rayDataIndex+3],m_allRayData[rayDataIndex+4],m_allRayData[rayDataIndex+5]);
		
		const float rayLength = length(rayEnd-rayStart);
		const vec3 direction = (rayEnd-rayStart)/rayLength;	// blindly trust we never receive a zero length ray
		
		float d = SDF(rayStart);
		const bool rayStartDGrtZero = d > 0.0;
		vec3 v = rayStart;
		while(true)
		{
			const float step = clamp(abs(d), 0.001, 0.5);	// parameterise? accuracy vs speed?
			v = v + direction * step;
			const float t = length(v-rayStart)/rayLength;
			if(t>1.0)
				break;
			d = SDF(v);
			if(rayStartDGrtZero != (d > 0.0) || (d < 0.00001 && d > -0.00001))
			{
				vec3 hitNormal = SampleNormal(v,0.0001);	// parameterise?
				m_results[p.x + RayIndexOffset].m_normalTPos = vec4(hitNormal, t);
				m_results[p.x + RayIndexOffset].m_entityID = EntityID;
				return;
			}
		}
		m_results[p.x + RayIndexOffset].m_normalTPos = vec4(-1);
	}
}