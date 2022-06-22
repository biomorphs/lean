#extension GL_ARB_gpu_shader_int64 : enable

struct PerInstanceData
{
	mat4 m_transform;
	
	vec4 m_diffuseOpacity;
	vec4 m_specular;	//r,g,b,strength
	vec4 m_shininess;
	uint64_t m_diffuseTexture;
	uint64_t m_normalsTexture;
	uint64_t m_specularTexture;
};

layout(std430, binding = 0) buffer InstanceDataBuffer
{
    PerInstanceData instance_data[];
};