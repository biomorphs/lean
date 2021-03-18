Medieval = {}

local BasicShader = Graphics.LoadShader("basic", "basic.vs", "basic.fs")
local DiffuseShader = Graphics.LoadShader("diffuse", "simplediffuse.vs", "simplediffuse.fs")
local ShadowShader = Graphics.LoadShader("shadow", "simpleshadow.vs", "simpleshadow.fs")

local House1 = Graphics.LoadModel("medieval/buildings/house_1.fbx")
local House2 = Graphics.LoadModel("medieval/buildings/house_2.fbx")
local House3 = Graphics.LoadModel("medieval/buildings/house_2.fbx")
local House4 = Graphics.LoadModel("medieval/buildings/house_2.fbx")
local Belltower = Graphics.LoadModel("medieval/buildings/Bell_Tower.fbx")
local Blacksmith = Graphics.LoadModel("medieval/buildings/Blacksmith.fbx")
local Inn = Graphics.LoadModel("medieval/buildings/Inn.fbx")
local Mill = Graphics.LoadModel("medieval/buildings/Mill.fbx")
local Sawmill = Graphics.LoadModel("medieval/buildings/Sawmill.fbx")
local Stable = Graphics.LoadModel("medieval/buildings/Stable.fbx")
local Path_Straight = Graphics.LoadModel("medieval/props/Path_Straight.fbx")
local Well = Graphics.LoadModel("medieval/props/well.fbx")
local Barrel = Graphics.LoadModel("medieval/props/barrel.fbx")
local Cart = Graphics.LoadModel("medieval/props/Cart.fbx")
local Market1 = Graphics.LoadModel("medieval/props/MarketStand_1.fbx")
local Market2 = Graphics.LoadModel("medieval/props/MarketStand_2.fbx")
local Rock1 = Graphics.LoadModel("medieval/props/Rock_1.fbx")
local Rock2 = Graphics.LoadModel("medieval/props/Rock_2.fbx")
local Rock3 = Graphics.LoadModel("medieval/props/Rock_2.fbx")
local Fence = Graphics.LoadModel("medieval/props/Fence.fbx")
local Floor = Graphics.LoadModel("floor_4x4.fbx")
local Sphere = Graphics.LoadModel("sphere.fbx")
local Tree = Graphics.LoadModel("treefixed.fbx")
local Fire = Graphics.LoadModel("medieval/props/Bonfire_Lit.fbx")
local Forest = Graphics.LoadModel("forest.gltf")

function MakeSunEntity()
	local newEntity = World.AddEntity()
	local transform = World.AddComponent_Transform(newEntity)
	transform:SetPosition(170,200,245)
	transform:SetRotation(38,0,-34)
	
	local light = World.AddComponent_Light(newEntity)
	light:SetDirectional();
	light:SetColour(0.22, 0.25, 0.5)
	light:SetAmbient(0.07)
	light:SetBrightness(0.15)
	light:SetDistance(600)
	light:SetCastsShadows(true)
	light:SetShadowmapSize(4096,4096)
	light:SetShadowBias(0.0015)
end

function MakeModelEntity(x,y,z,rx,ry,rz,model)
	local newEntity = World.AddEntity()
	local transform = World.AddComponent_Transform(newEntity)
	transform:SetPosition(x,y,z)
	transform:SetRotation(rx,ry,rz)
	transform:SetScale(0.25,0.25,0.25)
	local newModel = World.AddComponent_Model(newEntity)
	newModel:SetModel(model)
	newModel:SetShader(DiffuseShader)
end

function MakeModelEntityScaled(x,y,z,rx,ry,rz,s,model)
	local newEntity = World.AddEntity()
	local transform = World.AddComponent_Transform(newEntity)
	transform:SetPosition(x,y,z)
	transform:SetRotation(rx,ry,rz)
	transform:SetScale(s,s,s)
	local newModel = World.AddComponent_Model(newEntity)
	newModel:SetModel(model)
	newModel:SetShader(DiffuseShader)
end

function MakePointLight(x,y,z,d,r,g,b,a,br)
	local newEntity = World.AddEntity()
	local transform = World.AddComponent_Transform(newEntity)
	transform:SetPosition(x,y,z)
	
	local light = World.AddComponent_Light(newEntity)
	light:SetPointLight();
	light:SetColour(r,g,b)
	light:SetAmbient(a)
	light:SetDistance(d)
	light:SetAttenuation(2.6)
	light:SetCastsShadows(true)
	light:SetShadowmapSize(512,512)
	light:SetShadowBias(2.0)
	light:SetBrightness(br)
	
	-- local newModel = World.AddComponent_Model(newEntity)
	-- newModel:SetModel(Sphere)
	-- newModel:SetShader(BasicShader)
