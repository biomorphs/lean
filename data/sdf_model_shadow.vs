#version 430

layout(location = 0) in vec4 vs_in_position;
layout(location = 1) in vec4 vs_in_normal;
layout(location = 2) in mat4 vs_in_instance_modelmat;

#pragma sde include "global_uniforms.h"

uniform mat4 ShadowLightSpaceMatrix;

out vec3 vs_out_position;
out vec3 vs_out_normal;

void main()
{
	vec4 worldPos = vs_in_instance_modelmat * vec4(vs_in_position.xyz,1);
	vs_out_position = worldPos.xyz;
	gl_Position = ShadowLightSpaceMatrix * worldPos; 
}
