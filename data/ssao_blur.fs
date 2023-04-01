#version 460 
#extension GL_ARB_bindless_texture : enable

in vec2 vs_out_position;
out vec4 fs_out_colour;

uniform sampler2D SourceTexture;	// pre-blur ssao

const int c_noiseDims = 4;	// should match SSAO::c_sampleNoiseDimensions
 
void main()
{
	vec2 uv = (vs_out_position + 1.0) * 0.5;
	vec4 ssao = texture(SourceTexture, uv);
	vec2 texelSize = 1.0 / textureSize(SourceTexture, 0);
	float outputVal = 0.0f;
	const int halfNoiseDims = c_noiseDims / 2;
	for(int x=-halfNoiseDims;x<halfNoiseDims;++x)
	{
		for(int y=-halfNoiseDims;y<halfNoiseDims;++y)
		{
			vec2 sampleOffset = uv + vec2(x,y) * texelSize;
			outputVal += texture(SourceTexture, sampleOffset).r;
		}
	}
	outputVal = outputVal / (c_noiseDims * c_noiseDims);
	fs_out_colour = vec4(outputVal,outputVal,outputVal,1);
}