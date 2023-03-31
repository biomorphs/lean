#version 460 
#extension GL_ARB_bindless_texture : enable
#pragma sde include "shared.fs"

in vec3 vs_out_normal;
in vec2 vs_out_uv;
in vec3 vs_out_position;
in mat3 vs_out_tbnMatrix;
flat in uint vs_out_instanceID;

layout (location = 0) out vec4 fs_out_position;
layout (location = 1) out vec4 fs_out_normal;
layout (location = 2) out vec4 fs_out_albedo;
layout (location = 3) out vec4 fs_out_specular;

void main()
{
	vec4 diffuseOpacity = instance_data[vs_out_instanceID].m_diffuseOpacity;
	vec4 diffuseTex = srgbToLinear(texture(sampler2D(instance_data[vs_out_instanceID].m_diffuseTexture), vs_out_uv));	
	if(diffuseTex.a < 0.5)
	{
		discard;
		return;
	}
	vec4 meshSpecular = instance_data[vs_out_instanceID].m_specular;
	float specularTex = texture(sampler2D(instance_data[vs_out_instanceID].m_specularTexture), vs_out_uv).r;
	float shininess = instance_data[vs_out_instanceID].m_shininess.r;

	// transform normal map to world space
	vec3 finalNormal = texture(sampler2D(instance_data[vs_out_instanceID].m_normalsTexture), vs_out_uv).rgb;
	finalNormal = normalize(finalNormal * 2.0 - 1.0);   
	finalNormal = normalize(vs_out_tbnMatrix * finalNormal);
	
	vec4 viewSpacePos = ProjectionViewMatrix * vec4(vs_out_position,1); 
	fs_out_position = vec4(vs_out_position, viewSpacePos.z);
	fs_out_normal = vec4(finalNormal, shininess);
	fs_out_albedo = diffuseOpacity * diffuseTex;
	fs_out_specular = vec4(meshSpecular.xyz * specularTex, meshSpecular.a);
}