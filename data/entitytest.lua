EntityTest = {}

local CubeModel = Graphics.LoadModel("cube.fbx")
local SphereModel = Graphics.LoadModel("sphere_low.fbx")
local SponzaModel = Graphics.LoadModel("sponza.obj")
local DiffuseShader = Graphics.LoadShader("diffuse", "simplediffuse.vs", "simplediffuse.fs")
local BasicShader = Graphics.LoadShader("light",  "basic.vs", "basic.fs")
local ShadowShader = Graphics.LoadShader("shadow", "simpleshadow.vs", "simpleshadow.fs");
Graphics.SetShadowShader(DiffuseShader, ShadowShader)
local InstancingTestCount = 15
local LightColours = {
	{1.0,0.0,0.0},
	{0.0,1.0,0.0},
	{0.0,0.0,1.0},
	{1.0,0.0,1.0},
	{1.0,1.0,0.0},
	{0.0,1.0,1.0},
}
local LightColourIndex = math.random(0,#LightColours)
local lightBoxMin = {-284,4,-125}
local lightBoxMax = {256,64,113}
local lightGravity = -50.0
local lightBounceMul = 0.5
local lightFriction = 0.95
local lightXZSpeed = 50
local lightYSpeed = 30
local timeDeltaMulti = 1.0
local bouncyLights = {}		-- array of {entityHandle, velocity{xyz}}
local spinnyLights = {}		-- array of entityHandle

function MakeSunEntity()
	local newEntity = World.AddEntity()
	local transform = World.AddComponent_Transform(newEntity)
	transform:SetPosition(-40,500,0)
	transform:SetRotation(2.2,11,14.7)
	transform:SetScale(4,16,4)
	
	local light = World.AddComponent_Light(newEntity)
	light:SetDirectional();
	light:SetColour(0.4, 0.4, 0.4)
	light:SetAmbient(0.1)
	light:SetDistance(1500)
	light:SetCastsShadows(true)
	light:SetShadowmapSize(2048,2048)
	light:SetShadowBias(0.005)
	light:SetBrightness(1.0)

	local newModel = World.AddComponent_Model(newEntity)
	newModel:SetModel(CubeModel)
	newModel:SetShader(BasicShader)
end

function MakeLightEntity()
	local newEntity = World.AddEntity()
	local transform = World.AddComponent_Transform(newEntity)
	transform:SetPosition(math.random(lightBoxMin[1],lightBoxMax[1]),math.random(lightBoxMin[2],lightBoxMax[2]),math.random(lightBoxMin[3],lightBoxMax[3]))
	transform:SetPosition(-20,5,-5)
	transform:SetScale(1,1,1)
	
	LightColourIndex = LightColourIndex + 1
	if(LightColourIndex>#LightColours) then
		LightColourIndex = 1
	end
	local light = World.AddComponent_Light(newEntity)
	light:SetPointLight();
	light:SetColour(LightColours[LightColourIndex][1],LightColours[LightColourIndex][2],LightColours[LightColourIndex][3])
	light:SetAmbient(0.0)
	light:SetDistance(math.random(16,32))
	light:SetAttenuation(2.5)
	light:SetCastsShadows(false)
	light:SetBrightness(4.0)
	
	local newModel = World.AddComponent_Model(newEntity)
	newModel:SetModel(SphereModel)
	newModel:SetShader(BasicShader)
	
	table.insert(bouncyLights,{newEntity, {0.0,0.0,0.0}})
end

function MakeShadowLightEntity()
	local newEntity = World.AddEntity()
	local transform = World.AddComponent_Transform(newEntity)
	transform:SetPosition(math.random(lightBoxMin[1],lightBoxMax[1]),math.random(lightBoxMin[2],lightBoxMax[2]),math.random(lightBoxMin[3],lightBoxMax[3]))
	transform:SetPosition(-20,5,-5)
	transform:SetScale(1,1,1)
	
	LightColourIndex = LightColourIndex + 1
	if(LightColourIndex>#LightColours) then
		LightColourIndex = 1
	end
	local light = World.AddComponent_Light(newEntity)
	light:SetPointLight();
	light:SetColour(LightColours[LightColourIndex][1],LightColours[LightColourIndex][2],LightColours[LightColourIndex][3])
	light:SetAmbient(0.0)
	local radius = math.random(64,96)
	light:SetDistance(radius)
	light:SetAttenuation(2.5)
	light:SetCastsShadows(true)
	light:SetShadowmapSize(512,512)
	light:SetShadowBias(4.0)
	light:SetBrightness(4.0)
	
	local newModel = World.AddComponent_Model(newEntity)
	newModel:SetModel(SphereModel)
	newModel:SetShader(BasicShader)
	
	table.insert(bouncyLights,{newEntity, {0.0,0.0,0.0}})
end

function MakeSpotLight(x,y,z, rx, ry, rz)
	local newEntity = World.AddEntity()
	local transform = World.AddComponent_Transform(newEntity)
	transform:SetPosition(x,y,z)
	transform:SetRotation(rx, ry, rz)
	transform:SetScale(1,4,1)
	
	local light = World.AddComponent_Light(newEntity)
	LightColourIndex = LightColourIndex + 1
	if(LightColourIndex>#LightColours) then
		LightColourIndex = 1
	end
	light:SetColour(LightColours[LightColourIndex][1],LightColours[LightColourIndex][2],LightColours[LightColourIndex][3])
	light:SetSpotLight();
	light:SetAmbient(0.0)
	light:SetDistance(100)
	light:SetCastsShadows(true)
	light:SetShadowmapSize(1024,1024)
	light:SetShadowBias(0.0001)
	light:SetBrightness(4.0)
	light:SetSpotAngles(0.0,0.8)
	light:SetAttenuation(1.3)
	
	local newModel = World.AddComponent_Model(newEntity)
	newModel:SetModel(CubeModel)
	newModel:SetShader(BasicShader)
	
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
end

function EntityTest.Init()
	Graphics.SetClearColour(0.3,0.3,0.3)
	MakeSunEntity()
	for i=1,60 do 
		MakeLightEntity()
	end
	for i=1,15 do 
		MakeSpotLight(-25,5,-4.5,0,math.random(0,360),45)
	end
	for i=1,2 do 
		MakeShadowLightEntity()
	end
	
	MakeModelEntity(0,0,0,0.2,SponzaModel,DiffuseShader)
	local gap = 10
	local offset = -(InstancingTestCount * gap) / 2
	for x=0,InstancingTestCount do
		for y=0,InstancingTestCount do
			for z=0,InstancingTestCount do
				MakeModelEntity(offset + x * 6,8 + y * 6,offset + z * 8,1,SphereModel,DiffuseShader)
			end
		end		
	end
end

function Vec3Length(v)
	return math.sqrt((v[1] * v[1]) + (v[2] * v[2]) + (v[3] * v[3]));
end

function EntityTest.Tick(deltaTime)
	deltaTime = deltaTime * timeDeltaMulti
	local open = true
	DebugGui.BeginWindow(open,"Script")
	timeDeltaMulti = DebugGui.DragFloat("Time Multi", timeDeltaMulti, 0.01, 0.0, 10.0)
	DebugGui.EndWindow()
	for s=1,#spinnyLights do 
		local entity = spinnyLights[s]
		local transform = World.GetComponent_Transform(entity)
		local rotation = transform:GetRotationDegrees()
		rotation.y = rotation.y + deltaTime * 200
		transform:SetRotation(rotation.x,rotation.y,rotation.z)
	end
	for b=1,#bouncyLights do 
		local entity = bouncyLights[b][1]
		local velocity = bouncyLights[b][2]
		local transform = World.GetComponent_Transform(entity)
		local position = transform:GetPosition()
		if(position.y <= lightBoxMin[2] and Vec3Length(velocity) < 4.0) then
			velocity[1] = (math.random(-100,100) / 100.0) * lightXZSpeed
			velocity[2] = (math.random(100,400) / 100.0) * lightYSpeed
			velocity[3] = (math.random(-100,100) / 100.0) * lightXZSpeed
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