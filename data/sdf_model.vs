#version 460

// Per-instance data
layout(std430, binding = 0) buffer vs_instance_data
{
    mat4 instance_transforms[];
};

layout(location = 0) in vec3 vs_in_position;
layout(location = 1) in vec3 vs_in_normal;
layout(location = 2) in float vs_in_ao;

#pragma sde include "global_uniforms.h"

out vec3 vs_out_normal;
out vec3 vs_out_position;
out float vs_out_ao;

void main()
{
	mat4 instanceTransform = instance_transforms[gl_BaseInstance + gl_InstanceID];
	vec4 pos = vec4(vs_in_position,1);
	vec4 worldSpacePos = instanceTransform * pos;
	vec4 viewSpacePos = ProjectionViewMatrix * worldSpacePos; 
	vs_out_normal = mat3(transpose(inverse(instanceTransform))) * vs_in_normal; 
	vs_out_position = worldSpacePos.xyz;
	vs_out_ao = vs_in_ao;
    gl_Position = viewSpacePos;
}
