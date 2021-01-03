#version 430
#pragma sde include "shared.vs"

out vec3 vs_out_position;
out vec2 vs_out_uv;

void main()
{
	vec4 worldPos = vs_in_instance_modelmat * vec4(vs_in_position,1);
	vs_out_position = worldPos.xyz;
	vs_out_uv = vs_in_uv;
	gl_Position = ShadowLightSpaceMatrix * worldPos; 
}
