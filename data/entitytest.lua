EntityTest = {}

local SphereModel = Graphics.LoadModel("sphere_low.fbx")
local SponzaModel = Graphics.LoadModel("sponza.obj")
local DiffuseShader = Graphics.LoadShader("diffuse", "simplediffuse.vs", "simplediffuse.fs")
local BasicShader = Graphics.LoadShader("light",  "basic.vs", "basic.fs")
local ShadowShader = Graphics.LoadShader("shadow", "simpleshadow.vs", "simpleshadow.fs");
Graphics.SetShadowShader(DiffuseShader, ShadowShader)
local InstancingTestCount = 12
local LightColours = {
	{2.0,0.0,0.0},
	{0.0,2.0,0.0},
	{0.0,0.0,2.0},
	{2.0,0.0,2.0},
	{2.0,2.0,0.0},
	{0.0,2.0,2.0},
}
local LightColourIndex = math.random(0,#LightColours)
local lightBoxMin = {-284,2,-125}
local lightBoxMax = {256,64,113}
local lightGravity = -4096.0
local lightBounceMul = 0.5
local lightFriction = 0.95
local lightXZSpeed = 200
local lightYSpeed = 200
local bouncyLights = {}		-- array of {entityHandle, velocity{xyz}}

function MakeSunEntity()
	local newEntity = World.AddEntity()
	local transform = World.AddComponent_Transform(newEntity)
	transform:SetPosition(300,600,-40)
	transform:SetScale(16,16,16)
	
	local light = World.AddComponent_Light(newEntity)
	light:SetIsPointLight(false);
	light:SetColour(0.96, 0.95, 0.9)
	light:SetAmbient(0.04)
	light:SetCastsShadows(true)
	light:SetShadowmapSize(2048,2048)

	local newModel = World.AddComponent_Model(newEntity)
	newModel:SetModel(SphereModel)
	newModel:SetShader(BasicShader)
end

function MakeLightEntity()
	local newEntity = World.AddEntity()
	local transform = World.AddComponent_Transform(newEntity)
	transform:SetPosition(math.random(lightBoxMin[1],lightBoxMax[1]),math.random(lightBoxMin[2],lightBoxMax[2]),math.random(lightBoxMin[3],lightBoxMax[3]))
	transform:SetScale(1,1,1)
	
	LightColourIndex = LightColourIndex + 1
	if(LightColourIndex>#LightColours) then
		LightColourIndex = 1
	end
	local light = World.AddComponent_Light(newEntity)
	light:SetIsPointLight(true);
	light:SetColour(LightColours[LightColourIndex][1],LightColours[LightColourIndex][2],LightColours[LightColourIndex][3])
	light:SetAmbient(0.1)
	light:SetDistance(math.random(64,64))
	light:SetShadowmapSize(256,256)
	light:SetCastsShadows(true)
	
	local newModel = World.AddComponent_Model(newEntity)
	newModel:SetModel(SphereModel)
	newModel:SetShader(BasicShader)
	
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
end

function EntityTest.Init()
	MakeSunEntity()

	for i=1,8 do 
		MakeLightEntity()
	end
	MakeModelEntity(0,0,0,0.2,SponzaModel,DiffuseShader)
	
	local gap = 10
	local offset = -(InstancingTestCount * gap) / 2
	
	for x=0,InstancingTestCount do
		for y=0,InstancingTestCount do
			for z=0,InstancingTestCount do
				MakeModelEntity(offset + x * 6,8 + y * 6,offset + z * 6,1,SphereModel,DiffuseShader)
			end
		end		
	end
end

function Vec3Length(v)
	return math.sqrt((v[1] * v[1]) + (v[2] * v[2]) + (v[3] * v[3]));
end

function EntityTest.Tick(deltaTime)
	deltaTime = deltaTime * 0.2
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
end

return EntityTest