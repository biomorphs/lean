#version 430 
#pragma sde include "shared.fs"

uniform sampler2D SourceTexture;
uniform float BlurDirection;
uniform float Weights[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

in vec2 vs_out_position;

out vec4 fs_out_colour;
 
void main()
{
	vec2 uv = (vs_out_position + 1.0) * 0.5;
	
	// seperable gaussian 
	
	vec2 tex_offset = 1.0 / textureSize(SourceTexture, 0); // gets size of single texel
    vec3 result = texture(SourceTexture, uv).rgb * Weights[0]; // current fragment's contribution
    if(BlurDirection == 0.0)
    {
        for(int i = 1; i < 5; ++i)
        {
            result += texture(SourceTexture, uv + vec2(tex_offset.x * i, 0.0)).rgb * Weights[i];
            result += texture(SourceTexture, uv - vec2(tex_offset.x * i, 0.0)).rgb * Weights[i];
        }
    }
    else
    {
        for(int i = 1; i < 5; ++i)
        {
            result += texture(SourceTexture, uv + vec2(0.0, tex_offset.y * i)).rgb * Weights[i];
            result += texture(SourceTexture, uv - vec2(0.0, tex_offset.y * i)).rgb * Weights[i];
        }
    }
    fs_out_colour = vec4(result, 1.0);
}