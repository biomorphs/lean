#version 460
#pragma sde include "shared.vs"

uniform mat4 ShadowLightSpaceMatrix;
out vec3 vs_out_position;
out vec2 vs_out_uv;
flat out uint vs_out_instanceID;

void main()
{
	mat4 instanceTransform = instance_data[gl_BaseInstance + gl_InstanceID].m_transform;
	vec4 worldPos = instanceTransform * vec4(vs_in_position,1);
	vs_out_position = worldPos.xyz;
	vs_out_uv = vs_in_uv;
	vs_out_instanceID = gl_BaseInstance + gl_InstanceID;
	gl_Position = ShadowLightSpaceMatrix * worldPos; 
}
