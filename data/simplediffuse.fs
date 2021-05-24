#version 430 
#pragma sde include "shared.fs"

in vec3 vs_out_normal;
in vec2 vs_out_uv;
in vec3 vs_out_position;
in mat3 vs_out_tbnMatrix;
out vec4 fs_out_colour;

uniform vec4 MeshDiffuseOpacity;
uniform vec4 MeshSpecular;	//r,g,b,strength
uniform float MeshShininess;

uniform sampler2D DiffuseTexture;
uniform sampler2D NormalsTexture;
uniform sampler2D SpecularTexture;

uniform sampler2D ShadowMaps[16];
uniform samplerCube ShadowCubeMaps[16];

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
	// early out if we can
	vec4 diffuseTex = srgbToLinear(texture(DiffuseTexture, vs_out_uv));	
	if(diffuseTex.a == 0.0 || MeshDiffuseOpacity.a == 0.0)
		discard;

	vec3 finalColour = vec3(0.0);
	vec3 specularTex = texture(SpecularTexture, vs_out_uv).rgb;

	// transform normal map to world space
	vec3 finalNormal = texture(NormalsTexture, vs_out_uv).rgb;
	finalNormal = normalize(finalNormal * 2.0 - 1.0);   
	finalNormal = normalize(vs_out_tbnMatrix * finalNormal);
	finalNormal = normalize(vs_out_normal);	// fixme, tbn is wrong with transforms (non orthaganol?)

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
				specular = MeshSpecular.a * specFactor * specularColour * specularTex; 
			}
			
			vec3 diffuseSpec = (1.0-shadow) * (diffuse + specular);
			finalColour += attenuation * (ambient + diffuseSpec);
		}
	}
	
	// apply exposure here, assuming next pass is postfx
	fs_out_colour = vec4(finalColour * HDRExposure,MeshDiffuseOpacity.a * diffuseTex.a);
}