#version 430 
#pragma sde include "shared.fs"

in vec3 vs_out_normal;
in vec3 vs_out_position;

out vec4 fs_out_colour;

uniform sampler2D SandTexture;
uniform sampler2D RockTexture;
uniform sampler2D GrassTexture;
uniform sampler2D ShadowMaps[16];
uniform samplerCube ShadowCubeMaps[16];

uniform vec4 MeshUVOffsetScale = vec4(0.0,0.0,1.0,1.0);	// xy=offset, zw=scale
uniform vec4 MeshDiffuseOpacity = vec4(1.0,1.0,1.0,1.0);
uniform vec4 MeshSpecular = vec4(1.0,1.0,1.0,0.0);	//r,g,b,strength
uniform float MeshShininess = 1.0;

uniform vec4 PlanetCenter;
uniform float PlanetRadius;
uniform float OceanRadius;

float CalculateShadows(vec3 normal, vec3 position, float shadowIndex, mat4 lightSpaceTransform, float bias)
{
	vec4 positionLightSpace = lightSpaceTransform * vec4(position,1.0); 
	
	// perform perspective divide
	vec3 projCoords = positionLightSpace.xyz / positionLightSpace.w;
	
	// transform from ndc space since depth map is 0-1
	projCoords = projCoords * 0.5 + 0.5;	
	
	// early out if out of range of shadow map 
	if(projCoords.z > 1.0 || projCoords.x > 1.0f || projCoords.y > 1.0f || projCoords.x < 0.0f || projCoords.y < 0.0f)
	{
		return 0.0;
	}

	// simple pcf
	float currentDepth = projCoords.z;
	float shadow = 0.0;
	vec2 texelSize = 1.0 / textureSize(ShadowMaps[int(shadowIndex)], 0);
	for(int x = -1; x <= 1; ++x)
	{
		for(int y = -1; y <= 1; ++y)
		{
			float pcfDepth = texture(ShadowMaps[int(shadowIndex)], projCoords.xy + vec2(x, y) * texelSize).r; 
			shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;        
		}    
	}
	shadow /= 9.0;
	return shadow;
}

float CalculateCubeShadows(vec3 normal, vec3 pixelWorldSpace, vec3 lightPosition, float cubeDepthFarPlane, float shadowIndex, float bias)
{
	vec3 fragToLight = pixelWorldSpace - lightPosition;  
	vec3 sampleOffsetDirections[20] = vec3[]
	(
	   vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1), 
	   vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
	   vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
	   vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
	   vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
	);   

	// pcf
	float shadow = 0.0;
	int samples  = 20;
	float diskRadius = 0.1;
	float currentDepth = length(fragToLight);
	for(int i = 0; i < samples; ++i)
	{
		float closestDepth = texture(ShadowCubeMaps[int(shadowIndex)], fragToLight + sampleOffsetDirections[i] * diskRadius).r;
		closestDepth *= cubeDepthFarPlane;   // undo mapping [0;1]
		if(currentDepth - bias > closestDepth)
			shadow += 1.0;
	}
	shadow /= float(samples);  

	return shadow;
}

float CalculateShadow(uint i, vec3 normal)
{
	float shadow = 0.0;
	if(AllLights[i].Position.w == 1.0)		// point light
	{
		shadow = CalculateCubeShadows(normal,vs_out_position, AllLights[i].Position.xyz, AllLights[i].DistanceAttenuation.x, AllLights[i].ShadowParams.y, AllLights[i].ShadowParams.z);
	}
	else if(AllLights[i].Position.w == 0.0)	// directional
	{
		shadow = CalculateShadows(normal, vs_out_position, AllLights[i].ShadowParams.y, AllLights[i].LightspaceTransform, AllLights[i].ShadowParams.z);
	}
	else
	{
		shadow = CalculateShadows(normal, vs_out_position, AllLights[i].ShadowParams.y, AllLights[i].LightspaceTransform, AllLights[i].ShadowParams.z);
	} 
	return shadow;
}

float CalculateAttenuation(uint i, vec3 lightDir)
{
	if(AllLights[i].Position.w == 0.0)		// directional
	{
		return 1.0;
	}
	else if(AllLights[i].Position.w == 2.0)		// spot
	{
		float distance = length(AllLights[i].Position.xyz - vs_out_position);			
		float attenuation = pow(smoothstep(AllLights[i].DistanceAttenuation.x, 0, distance),AllLights[i].DistanceAttenuation.y);
		
		float theta = dot(lightDir, normalize(AllLights[i].Position.xyz - vs_out_position)); 
		float outerAngle = AllLights[i].SpotlightAngles.y;
		float innerAngle = AllLights[i].SpotlightAngles.x;
		attenuation *= clamp((theta - outerAngle) / (outerAngle - innerAngle), 0.0, 1.0);    
		
		return attenuation;
	}
	else	
	{
		float distance = length(AllLights[i].Position.xyz - vs_out_position);			
		float attenuation = pow(smoothstep(AllLights[i].DistanceAttenuation.x, 0, distance),AllLights[i].DistanceAttenuation.y);
		return attenuation;
	}
}

