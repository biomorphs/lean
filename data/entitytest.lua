EntityTest = {}

local SphereModel = Graphics.LoadModel("sphere_low.fbx")
local SponzaModel = Graphics.LoadModel("sponza.obj")
local DiffuseShader = Graphics.LoadShader("diffuse", "simplediffuse.vs", "simplediffuse.fs")
local BasicShader = Graphics.LoadShader("light",  "basic.vs", "basic.fs")
local ShadowShader = Graphics.LoadShader("shadow", "simpleshadow.vs", "simpleshadow.fs");
Graphics.SetShadowShader(DiffuseShader, ShadowShader)
local InstancingTestCount = 0

function MakeSunEntity()
	local newEntity = World.AddEntity()
	local transform = World.AddComponent_Transform(newEntity)
	transform:SetPosition(300,600,-40)
	transform:SetScale(16,16,16)
	
	local light = World.AddComponent_Light(newEntity)
	light:SetIsPointLight(false);
	light:SetColour(0.165, 0.2133, 0.3)
	light:SetAmbient(0.3)
	light:SetCastsShadows(true)
	light:SetShadowmapSize(1024,1024)

	local newModel = World.AddComponent_Model(newEntity)
	newModel:SetModel(SphereModel)
	newModel:SetShader(BasicShader)
	
	local newEntity = World.AddEntity()
	local transform = World.AddComponent_Transform(newEntity)
	transform:SetPosition(100,400,500)
	transform:SetScale(16,16,16)
	
	local light = World.AddComponent_Light(newEntity)
	light:SetIsPointLight(false);
	light:SetColour(0.165, 0.2133, 0.3)
	light:SetAmbient(0.3)
	light:SetCastsShadows(true)
	light:SetShadowmapSize(1024,1024)

	local newModel = World.AddComponent_Model(newEntity)
	newModel:SetModel(SphereModel)
	newModel:SetShader(BasicShader)
end

function MakeLightEntity()
	local newEntity = World.AddEntity()
	local transform = World.AddComponent_Transform(newEntity)
	transform:SetPosition(math.random(-48,48),math.random(16,64),math.random(-48,48))
	transform:SetScale(2,2,2)
	
	local light = World.AddComponent_Light(newEntity)
	light:SetIsPointLight(true);
	light:SetColour(0.8,0.7,0.7)
	light:SetAmbient(0.0)
	light:SetDistance(400)
	light:SetCastsShadows(false)
	
	local newModel = World.AddComponent_Model(newEntity)
	newModel:SetModel(SphereModel)
	newModel:SetShader(BasicShader)
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

	for i=1,1 do 
		MakeLightEntity()
	end
	MakeModelEntity(0,0,0,0.2,SponzaModel,DiffuseShader)
	-- 
	-- local gap = 7
	-- local offset = -(InstancingTestCount * gap) / 2
	-- 
	-- for x=0,InstancingTestCount do
	-- 	for y=0,InstancingTestCount do
	-- 		for z=0,InstancingTestCount do
	-- 			MakeModelEntity(offset + x * 6,8 + y * 6,offset + z * 6,1,SphereModel,DiffuseShader)
	-- 		end
	-- 	end		
	-- end
end

function EntityTest.Tick(deltaTime)
end

return EntityTest