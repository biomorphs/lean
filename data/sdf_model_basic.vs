#version 460

#pragma sde include "per_instance_data.h"

layout(location = 0) in vec4 vs_in_positionMat;
layout(location = 1) in vec4 vs_in_normal;

#pragma sde include "global_uniforms.h"

out vec3 vs_out_position;
out vec3 vs_out_normal;

void main()
{
	mat4 instanceTransform = instance_data[gl_BaseInstance + gl_InstanceID].m_transform;
	vec4 pos = vec4(vs_in_positionMat.xyz,1);
	vec4 worldSpacePos = instanceTransform * pos;
	vec4 viewSpacePos = ProjectionViewMatrix * worldSpacePos; 
	vs_out_normal = normalize(mat3(transpose(inverse(instanceTransform))) * vs_in_normal.xyz); 
	vs_out_position = worldSpacePos.xyz;
    gl_Position = viewSpacePos;
}
