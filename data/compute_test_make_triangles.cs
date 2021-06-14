#version 430

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
layout(binding = 0, std430) buffer OutputIndices
{
	uint m_count;
	uint m_dimensions[3];
	uint m_indices[];
};
layout(binding = 0, r32ui) uniform uimage3D InputIndices;	// index per cell in 3d texture
uniform sampler3D InputVolume;

void OutputQuad(uint v0,uint v1,uint v2,uint v3)
{
	uint startIndex = atomicAdd(m_count,6);
	
	m_indices[startIndex] =   v0;
	m_indices[startIndex+1] = v1;
	m_indices[startIndex+2] = v2;
							   
	m_indices[startIndex+3] = v2;
	m_indices[startIndex+4] = v3;
	m_indices[startIndex+5] = v0;
}

void main() 
{
	// get index in global work group i.e x,y position
	ivec3 p = ivec3(gl_GlobalInvocationID.xyz);
	
	// we need to skip 2 outer 'layers' of the samples to ensure no sampling artifacts due to filtering
	const uint skipLayers = 2;
	ivec3 maxSample = ivec3(m_dimensions[0]-skipLayers,m_dimensions[1]-skipLayers,m_dimensions[2]-skipLayers);
	if(p.x < skipLayers || p.y < skipLayers || p.z < skipLayers)
	{
		return;
	}
	
	ivec3 volSize = textureSize(InputVolume, 0);
	vec3 cellSize = vec3(1.0f,1.0f,1.0f) / vec3(volSize);
	
	// for any edge that has a sign change, find vertices from the surrounding cells and connect them as a quad
	if (p.x > 0 && p.y > 0)
	{
		float s0 = texture(InputVolume, cellSize * (p + ivec3(0,0,0))).r;
		float s1 = texture(InputVolume, cellSize * (p + ivec3(0,0,1))).r;
		if ((s0 > 0.0) != (s1 > 0.0))
		{			
			uint i0 = imageLoad(InputIndices, p + ivec3(-1,-1,0)).x;
			uint i1 = imageLoad(InputIndices, p + ivec3(0,-1,0)).x;
			uint i2 = imageLoad(InputIndices, p + ivec3(0,0,0)).x;
			uint i3 = imageLoad(InputIndices, p + ivec3(-1,0,0)).x;
			if (s1 > 0.0f)
			{
				OutputQuad(i0,i1,i2,i3);
			}
			else
			{
				OutputQuad(i3,i2,i1,i0);
			}
		}
	}
	if (p.x > 0 && p.z > 0)
	{
		float s0 = texture(InputVolume, cellSize * (p + ivec3(0,0,0))).r;
		float s1 = texture(InputVolume, cellSize * (p + ivec3(0,1,0))).r;
		if ((s0 > 0.0) != (s1 > 0.0))
		{
			uint i0 = imageLoad(InputIndices, p + ivec3(-1,0,-1)).x;
			uint i1 = imageLoad(InputIndices, p + ivec3(0,0,-1)).x; 
			uint i2 = imageLoad(InputIndices, p + ivec3(0,0,0)).x;  
			uint i3 = imageLoad(InputIndices, p + ivec3(-1,0,0)).x; 
			if (s0 > 0.0f)
			{
				OutputQuad(i0,i1,i2,i3);
			}
			else
			{
				OutputQuad(i3,i2,i1,i0);
			}
		}
	}
	if (p.y > 0 && p.z > 0)
	{
		float s0 = texture(InputVolume, cellSize * (p + ivec3(0,0,0))).r;
		float s1 = texture(InputVolume, cellSize * (p + ivec3(1,0,0))).r;
		if ((s0 > 0.0) != (s1 > 0.0))
		{
			uint i0 = imageLoad(InputIndices, p + ivec3(0,-1,-1)).x;
			uint i1 = imageLoad(InputIndices, p + ivec3(0,0,-1)).x; 
			uint i2 = imageLoad(InputIndices, p + ivec3(0,0,0)).x;  
			uint i3 = imageLoad(InputIndices, p + ivec3(0,-1,0)).x; 
			if (s1 > 0.0f)
			{
				OutputQuad(i0,i1,i2,i3);
			}
			else
			{
				OutputQuad(i3,i2,i1,i0);
			}
		}
	}
}