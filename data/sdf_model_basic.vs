#version 430

layout(location = 0) in vec4 vs_in_positionMat;
layout(location = 1) in mat4 vs_in_instance_modelmat;

#pragma sde include "global_uniforms.h"

out vec3 vs_out_position;


void main()
{
	vec4 pos = vec4(vs_in_positionMat.xyz,1);
	vec4 worldSpacePos = vs_in_instance_modelmat * pos;
	vec4 viewSpacePos = ProjectionViewMatrix * worldSpacePos; 
	vs_out_position = worldSpacePos.xyz;
    gl_Position = viewSpacePos;
}