end

function Medieval.Init()
	Graphics.SetClearColour(0.01,0.02,0.04)
	Graphics.SetShadowShader(DiffuseShader, ShadowShader)
	Graphics.SetShadowShader(BasicShader, ShadowShader)
	
	MakeSunEntity()
	
	MakeModelEntity(0,0,0,0,180,0,House1)
	MakeModelEntity(64,0,0,0,180,0,House2)
	MakeModelEntity(-64,0,0,0,180,0,House3)
	MakeModelEntity(-160,0,96,0,270,0,House4)
	
	MakeModelEntity(-160,0,16,0,270,0,Belltower)
	MakeModelEntity(64,0,200,0,180,0,Blacksmith)
	MakeModelEntity(-64,0,200,0,180,0,Inn)
	MakeModelEntity(-240,0,350,0,90,0,Mill)
	MakeModelEntity(200,0,0,0,0,0,Stable)
	MakeModelEntity(-240,0,250,0,90,0,Sawmill)
	
	for i=0,10 do
		if(math.random(1,100)<80) then
			MakeModelEntity(-16 + i * math.random(22,23),0,64,0,math.random(-2,2),0,Fence)
		end
		if(math.random(1,100)<80) then
			MakeModelEntity(-16 + i * 22,0,130,0,math.random(-2,2),0,Fence)
		end
		MakeModelEntity(-16 + i * 24.45,0,83.6,0,90,0,Path_Straight)
		MakeModelEntity(-16 + i * 24.45,0,96,0,90,0,Path_Straight)
		MakeModelEntity(-16 + i * 24.45,0,108.4,0,90,0,Path_Straight)
	end
	
	MakeModelEntity(-64,0,96,0,-12,0,Well)
	
	MakeModelEntityScaled(-21,0,176,0,0,0,0.5,Barrel)
	MakeModelEntityScaled(-16.5,0,184,0,0,0,0.5,Barrel)
	MakeModelEntityScaled(-15.5,4,169,90,0,-27,0.5,Barrel)
	
	MakeModelEntity(171.5,0,164,0,107,0,Cart)
	
	MakeModelEntity(-64.5,0,320,0,0,0,Market1)
	MakeModelEntity(-64.5,0,360,0,0,0,Market2)
	
	for i=0,20 do 
		MakeModelEntity(math.random(-256,256),0,math.random(-128,512),0,0,0,Rock1)
	end
	for i=0,20 do 
		MakeModelEntity(math.random(-256,256),0,math.random(-128,512),0,0,0,Rock2)
	end
	for i=0,20 do 
		MakeModelEntity(math.random(-256,256),0,math.random(-128,512),0,0,0,Rock3)
	end

	MakeModelEntityScaled(-64,-100,128,0,0,0,100,Floor)
	
	MakeModelEntityScaled(2,1.3,96,0,0,0,0.5,Fire)
	MakePointLight(2,20,96,128, 1.0,0.56,0.3, 0.1,2.0)
	
	MakeModelEntityScaled(-134,1.3,320,0,0,0,0.5,Fire)
	MakePointLight(-134,20,320,128, 0.95,0.3,0.12, 0.2,4.0)
	
	MakePointLight(155,17.5,14.5,96, 0.99,0.56,0.3, 0.05,1.0)
	
	MakeModelEntityScaled(105.75,-5,34,0,146,0,15,Tree)
	MakeModelEntityScaled(-105.75,-28,55,0,-86,-5,12,Tree)
	
	MakeModelEntityScaled(-295.25,54.25,3.5,0,0,0,5,Forest)
	MakePointLight(-289,70,-9.25,64,1.0,0.42,0.149,0.0,20.0)
	MakePointLight(-297,86,11.0,45,0.0,0.88,1.0,0.01,40.0)
end

function DrawGrid(startX,endX,stepX,startZ,endZ,stepZ,yAxis)
	for x=startX,endX,stepX do
		Graphics.DebugDrawLine(x,yAxis,startZ,x,yAxis,endZ,0.2,0.2,0.2,1.0,0.2,0.2,0.2,1.0)
	end
	for z=startZ,endZ,stepZ do
		Graphics.DebugDrawLine(startX,yAxis,z,endX,yAxis,z,0.2,0.2,0.2,1.0,0.2,0.2,0.2,1.0)
	end
end

function Medieval.Tick(deltaTime)
	-- DrawGrid(-512,512,32,-512,512,32,0.0)
end

return Medieval