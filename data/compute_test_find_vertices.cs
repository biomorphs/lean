#version 430

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
layout(rgba32f, binding = 0) uniform image3D outVertices;

uniform sampler3D InputVolume;

void main() 
{
	// get index in global work group i.e x,y position
	ivec3 p = ivec3(gl_GlobalInvocationID.xyz);
	
	ivec3 volSize = textureSize(InputVolume, 0);
	vec3 cellSize = vec3(1.0f,1.0f,1.0f) / vec3(volSize);

	// sample all corners of the cell
	float c[2][2][2];
	c[0][0][0] = texture(InputVolume, cellSize * (p + ivec3(0,0,0))).r;
	c[1][0][0] = texture(InputVolume, cellSize * (p + ivec3(1,0,0))).r;
	c[0][1][0] = texture(InputVolume, cellSize * (p + ivec3(0,1,0))).r;
	c[1][1][0] = texture(InputVolume, cellSize * (p + ivec3(1,1,0))).r;
	c[0][0][1] = texture(InputVolume, cellSize * (p + ivec3(0,0,1))).r;
	c[1][0][1] = texture(InputVolume, cellSize * (p + ivec3(1,0,1))).r;
	c[0][1][1] = texture(InputVolume, cellSize * (p + ivec3(0,1,1))).r;
	c[1][1][1] = texture(InputVolume, cellSize * (p + ivec3(1,1,1))).r;

	// go through edges of the cell, if any contain sign changes on vertices, ...
	// then we calculate a vertex on the zero point
	vec3 intersections[16];
	int intersectionCount = 0;
	for (int crnX = 0; crnX < 2; ++crnX)
	{
		for (int crnY = 0; crnY < 2; ++crnY)
		{
			if ((c[crnX][crnY][0] > 0.0) != (c[crnX][crnY][1] > 0.0))
			{
				float zero = (0.0 - c[crnX][crnY][0]) / (c[crnX][crnY][1] - c[crnX][crnY][0]);
				intersections[intersectionCount++] = vec3(p.x + crnX, p.y + crnY, p.z + zero);
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
				intersections[intersectionCount++] = vec3( p.x + crnX, p.y + zero, p.z + crnZ );
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
				intersections[intersectionCount++] = vec3( p.x + zero, p.y + crnY, p.z + crnZ );
			}
		}
	}
	
	// Now we can get a vertex position by averaging all found intersection points
	vec3 outPosition = vec3(0,0,0);
	for(int i=0;i<intersectionCount;++i)
	{
		outPosition = outPosition + intersections[i];
	}
	if(intersectionCount > 0)
	{
		outPosition = outPosition / intersectionCount;
	}
	
	// write the position for this cell
	imageStore(outVertices, p, vec4(outPosition,1));
}