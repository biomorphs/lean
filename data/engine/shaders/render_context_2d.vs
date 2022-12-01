#version 460
#extension GL_ARB_gpu_shader_int64 : enable

layout(location = 0) in vec2 vs_in_position;
layout(location = 1) in vec2 vs_in_uv;
layout(location = 2) in vec4 vs_in_colour;

struct PerTriangleData
{
	uint64_t m_texture;
};
layout(std430, binding = 0) buffer TriangleData
{
    PerTriangleData triangle_data[];
};

uniform mat4 ProjectionMat;

out vec2 vs_out_position;
out vec2 vs_out_uv;
out vec4 vs_out_colour;
flat out uint vs_out_triangleID;

void main()
{
    vs_out_position = vs_in_position;
	vs_out_uv = vs_in_uv;
	vs_out_colour = vs_in_colour;
    gl_Position = ProjectionMat * vec4(vs_in_position,0.0,1.0);
	vs_out_triangleID = gl_VertexID / 3;
}
