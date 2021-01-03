#version 430
#pragma sde include "global_uniforms.h"

layout(location = 0) in vec4 vs_in_pos_modelSpace;
layout(location = 1) in vec4 vs_in_colour;
layout(location = 2) in mat4 vs_in_instance_modelmat;

out vec4 out_colour;

void main()
{
	vec4 pos = vec4(vs_in_pos_modelSpace.xyz,1);
	vec4 v = ProjectionViewMatrix * vs_in_instance_modelmat * pos; 
    out_colour = vs_in_colour;
    gl_Position = v;
}
