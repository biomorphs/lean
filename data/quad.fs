#version 430
in vec4 out_colour;
in vec2 out_uv;
out vec4 colour;

uniform sampler2D DiffuseTexture;

const float gamma = 2.2;

vec4 srgbToLinear(vec4 v)
{
	return vec4(pow(v.rgb, vec3(gamma)), v.a);
}

vec4 linearToSRGB(vec4 v)
{
	return vec4(pow(v.rgb, vec3(1.0 / gamma)), v.a);
}
 
void main(){
	colour = linearToSRGB(srgbToLinear(texture(DiffuseTexture, out_uv)) * out_colour);
}