vec3 CalculateDirection(uint i)
{
	if(AllLights[i].Position.w == 1.0)		// point light
	{
		return normalize(AllLights[i].Position.xyz - vs_out_position);
	}
	else
	{
		return normalize(-AllLights[i].Direction);
	}
}
 
void main()
{
	vec3 finalColour = vec3(0.0);

	// transform normal map to world space
	vec3 finalNormal = normalize(vs_out_normal);
	
	// triplanar mapping 
	vec3 blending = abs( finalNormal );
	blending = normalize(max(blending, 0.00001)); // Force weights to sum to 1.0
	float b = (blending.x + blending.y + blending.z);
	blending /= vec3(b, b, b);
	
	vec2 xAxisUV = MeshUVOffsetScale.xy + vs_out_position.yz * MeshUVOffsetScale.zw * 0.05f;
	vec2 yAxisUV = MeshUVOffsetScale.xy + vs_out_position.xz * MeshUVOffsetScale.zw * 0.05f;
	vec2 zAxisUV = MeshUVOffsetScale.xy + vs_out_position.xy * MeshUVOffsetScale.zw * 0.05f;
	vec4 xaxis = srgbToLinear( texture2D( GrassTexture, xAxisUV) );
	vec4 yaxis = srgbToLinear( texture2D( GrassTexture, yAxisUV) );
	vec4 zaxis = srgbToLinear( texture2D( GrassTexture, zAxisUV) );
	vec4 grassTex = xaxis * blending.x + yaxis * blending.y + zaxis * blending.z;
	
	xaxis = srgbToLinear( texture2D( RockTexture, xAxisUV) );
	yaxis = srgbToLinear( texture2D( RockTexture, yAxisUV) );
	zaxis = srgbToLinear( texture2D( RockTexture, zAxisUV) );
	vec4 rockTex = xaxis * blending.x + yaxis * blending.y + zaxis * blending.z;
	
	xaxis = srgbToLinear( texture2D( SandTexture, xAxisUV) );
	yaxis = srgbToLinear( texture2D( SandTexture, yAxisUV) );
	zaxis = srgbToLinear( texture2D( SandTexture, zAxisUV) );
	vec4 sandTex = xaxis * blending.x + yaxis * blending.y + zaxis * blending.z;
	
	vec3 viewDir = normalize(vs_out_position - CameraPosition.xyz);
	
	vec3 worldToCenter = vs_out_position - PlanetCenter.xyz;
	float grassAmount = clamp((dot(normalize(worldToCenter), finalNormal) - 0.9) * 8,0,1);
	float distanceToOcean = length(worldToCenter)-OceanRadius;
	float sandAmount = clamp((distanceToOcean -8) * 0.25,0,1) - clamp((distanceToOcean - 25) * 0.1,0,1);;
	
	vec3 diffuseColour = grassAmount * grassTex.rgb + (1-grassAmount) * rockTex.rgb;
	diffuseColour = sandAmount * sandTex.rgb + (1-sandAmount) * diffuseColour;
	uint lightTileIndex = GetLightTileIndex(GetScreenTileIndices(gl_FragCoord.xy));
	uint lightCount = LightTiles[lightTileIndex].Count;
	for(int i=0;i<lightCount;++i)
	{
		uint lightIndex = LightTiles[lightTileIndex].Indices[i];
		vec3 lightDir = CalculateDirection(lightIndex);
		float attenuation = CalculateAttenuation(lightIndex, lightDir);
		float shadow = 0.0;
		if(attenuation > 0.0)
		{	
			if(AllLights[lightIndex].ShadowParams.x != 0.0)
			{
				shadow = CalculateShadow(lightIndex, finalNormal);
			}
			
			vec3 matColour = diffuseColour * AllLights[lightIndex].ColourAndAmbient.rgb;

			// diffuse light
			float diffuseFactor = max(dot(finalNormal, lightDir),0.0);
			vec3 diffuse = matColour * diffuseFactor;

			// ambient light
			vec3 ambient = matColour * AllLights[lightIndex].ColourAndAmbient.a;

			// specular light (blinn phong)
			vec3 specular = vec3(0.0);
			if(MeshSpecular.a > 0.0)
			{
				vec3 halfwayDir = normalize(lightDir + viewDir);  
				float specFactor = pow(max(dot(finalNormal, halfwayDir), 0.0), max(MeshShininess,0.000000001));
				vec3 specularColour = MeshSpecular.rgb * AllLights[lightIndex].ColourAndAmbient.rgb;
				specular = MeshSpecular.a * specFactor * specularColour; 
			}
			
			vec3 diffuseSpec = (1.0-shadow) * (diffuse + specular);
			finalColour += attenuation * (ambient + diffuseSpec);
		}
	}

	// apply exposure here, assuming next pass is postfx
	//fs_out_colour = vec4(vec3(distanceToOcean) / 20 * HDRExposure,1.0);
	fs_out_colour = vec4(finalColour.rgb * HDRExposure,1.0);
	//fs_out_colour = vec4(clamp(normalize(worldToCenter).rgb,0,1),1.0);
	//fs_out_colour = vec4(vec3(grassAmount) * HDRExposure,1.0);
	//fs_out_colour = vec4(vec3(grassAmount),1.0);
	//fs_out_colour = vec4(max(vec3(0),finalNormal.rgb) * HDRExposure,1.0);
}