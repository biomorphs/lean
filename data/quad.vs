#version 460
layout(location = 0) in vec2 vs_in_position;

#pragma sde include "per_instance_data.h"

uniform mat4 ProjectionMat;
out vec2 out_uv;

void main()
{
	mat4 instanceTransform = instance_data[gl_BaseInstance + gl_InstanceID].m_transform;
	vec4 pos = vec4(vs_in_position,0,1);
	vec4 v = ProjectionMat * instanceTransform * pos; 
	out_uv = vs_in_position;	// we always draw a (0,0)-(1,1) quad
    gl_Position = v;
}
