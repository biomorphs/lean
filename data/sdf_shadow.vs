#version 460

#pragma sde include "per_instance_data.h"

layout(location = 0) in vec3 vs_in_position;
layout(location = 1) in vec3 vs_in_normal;
layout(location = 2) in float vs_in_ao;

#pragma sde include "global_uniforms.h"

uniform mat4 ShadowLightSpaceMatrix;
out vec3 vs_out_position;

void main()
{
	mat4 instanceTransform = instance_data[gl_BaseInstance + gl_InstanceID].m_transform;
	vec4 worldPos = instanceTransform * vec4(vs_in_position,1);
	vs_out_position = worldPos.xyz;
	gl_Position = ShadowLightSpaceMatrix * worldPos; 
}
