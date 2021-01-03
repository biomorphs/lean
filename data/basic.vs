#version 430
#pragma sde include "shared.vs"

out vec4 out_colour;
out vec2 out_uv;

void main()
{
	vec4 pos = vec4(vs_in_position,1);
	vec4 v = ProjectionViewMatrix * vs_in_instance_modelmat * pos; 
    out_colour = vs_in_instance_colour;
	out_uv = vs_in_uv;
    gl_Position = v;
}
