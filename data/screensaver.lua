EntityTest = {}

ProFi = require 'ProFi'

local CubeModel = Graphics.LoadModel("cube.fbx")
local SphereModel = Graphics.LoadModel("sphere_low.fbx")
local DiffuseShader = Graphics.LoadShader("diffuse", "simplediffuse.vs", "simplediffuse.fs")
local ShadowShader = Graphics.LoadShader("shadow", "simpleshadow.vs", "simpleshadow.fs");
Graphics.SetShadowShader(DiffuseShader, ShadowShader)
local BasicShader = Graphics.LoadShader("light",  "basic.vs", "basic.fs")

local InstancingTestCount = 18
local LightColours = {
	"004777",
	"a30000",
	"ff7700",
	"efd28d",
	"00afb5"
}
local LightColourIndex = math.random(0,#LightColours)
local lightBoxMin = {-100,7,-100}
local lightBoxMax = {20,128,60}
local lightGravity = -4096.0
local lightBounceMul = 0.5
local lightFriction = 0.95
local lightXZSpeed = 200
local lightYSpeed = 200
local bouncyLights = {}		-- array of {entityHandle, velocity{xyz}}
local spinnyLights = {}		-- array of entityHandle
local spheres = {}			-- array of entityHandle, {transform component, anchorpos}

function GetLightStartPos()
	return {0,0,0}
end

function hex(h)
	local r, g, b = h:match("(%w%w)(%w%w)(%w%w)")
	r = (tonumber(r, 16) or 0) / 255
	g = (tonumber(g, 16) or 0) / 255
	b = (tonumber(b, 16) or 0) / 255
	return r, g, b
end

function MakeLightEntity()
	local newEntity = World.AddEntity()
	local transform = World.AddComponent_Transform(newEntity)
	local p = GetLightStartPos()
	transform:SetPosition(p[1],p[2],p[3])
	transform:SetScale(1,1,1)
	
	LightColourIndex = LightColourIndex + 1
	if(LightColourIndex>#LightColours) then
		LightColourIndex = 1
	end
	local light = World.AddComponent_Light(newEntity)
	light:SetPointLight();
	local r,g,b = hex(LightColours[LightColourIndex])
	light:SetColour(r,g,b)
	light:SetAmbient(0.0)
	light:SetDistance(math.random(16,32))
	light:SetAttenuation(2.5)
	light:SetCastsShadows(false)
	light:SetBrightness(2.0)
	
	table.insert(bouncyLights,{newEntity, {0.0,0.0,0.0}})
end

function MakeShadowLightEntity()
	local newEntity = World.AddEntity()
	local transform = World.AddComponent_Transform(newEntity)
	local p = GetLightStartPos()
	transform:SetPosition(p[1],p[2],p[3])
	transform:SetScale(1,1,1)
	
	LightColourIndex = LightColourIndex + 1
	if(LightColourIndex>#LightColours) then
		LightColourIndex = 1
	end
	local light = World.AddComponent_Light(newEntity)
	light:SetPointLight();
	local r,g,b = hex(LightColours[LightColourIndex])
	light:SetColour(r,g,b)
	light:SetAmbient(0.0)
	local radius = math.random(64,96)
	light:SetDistance(radius)
	light:SetAttenuation(2.5)
	light:SetCastsShadows(true)
	light:SetShadowmapSize(256,256)
	light:SetShadowBias(4.0)
	light:SetBrightness(2.0)
	
	table.insert(bouncyLights,{newEntity, {0.0,0.0,0.0}})
end

function MakeSpotLight()
	local newEntity = World.AddEntity()
	local transform = World.AddComponent_Transform(newEntity)
	local p = GetLightStartPos();
	transform:SetPosition(p[1],p[2],p[3])
	transform:SetRotation(0,math.random(0,360),80)
	transform:SetScale(1,4,1)
	
	local light = World.AddComponent_Light(newEntity)
	LightColourIndex = LightColourIndex + 1
	if(LightColourIndex>#LightColours) then
		LightColourIndex = 1
	end
	local r,g,b = hex(LightColours[LightColourIndex])
	light:SetColour(r,g,b)
	light:SetSpotLight();
	light:SetAmbient(0.05)
	light:SetDistance(100)
	light:SetCastsShadows(true)
	light:SetShadowmapSize(512,512)
	light:SetShadowBias(0.0001)
	light:SetBrightness(2.0)
	light:SetSpotAngles(0.1,0.5)
	
	table.insert(spinnyLights,newEntity)
	table.insert(bouncyLights,{newEntity, {0.0,0.0,0.0}})
end

function MakeModelEntity(x,y,z,scale,model,shader)
	local newEntity = World.AddEntity()
	local transform = World.AddComponent_Transform(newEntity)
	transform:SetPosition(x,y,z)
	transform:SetScale(scale,scale,scale)
	local newModel = World.AddComponent_Model(newEntity)
	newModel:SetModel(model)
	newModel:SetShader(shader)
	table.insert(spheres,{newEntity, {x,y,z}})
end

function EntityTest.Init()
	Graphics.SetClearColour(0.0,0.0,0.0)

	for i=1,4 do 
		MakeLightEntity()
	end
	for i=1,32 do 
		MakeSpotLight()
	end
	for i=1,2 do 
		MakeShadowLightEntity()
	end
	
	local gap = 10
	local offset = -(InstancingTestCount * gap) / 2
	for x=0,InstancingTestCount do
		for y=0,InstancingTestCount do
			for z=0,InstancingTestCount do
				MakeModelEntity(offset + x * 6,8 + y * 6,offset + z * 8,2,SphereModel,DiffuseShader)
			end
		end		
	end
end

function Vec3Length(v)
	return math.sqrt((v[1] * v[1]) + (v[2] * v[2]) + (v[3] * v[3]));
end

local sphereTheta = 0.0
local profile = 10

function EntityTest.Tick(deltaTime)
	if(profile < 10) then
		ProFi:start()
	end
	deltaTime = deltaTime * 0.05
	sphereTheta = sphereTheta + deltaTime * 80
	local sin = math.sin
	local getTransform = World.GetComponent_Transform
	for sp=1,#spheres do
		local anchorPos = spheres[sp][2]
		local transform = getTransform(spheres[sp][1])
		local position = {}
		position[1] = anchorPos[1] + sin(sp * 0.1 + sphereTheta * 0.7) * 1.4
		position[2] = anchorPos[2] + sin(sp * 0.5 + sphereTheta * 0.15) * 1.0
		position[3] = anchorPos[3] + sin(sp * 0.9 + sphereTheta * 0.3) * 2.5
		transform:SetPosition(position[1],position[2],position[3])
	end
	for s=1,#spinnyLights do 
		local entity = spinnyLights[s]
		local transform = World.GetComponent_Transform(entity)
		local rotation = transform:GetRotationDegrees()
		rotation.y = rotation.y + deltaTime * 2000
		transform:SetRotation(rotation.x,rotation.y,rotation.z)
	end
	for b=1,#bouncyLights do 
		local entity = bouncyLights[b][1]
		local velocity = bouncyLights[b][2]
		local transform = World.GetComponent_Transform(entity)
		local position = transform:GetPosition()
		if(Vec3Length(velocity) < 32.0) then
			velocity[1] = (math.random(-200,200) / 100.0) * lightXZSpeed
			velocity[2] = (math.random(200,400) / 100.0) * lightYSpeed
			velocity[3] = (math.random(-200,200) / 100.0) * lightXZSpeed
		end
		velocity[2] = velocity[2] + (lightGravity * deltaTime);
		position.x = position.x + velocity[1] * deltaTime
		position.y = position.y + velocity[2] * deltaTime
		position.z = position.z + velocity[3] * deltaTime
		
		if(position.x < lightBoxMin[1]) then
		 	position.x = lightBoxMin[1]
		 	velocity[1] = -velocity[1] * lightBounceMul
		end
		if(position.y < lightBoxMin[2]) then
			position.y = lightBoxMin[2]
			velocity[1] = velocity[1] * lightFriction
			velocity[2] = -velocity[2] * lightBounceMul
			velocity[3] = velocity[3] * lightFriction
		end
		if(position.z < lightBoxMin[3]) then
			position.z = lightBoxMin[3]
			velocity[3] = -velocity[3] * lightBounceMul
		end
		if(position.x > lightBoxMax[1]) then
			position.x = lightBoxMax[1]
			velocity[1] = -velocity[1] * lightBounceMul
		end
		if(position.y > lightBoxMax[2]) then
			position.y = lightBoxMax[2]
			velocity[2] = -velocity[2] * lightBounceMul
		end
		if(position.z > lightBoxMax[3]) then
			position.z = lightBoxMax[3]
			velocity[3] = -velocity[3] * lightBounceMul
		end
		transform:SetPosition(position.x,position.y,position.z)
		bouncyLights[b][2] = velocity
	end
	if(profile < 10) then
		ProFi:stop()
		ProFi:writeReport( 'prof_' .. profile .. '.txt' )
		profile = profile + 1
	end
end

return EntityTest