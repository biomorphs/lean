// smol renderer shared vertex shader data

// Shared mesh layout
layout(location = 0) in vec3 vs_in_position;
layout(location = 1) in vec3 vs_in_normal;
layout(location = 2) in vec3 vs_in_tangent;
layout(location = 3) in vec2 vs_in_uv;
layout(location = 4) in mat4 vs_in_instance_modelmat;
layout(location = 8) in vec4 vs_in_instance_colour;

#pragma sde include "global_uniforms.h"

mat3 CalculateTBN(mat4 modelMat, vec3 tangent, vec3 normal)
{
	// Gram-shmidt from https://learnopengl.com
	vec3 T = normalize(vec3(modelMat * vec4(tangent, 0.0)));
	vec3 N = normalize(vec3(modelMat * vec4(normal, 0.0)));

	// re-orthogonalize T with respect to N
	T = normalize(T - dot(T, N) * N);
	// then retrieve perpendicular vector B with the cross product of T and N
	vec3 B = cross(N, T);

	return mat3(T, B, N); 
}