#version 430

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
layout(std430, binding = 0) buffer OutputVertices
{
	uint m_count;
	uint m_dimensions[3];
	vec4 m_vertices[];		// pos, normal
};
layout(binding = 0, r32ui) uniform uimage3D OutputIndices;
uniform sampler3D InputVolume;

uniform vec4 WorldOffset;
uniform vec4 CellSize;
uniform float NormalSampleBias = 1;

vec3 SampleNormal(vec3 p, vec3 uvScale, float sampleDelta)
{
	float samples[6];
	p = (p - WorldOffset.xyz) / CellSize.xyz;
	samples[0] = texture(InputVolume, uvScale * (p + vec3(sampleDelta,0,0))).r;
	samples[1] = texture(InputVolume, uvScale * (p + vec3(-sampleDelta,0,0))).r;
	samples[2] = texture(InputVolume, uvScale * (p + vec3(0,sampleDelta,0))).r;
	samples[3] = texture(InputVolume, uvScale * (p + vec3(0,-sampleDelta,0))).r;
	samples[4] = texture(InputVolume, uvScale * (p + vec3(0,0,sampleDelta))).r;
	samples[5] = texture(InputVolume, uvScale * (p + vec3(0,0,-sampleDelta))).r;
	vec3 normal;
	normal.x = (samples[0] - samples[1]) / 2 / sampleDelta;
	normal.y = (samples[2] - samples[3]) / 2 / sampleDelta;
	normal.z = (samples[4] - samples[5]) / 2 / sampleDelta;
	normal = normalize(normal);
	return normal;
}

void main() 
{
	// get index in global work group i.e x,y position
	ivec3 p = ivec3(gl_GlobalInvocationID.xyz);
	
	ivec3 volSize = textureSize(InputVolume, 0);
	vec3 uvScale = vec3(1.0f,1.0f,1.0f) / vec3(volSize);

	// sample all corners of the cell
	float c[2][2][2];
	c[0][0][0] = texture(InputVolume, uvScale * (p + ivec3(0,0,0))).r;
	c[1][0][0] = texture(InputVolume, uvScale * (p + ivec3(1,0,0))).r;
	c[0][1][0] = texture(InputVolume, uvScale * (p + ivec3(0,1,0))).r;
	c[1][1][0] = texture(InputVolume, uvScale * (p + ivec3(1,1,0))).r;
	c[0][0][1] = texture(InputVolume, uvScale * (p + ivec3(0,0,1))).r;
	c[1][0][1] = texture(InputVolume, uvScale * (p + ivec3(1,0,1))).r;
	c[0][1][1] = texture(InputVolume, uvScale * (p + ivec3(0,1,1))).r;
	c[1][1][1] = texture(InputVolume, uvScale * (p + ivec3(1,1,1))).r;

	// go through edges of the cell, if any contain sign changes on vertices, ...
	// then we calculate a vertex on the zero point
	vec3 worldPos = WorldOffset.xyz + CellSize.xyz * vec3(p);
	vec3 intersections[16];
	int intersectionCount = 0;
	for (int crnX = 0; crnX < 2; ++crnX)
	{
		for (int crnY = 0; crnY < 2; ++crnY)
		{
			if ((c[crnX][crnY][0] > 0.0) != (c[crnX][crnY][1] > 0.0))
			{
				float zero = (0.0 - c[crnX][crnY][0]) / (c[crnX][crnY][1] - c[crnX][crnY][0]);
				intersections[intersectionCount++] = vec3(worldPos.x + crnX * CellSize.x, 
														  worldPos.y + crnY * CellSize.y, 
														  worldPos.z + zero * CellSize.z);
			}
		}
	}
	for (int crnX = 0; crnX < 2; ++crnX)
	{
		for (int crnZ = 0; crnZ < 2; ++crnZ)
		{
			if ((c[crnX][0][crnZ] > 0.0f) != (c[crnX][1][crnZ] > 0.0f))
			{
				float zero = (0.0f - c[crnX][0][crnZ]) / (c[crnX][1][crnZ] - c[crnX][0][crnZ]);
				intersections[intersectionCount++] = vec3( worldPos.x + crnX * CellSize.x, 
															worldPos.y + zero * CellSize.y, 
															worldPos.z + crnZ * CellSize.z);
			}
		}
	}
	for (int crnY = 0; crnY < 2; ++crnY)
	{
		for (int crnZ = 0; crnZ < 2; ++crnZ)
		{
			if ((c[0][crnY][crnZ] > 0.0f) != (c[1][crnY][crnZ] > 0.0f))
			{
				float zero = (0.0f - c[0][crnY][crnZ]) / (c[1][crnY][crnZ] - c[0][crnY][crnZ]);
				intersections[intersectionCount++] = vec3( worldPos.x + zero * CellSize.x, 
															worldPos.y + crnY * CellSize.y, 
															worldPos.z + crnZ * CellSize.z);
			}
		}
	}
	
	// Now we can get a vertex position by averaging all found intersection points
	// Do the same for normals too!
	if(intersectionCount > 0)
	{
		vec3 averageNormal = vec3(0,0,0);
		vec3 outPosition = vec3(0,0,0);
		for(int i=0;i<intersectionCount;++i)
		{
			outPosition = outPosition + intersections[i];		
			averageNormal = averageNormal + SampleNormal(intersections[i],uvScale,NormalSampleBias);
		}
	
		outPosition = outPosition / intersectionCount;
		averageNormal = normalize(averageNormal / intersectionCount);
		
		//vec3 outNormal = SampleNormal(outPosition,uvScale,NormalSampleBias);		
		//vec3 outNormal = averageNormal;
		vec3 outNormal = (averageNormal + SampleNormal(outPosition,uvScale,NormalSampleBias))/2.0;
		
		// Write the vertex to the buffer
		uint startIndex = atomicAdd(m_count,2);
		m_vertices[startIndex] = vec4(outPosition,1.0);
		m_vertices[startIndex+1] = vec4(outNormal,1.0);
		
		// Write the index to the lookup image
		imageStore(OutputIndices, p, ivec4(startIndex/2,0,0,0));
	}
	else
	{
		imageStore(OutputIndices, p, ivec4(0,0,0,0));
	}
}