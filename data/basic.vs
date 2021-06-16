#version 460
#pragma sde include "shared.vs"

out vec2 out_uv;

void main()
{
	mat4 instanceTransform = instance_transforms[gl_BaseInstance + gl_InstanceID];
	vec4 pos = vec4(vs_in_position,1);
	vec4 v = ProjectionViewMatrix * instanceTransform * pos; 
	out_uv = vs_in_uv;
    gl_Position = v;
}
