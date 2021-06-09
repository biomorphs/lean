#version 430

uniform sampler3D InputVolume;
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
layout(rgba32f, binding = 0) uniform image3D InputVertices;
layout(std430, binding = 1) buffer OutputVertices
{
	uint m_countAndPadding[4];
	vec4 m_position[];
};


void OutputQuad(vec3 v0,vec3 v1,vec3 v2,vec3 v3)
{
	uint startIndex = atomicAdd(m_countAndPadding[0],6);
	
	m_position[startIndex] =   vec4(v0, 1.0);
	m_position[startIndex+1] = vec4(v1, 1.0);
	m_position[startIndex+2] = vec4(v2, 1.0);
							   
	m_position[startIndex+3] = vec4(v2, 1.0);
	m_position[startIndex+4] = vec4(v3, 1.0);
	m_position[startIndex+5] = vec4(v0, 1.0);
}

void main() 
{
	// get index in global work group i.e x,y position
	ivec3 p = ivec3(gl_GlobalInvocationID.xyz);
	
	ivec3 volSize = textureSize(InputVolume, 0);
	vec3 cellSize = vec3(1.0f,1.0f,1.0f) / vec3(volSize);
	
	// for any edge that has a sign change, find vertices from the surrounding cells and connect them as a quad
	if (p.x > 0 && p.y > 0)
	{
		float s0 = texture(InputVolume, cellSize * (p + ivec3(0,0,0))).r;
		float s1 = texture(InputVolume, cellSize * (p + ivec3(0,0,1))).r;
		if ((s0 > 0.0) != (s1 > 0.0))
		{			
			vec3 v0 = imageLoad(InputVertices, p + ivec3(-1,-1,0)).xyz;
			vec3 v1 = imageLoad(InputVertices, p + ivec3(0,-1,0)).xyz;
			vec3 v2 = imageLoad(InputVertices, p + ivec3(0,0,0)).xyz;
			vec3 v3 = imageLoad(InputVertices, p + ivec3(-1,0,0)).xyz;
			
			if (s1 > 0.0f)
			{
				OutputQuad(v0,v1,v2,v3);
			}
			else
			{
				OutputQuad(v3,v2,v1,v0);
			}
		}
	}
	if (p.x > 0 && p.z > 0)
	{
		float s0 = texture(InputVolume, cellSize * (p + ivec3(0,0,0))).r;
		float s1 = texture(InputVolume, cellSize * (p + ivec3(0,1,0))).r;
		if ((s0 > 0.0) != (s1 > 0.0))
		{
			vec3 v0 = imageLoad(InputVertices, p + ivec3(-1,0,-1)).xyz;
			vec3 v1 = imageLoad(InputVertices, p + ivec3(0,0,-1)).xyz;
			vec3 v2 = imageLoad(InputVertices, p + ivec3(0,0,0)).xyz;
			vec3 v3 = imageLoad(InputVertices, p + ivec3(-1,0,0)).xyz;
			if (s0 > 0.0f)
			{
				OutputQuad(v0,v1,v2,v3);
			}
			else
			{
				OutputQuad(v3,v2,v1,v0);
			}
		}
	}
	if (p.y > 0 && p.z > 0)
	{
		float s0 = texture(InputVolume, cellSize * (p + ivec3(0,0,0))).r;
		float s1 = texture(InputVolume, cellSize * (p + ivec3(1,0,0))).r;
		if ((s0 > 0.0) != (s1 > 0.0))
		{
			vec3 v0 = imageLoad(InputVertices, p + ivec3(0,-1,-1)).xyz;
			vec3 v1 = imageLoad(InputVertices, p + ivec3(0,0,-1)).xyz;
			vec3 v2 = imageLoad(InputVertices, p + ivec3(0,0,0)).xyz;
			vec3 v3 = imageLoad(InputVertices, p + ivec3(0,-1,0)).xyz;
			if (s1 > 0.0f)
			{
				OutputQuad(v0,v1,v2,v3);
			}
			else
			{
				OutputQuad(v3,v2,v1,v0);
			}
		}
	}
}