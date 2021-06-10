#version 430 
#pragma sde include "shared.fs"

in vec3 vs_out_normal;
in vec3 vs_out_position;

out vec4 fs_out_colour;

uniform sampler2D DiffuseTexture;
uniform sampler2D ShadowMaps[16];
uniform samplerCube ShadowCubeMaps[16];

uniform vec4 MeshDiffuseOpacity = vec4(1.0,1.0,1.0,1.0);
uniform vec4 MeshSpecular = vec4(1.0,1.0,1.0,0.0);	//r,g,b,strength
uniform float MeshShininess = 1.0;

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
	float mul = 0.25;
	vec2 texelSize = 1.0 / textureSize(ShadowMaps[int(shadowIndex)], 0);
	for(int x = -1; x <= 1; ++x)
	{
		for(int y = -1; y <= 1; ++y)
		{
			float pcfDepth = texture(ShadowMaps[int(shadowIndex)], projCoords.xy + vec2(x, y) * texelSize * mul).r; 
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
	float diskRadius = 0.05;
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

float CalculateShadow(int i, vec3 normal)
{
	float shadow = 0.0;
	if(Lights[i].Position.w == 1.0)		// point light
	{
		shadow = CalculateCubeShadows(normal,vs_out_position, Lights[i].Position.xyz, Lights[i].DistanceAttenuation.x, Lights[i].ShadowParams.y, Lights[i].ShadowParams.z);
	}
	else if(Lights[i].Position.w == 0.0)	// directional
	{
		shadow = CalculateShadows(normal, vs_out_position, Lights[i].ShadowParams.y, Lights[i].LightspaceTransform, Lights[i].ShadowParams.z);
	}
	else
	{
		shadow = CalculateShadows(normal, vs_out_position, Lights[i].ShadowParams.y, Lights[i].LightspaceTransform, Lights[i].ShadowParams.z);
	} 
	return shadow;
}

float CalculateAttenuation(int i, vec3 lightDir)
{
	if(Lights[i].Position.w == 0.0)		// directional
	{
		return 1.0;
	}
	else if(Lights[i].Position.w == 2.0)		// spot
	{
		float distance = length(Lights[i].Position.xyz - vs_out_position);			
		float attenuation = pow(smoothstep(Lights[i].DistanceAttenuation.x, 0, distance),Lights[i].DistanceAttenuation.y);
		
		float theta = dot(lightDir, normalize(Lights[i].Position.xyz - vs_out_position)); 
		float outerAngle = Lights[i].SpotlightAngles.y;
		float innerAngle = Lights[i].SpotlightAngles.x;
		attenuation *= clamp((theta - outerAngle) / (outerAngle - innerAngle), 0.0, 1.0);    
		
		return attenuation;
	}
	else	
	{
		float distance = length(Lights[i].Position.xyz - vs_out_position);			
		float attenuation = pow(smoothstep(Lights[i].DistanceAttenuation.x, 0, distance),Lights[i].DistanceAttenuation.y);
		return attenuation;
	}
}

vec3 CalculateDirection(int i)
{
	if(Lights[i].Position.w == 1.0)		// point light
	{
		return normalize(Lights[i].Position.xyz - vs_out_position);
	}
	else
	{
		return normalize(-Lights[i].Direction);
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
	
	vec4 xaxis = srgbToLinear( texture2D( DiffuseTexture, vs_out_position.yz * 0.05f) );
	vec4 yaxis = srgbToLinear( texture2D( DiffuseTexture, vs_out_position.xz * 0.05f) );
	vec4 zaxis = srgbToLinear( texture2D( DiffuseTexture, vs_out_position.xy * 0.05f) );
	
	// blend the results of the 3 planar projections.
	vec4 diffuseTex = xaxis * blending.x + yaxis * blending.y + zaxis * blending.z;
	
	vec3 viewDir = normalize(CameraPosition.xyz - vs_out_position);
	
	for(int i=0;i<LightCount;++i)
	{
		vec3 lightDir = CalculateDirection(i);
		float attenuation = CalculateAttenuation(i, lightDir);
		float shadow = 0.0;
		if(attenuation > 0.0)
		{	
			if(Lights[i].ShadowParams.x != 0.0)
			{
				shadow = CalculateShadow(i, finalNormal);
			}
			
			vec3 matColour = MeshDiffuseOpacity.rgb * diffuseTex.rgb * Lights[i].ColourAndAmbient.rgb;

			// diffuse light
			float diffuseFactor = max(dot(finalNormal, lightDir),0.0);
			vec3 diffuse = matColour * diffuseFactor;

			// ambient light
			vec3 ambient = matColour * Lights[i].ColourAndAmbient.a;

			// specular light (blinn phong)
			vec3 specular = vec3(0.0);
			if(MeshSpecular.a > 0.0)
			{
				vec3 halfwayDir = normalize(lightDir + viewDir);  
				float specFactor = pow(max(dot(finalNormal, halfwayDir), 0.0), max(MeshShininess,0.000000001));
				vec3 specularColour = MeshSpecular.rgb * Lights[i].ColourAndAmbient.rgb;
				specular = MeshSpecular.a * specFactor * specularColour; 
			}
			
			vec3 diffuseSpec = (1.0-shadow) * (diffuse + specular);
			finalColour += attenuation * (ambient + diffuseSpec);
		}
	}
	
	// apply exposure here, assuming next pass is postfx
	fs_out_colour = vec4(finalColour.rgb * HDRExposure,1.0);
}