#version 460

// Per-instance data
layout(std430, binding = 0) buffer vs_instance_data
{
    mat4 instance_transforms[];
};

layout(location = 0) in vec4 vs_in_positionMat;
layout(location = 1) in vec4 vs_in_normal;

#pragma sde include "global_uniforms.h"

out vec3 vs_out_position;
out vec3 vs_out_normal;

void main()
{
	mat4 instanceTransform = instance_transforms[gl_BaseInstance + gl_InstanceID];
	vec4 pos = vec4(vs_in_positionMat.xyz,1);
	vec4 worldSpacePos = instanceTransform * pos;
	vec4 viewSpacePos = ProjectionViewMatrix * worldSpacePos; 
	vs_out_normal = normalize(mat3(transpose(inverse(instanceTransform))) * vs_in_normal.xyz); 
	vs_out_position = worldSpacePos.xyz;
    gl_Position = viewSpacePos;
}
