#version 430
layout(location = 0) in vec2 vs_in_position;
layout(location = 1) in mat4 vs_in_instance_modelmat;
layout(location = 5) in vec4 vs_in_instance_colour;

uniform mat4 ProjectionMat;
out vec4 out_colour;
out vec2 out_uv;

void main()
{
	vec4 pos = vec4(vs_in_position,0,1);
	vec4 v = ProjectionMat * vs_in_instance_modelmat * pos; 
    out_colour = vs_in_instance_colour;
	out_uv = vs_in_position;	// we always draw a (0,0)-(1,1) quad
    gl_Position = v;
}
