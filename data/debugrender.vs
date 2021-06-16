#version 460
#pragma sde include "global_uniforms.h"

// Per-instance data
layout(std430, binding = 0) buffer vs_instance_data
{
    mat4 instance_transforms[];
};

layout(location = 0) in vec4 vs_in_pos_modelSpace;
layout(location = 1) in vec4 vs_in_colour;

out vec4 out_colour;

void main()
{
	mat4 instanceTransform = instance_transforms[gl_BaseInstance + gl_InstanceID];
	vec4 pos = vec4(vs_in_pos_modelSpace.xyz,1);
	vec4 v = ProjectionViewMatrix * instanceTransform * pos; 
    out_colour = vs_in_colour;
    gl_Position = v;
}
