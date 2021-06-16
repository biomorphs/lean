#version 460
#pragma sde include "shared.vs"

uniform vec4 MeshUVOffsetScale = vec4(0.0,0.0,1.0,1.0);	// xy=offset, zw=scale

out vec3 vs_out_normal;
out vec2 vs_out_uv;
out vec3 vs_out_position;
out mat3 vs_out_tbnMatrix;

void main()
{
	vec4 pos = vec4(vs_in_position,1);
	mat4 instanceTransform = instance_transforms[gl_BaseInstance + gl_InstanceID];
	vec4 worldSpacePos = instanceTransform * pos;
	vec4 viewSpacePos = ProjectionViewMatrix * worldSpacePos; 
	vs_out_normal = normalize(mat3(transpose(inverse(instanceTransform))) * vs_in_normal); 
	vs_out_uv =  MeshUVOffsetScale.xy + vs_in_uv * MeshUVOffsetScale.zw;
	vs_out_position = worldSpacePos.xyz;
	vs_out_tbnMatrix = CalculateTBN(instanceTransform, vs_in_tangent, vs_in_normal);
    gl_Position = viewSpacePos;
}
