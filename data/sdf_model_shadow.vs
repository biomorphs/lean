#version 460

// Per-instance data
layout(std430, binding = 0) buffer vs_instance_data
{
    mat4 instance_transforms[];
};

layout(location = 0) in vec4 vs_in_position;
layout(location = 1) in vec4 vs_in_normal;

#pragma sde include "global_uniforms.h"

uniform mat4 ShadowLightSpaceMatrix;

out vec3 vs_out_position;
out vec3 vs_out_normal;

void main()
{
	mat4 instanceTransform = instance_transforms[gl_BaseInstance + gl_InstanceID];
	vec4 worldPos = instanceTransform * vec4(vs_in_position.xyz,1);
	vs_out_position = worldPos.xyz;
	gl_Position = ShadowLightSpaceMatrix * worldPos; 
}
