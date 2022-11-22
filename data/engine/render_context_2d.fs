#version 460 
#extension GL_ARB_gpu_shader_int64 : enable

in vec2 vs_out_position;
in vec2 vs_out_uv;
in vec4 vs_out_colour;
flat in uint vs_out_triangleID;

struct PerTriangleData
{
	uint64_t m_texture;
};
layout(std430, binding = 0) buffer TriangleData
{
    PerTriangleData triangle_data[];
};

out vec4 fs_out_colour;
 
void main()
{
	fs_out_colour = texture(sampler2D(triangle_data[vs_out_triangleID].m_texture), vs_out_uv) * vs_out_colour;
}