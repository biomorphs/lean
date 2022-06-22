#version 460

#pragma sde include "per_instance_data.h"

layout(location = 0) in vec4 vs_in_position;
layout(location = 1) in vec4 vs_in_normal;

#pragma sde include "global_uniforms.h"

uniform mat4 ShadowLightSpaceMatrix;

out vec3 vs_out_position;
out vec3 vs_out_normal;

void main()
{
	mat4 instanceTransform = instance_data[gl_BaseInstance + gl_InstanceID].m_transform;
	vec4 worldPos = instanceTransform * vec4(vs_in_position.xyz,1);
	vs_out_position = worldPos.xyz;
	gl_Position = ShadowLightSpaceMatrix * worldPos; 
}
