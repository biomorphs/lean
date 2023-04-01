#version 460 
#extension GL_ARB_bindless_texture : enable
#pragma sde include "shared.fs"

in vec2 vs_out_position;

layout(std140, binding = 1) uniform SSAO_HemisphereSamples
{
	vec3 Samples[32];
};
const int SSAO_KernelSize = 16;
uniform float SSAO_Radius = 8.0f;
uniform float SSAO_Bias = 0.2;
uniform float SSAO_RangeMulti = 0.5;
uniform float SSAO_PowerMulti = 1.0;
uniform sampler2D SSAO_Noise;
uniform vec2 SSAO_NoiseScale;
uniform sampler2D GBuffer_Pos;
uniform sampler2D GBuffer_NormalShininess;

out vec4 fs_out_colour;
 
void main()
{
	vec2 uv = (vs_out_position + 1.0) * 0.5;
	vec4 wsPos = texture(GBuffer_Pos, uv);
	vec3 wsNormal = texture(GBuffer_NormalShininess, uv).xyz;
	vec3 randomNoise = texture(SSAO_Noise, uv * SSAO_NoiseScale).xyz;  

	// gramm-schmit to create tbn from jittered current normal
	vec3 tangent   = normalize(randomNoise - wsNormal * dot(randomNoise, wsNormal));
	vec3 bitangent = cross(wsNormal, tangent);
	mat3 TBN       = mat3(tangent, bitangent, wsNormal);  
	
	float occlusionAmount = 0.0f;
	for(int i=0; i < SSAO_KernelSize; i++)
	{
		// jitter position on hemisphere around normal in world space
		vec3 samplePosition = TBN * Samples[i];
		samplePosition = wsPos.xyz + samplePosition * SSAO_Radius;
		
		// transform pos to screen space
		vec4 posScreenSpace = ProjectionViewMatrix * vec4(samplePosition,1);
		posScreenSpace.xyz /= posScreenSpace.w;
		posScreenSpace.xyz = posScreenSpace.xyz * 0.5 + 0.5;	// back to 0-1 for sampling texture 
		
		if(posScreenSpace.x >= 0.0 && posScreenSpace.x <= 1.0f && posScreenSpace.y >= 0.0 && posScreenSpace.y < 1.0f)
		{
			// calculate occlusion
			float sampleDepth = texture(GBuffer_Pos, posScreenSpace.xy).w;
			float rangeCheck = smoothstep(0.0, 1.0, SSAO_Radius / abs(wsPos.w - sampleDepth) * SSAO_RangeMulti);
			float occlusion = (sampleDepth + SSAO_Bias < wsPos.w ? 1.0 : 0.0) * rangeCheck;  
			occlusionAmount += occlusion;
		}
	}
	occlusionAmount = 1.0 - (occlusionAmount / SSAO_KernelSize);
	occlusionAmount = pow(occlusionAmount,SSAO_PowerMulti);
	
	fs_out_colour = vec4(vec3(occlusionAmount), 1);
}