#version 460
layout(location = 0) in vec2 vs_in_position;

// Per-instance data
layout(std430, binding = 0) buffer vs_instance_data
{
    mat4 instance_transforms[];
};

uniform mat4 ProjectionMat;
out vec2 out_uv;

void main()
{
	mat4 instanceTransform = instance_transforms[gl_BaseInstance + gl_InstanceID];
	vec4 pos = vec4(vs_in_position,0,1);
	vec4 v = ProjectionMat * instanceTransform * pos; 
	out_uv = vs_in_position;	// we always draw a (0,0)-(1,1) quad
    gl_Position = v;
}
