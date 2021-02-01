EntityTest = {}

local SphereModel = Graphics.LoadModel("sphere_low.fbx")
local SponzaModel = Graphics.LoadModel("sponza.obj")
local IslandModel = Graphics.LoadModel("islands_low.fbx")
local DiffuseShader = Graphics.LoadShader("diffuse", "simplediffuse.vs", "simplediffuse.fs")
local BasicShader = Graphics.LoadShader("light",  "basic.vs", "basic.fs")
local ShadowShader = Graphics.LoadShader("shadow", "simpleshadow.vs", "simpleshadow.fs");
Graphics.SetShadowShader(DiffuseShader, ShadowShader)
local InstancingTestCount = 17

local SunMulti = 0.3
local SunPosition = {300,800,-40}
local SunColour = {0.55, 0.711, 1.0}
local SunAmbient = 0.3

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
	for i=1,1 do 
		MakeLightEntity()
	end
	MakeModelEntity(0,1,0,0.4,IslandModel,DiffuseShader)
	MakeModelEntity(0,0,0,0.2,SponzaModel,DiffuseShader)
	
	local gap = 7
	local offset = -(InstancingTestCount * gap) / 2
	
	for x=0,InstancingTestCount do
		for y=0,InstancingTestCount do
			for z=0,InstancingTestCount do
				MakeModelEntity(offset + x * 6,8 + y * 6,offset + z * 6,1,SphereModel,DiffuseShader)
			end
		end		
	end
end

function EntityTest.Tick(deltaTime)
	Graphics.DirectionalLight(SunPosition[1],SunPosition[2],SunPosition[3], SunMulti * SunColour[1], SunMulti * SunColour[2], SunMulti * SunColour[3], SunAmbient)
end

return EntityTest