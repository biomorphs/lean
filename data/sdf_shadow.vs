#version 430

layout(location = 0) in vec3 vs_in_position;
layout(location = 1) in vec3 vs_in_normal;
layout(location = 2) in float vs_in_ao;
layout(location = 3) in mat4 vs_in_instance_modelmat;

#pragma sde include "global_uniforms.h"

uniform mat4 ShadowLightSpaceMatrix;
out vec3 vs_out_position;

void main()
{
	vec4 worldPos = vs_in_instance_modelmat * vec4(vs_in_position,1);
	vs_out_position = worldPos.xyz;
	gl_Position = ShadowLightSpaceMatrix * worldPos; 
}
