#version 430 
#pragma sde include "shared.fs"

in vec4 vs_out_colour;
in vec3 vs_out_normal;
in vec4 vs_out_positionLightSpace;
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
uniform sampler2D ShadowMapTexture;
uniform samplerCube ShadowCubeMapTexture;

float CalculateShadows(vec3 normal, vec4 lightSpacePos)
{
	// perform perspective divide
	vec3 projCoords = vs_out_positionLightSpace.xyz / vs_out_positionLightSpace.w;
	
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
	vec2 texelSize = 1.0 / textureSize(ShadowMapTexture, 0);
	for(int x = -1; x <= 1; ++x)
	{
		for(int y = -1; y <= 1; ++y)
		{
			float pcfDepth = texture(ShadowMapTexture, projCoords.xy + vec2(x, y) * texelSize).r; 
			shadow += currentDepth - ShadowBias > pcfDepth ? 1.0 : 0.0;        
		}    
	}
	shadow /= 9.0;
	return shadow;
}

float CalculateCubeShadows(vec3 normal, vec3 pixelWorldSpace, vec3 lightPosition, float cubeDepthFarPlane)
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
	
	float bias = max(0.004 * (1.0 - dot(normal, normalize(fragToLight))), CubeShadowBias);  

	// scale bias based on distance to light (munge factor)
	float distanceToLight = length(fragToLight);
	bias = min(bias * max(distanceToLight * 0.1, 0.0),2.5);

	// pcf
	float shadow = 0.0;
	int samples  = 20;
	float diskRadius = 0.05;
	float currentDepth = length(fragToLight);
	for(int i = 0; i < samples; ++i)
	{
		float closestDepth = texture(ShadowCubeMapTexture, fragToLight + sampleOffsetDirections[i] * diskRadius).r;
		closestDepth *= cubeDepthFarPlane;   // undo mapping [0;1]
		if(currentDepth - bias > closestDepth)
			shadow += 1.0;
	}
	shadow /= float(samples);  

	return shadow;
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
	finalNormal = finalNormal * 2.0 - 1.0;   
	finalNormal = normalize(vs_out_tbnMatrix * finalNormal);

	for(int i=0;i<LightCount;++i)
	{
		float attenuation;
		vec3 lightDir;
		float shadow = 0.0;
		if(Lights[i].Position.w == 0.0)		// directional light
		{
			attenuation = 1.0;
			lightDir = normalize(Lights[i].Position.xyz);
			if(Lights[i].ShadowParams.x != 0.0)
			{
				shadow = CalculateShadows(finalNormal, vs_out_positionLightSpace);
			}
		}
		else	// point light
		{
			float lightDistance = length(Lights[i].Position.xyz - vs_out_position);
			attenuation = 1.0 / (Lights[i].Attenuation[0] + 
								(Lights[i].Attenuation[1] * lightDistance) + 
								(Lights[i].Attenuation[2] * (lightDistance * lightDistance)));
			lightDir = normalize(Lights[i].Position.xyz - vs_out_position);
			if(Lights[i].ShadowParams.x != 0.0)
			{
				shadow = CalculateCubeShadows(finalNormal,vs_out_position, Lights[i].Position.xyz, Lights[i].ShadowParams.y);
			}
		}

		// diffuse light
		float diffuseFactor = max(dot(finalNormal, lightDir),0.0);
		vec3 diffuse = MeshDiffuseOpacity.rgb * diffuseTex.rgb * Lights[i].ColourAndAmbient.rgb * diffuseFactor;

		// ambient light
		vec3 ambient = diffuseTex.rgb * Lights[i].ColourAndAmbient.rgb * Lights[i].ColourAndAmbient.a;

		// specular light 
		vec3 viewDir = normalize(CameraPosition.xyz - vs_out_position);
		vec3 reflectDir = normalize(reflect(-lightDir, finalNormal));  
		float specFactor = pow(max(dot(viewDir, reflectDir), 0.0), MeshShininess);
		vec3 specularColour = MeshSpecular.rgb * Lights[i].ColourAndAmbient.rgb;
		vec3 specular = MeshSpecular.a * specFactor * specularColour * specularTex; 

		vec3 diffuseSpec = (1.0-shadow) * (diffuse + specular);
		finalColour += attenuation * (ambient + diffuseSpec);
	}
	
	// tonemap
	finalColour = Tonemap_ACESFilm(vs_out_colour.rgb * finalColour * HDRExposure);
	fs_out_colour = vec4(linearToSRGB(finalColour),MeshDiffuseOpacity.a * diffuseTex.a);
}