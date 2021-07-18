uniform sampler2D Heightmap2D;
uniform float MaxHeight = 500;

vec2 WorldPosToHeightmapUV(vec3 worldPos)
{
	return vec2(worldPos.x,-worldPos.z) / textureSize(Heightmap2D,0);
}

float SDF(vec3 worldPos)
{
	vec2 uv = WorldPosToHeightmapUV(worldPos);

	return worldPos.y - (texture2D(Heightmap2D,uv).r * MaxHeight);
}