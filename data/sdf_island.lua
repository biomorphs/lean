Island = {}

-- todo 
-- user edits
-- make a giant buffer to store all user SDFs (100mb+, persistent mapped?)
-- keep all SDFs stored in blocks of (x*y*z), indexed in a hastable 
-- beginWriteUserData(areaMin,areaMax,writeUserDataFn)
--	for each cell in grid (simple 3d grid hashing)
--	 (user data storage is NOT tied to the octree at all - allows global sharing later)
--	 if no data exists for this cell, allocate now from shared buffer
--	 writeUserDataFn(cell index, pos, ptr, etc, etc...)
--	mark octree nodes touching the areas as out of date
-- when calling sdf write:
--	upload cells hash table along with buffer to write

-- in sdf write shader:
-- 	check if userdata cell exists at this pos
--		if it does, combine it with the current d somehow (write min value?)

local IslandSDFSize = {2000,128,2000}
local IslandSDFBlockRes = {32,32,32}
local DrawSDFShader = Graphics.LoadShader("Planet Shader",  "sdf_model_basic.vs", "island_terrain.fs")
local ShadowShader = Graphics.LoadShader("SDF Shadows", "sdf_model_shadow.vs", "sdf_shadow.fs")
local ModelDiffuseShader = Graphics.LoadShader("model_diffuse", "simplediffuse.vs", "simplediffuse.fs")
local ModelShadowShader = Graphics.LoadShader("model_shadow", "simpleshadow.vs", "simpleshadow.fs");
Graphics.SetShadowShader(DrawSDFShader, ShadowShader)
Graphics.SetShadowShader(ModelDiffuseShader, ModelShadowShader)

local currentIsland = {}

function MakeSunEntity()
	local newEntity = World.AddEntity()
	local t = World.AddComponent_Tags(newEntity)
	t:AddTag(Tag.new("Sun"))
	
	local transform = World.AddComponent_Transform(newEntity)
	transform:SetPosition(2000,1316,2000)
	transform:SetRotation(46.5,42.3,-5.4)
	
	local light = World.AddComponent_Light(newEntity)
	light:SetDirectional();
	light:SetColour(1,1,1)
	light:SetAmbient(0.02)
	light:SetAttenuation(3)
	light:SetBrightness(0.724)
	light:SetDistance(4000)
	light:SetCastsShadows(true)
	light:SetShadowmapSize(4096,4096)
	light:SetShadowOrthoScale(1420)
	light:SetShadowBias(0.001)
	
	newEntity = World.AddEntity()
	local t = World.AddComponent_Tags(newEntity)
	t:AddTag(Tag.new("Backlight"))
	transform = World.AddComponent_Transform(newEntity)
	transform:SetPosition(0,0,0)
	transform:SetRotation(-86,-85,-171)
	light = World.AddComponent_Light(newEntity)
	light:SetDirectional();
	light:SetColour(0.05,0.125,0.2)
	light:SetAmbient(0.0)
	light:SetBrightness(1)
end

function MakeBlighty()
	e = World.AddEntity()
	local t = World.AddComponent_Tags(e)
	t:AddTag(Tag.new("Blighty"))
	local m = World.AddComponent_Material(e)
	m:SetVec4("MeshUVOffsetScale", vec4.new(0,0,1,1))
	m:SetFloat("MaxHeight",1024)
	m:SetSampler("DiffuseTexture", Graphics.LoadTexture("Rock_028_SD/Rock_028_COLOR.jpg"))
	m:SetSampler("BeachTexture", Graphics.LoadTexture("sand.jpg"))
	m:SetSampler("WaterTexture", Graphics.LoadTexture("textures/water/Water_001_COLOR.jpg"))
	m:SetSampler("GrassTexture", Graphics.LoadTexture("grass2.jpg"))
	m:SetSampler("Heightmap2D", Graphics.LoadTexture("Rocky Land and Rivers/Height Map PNG.png"))
	m:SetIsTransparent(false)
	m:SetFloat("GrassMinHeight",1.5)
	m:SetFloat("GrassAngleMin",0.98)
	m:SetFloat("GrassAngleMul",40)
	m:SetFloat("BeachHeight",1.7)
	m:SetFloat("BeachAngleMin",0.51)
	m:SetFloat("BeachAngleMul",1.23)
	m:SetFloat("BeachThickness",0.87)
	m:SetFloat("WaterHeight",0.0)
	m:SetVec4("WaterColour",vec4.new(0.05,0.1,0.12,1))
	local transform = World.AddComponent_Transform(e)
	transform:SetPosition(0,0,0)
	local sdfModel = World.AddComponent_SDFMesh(e)
	sdfModel:SetBounds(vec3.new(0,0,0),vec3.new(2000,1024,2000))
	sdfModel:SetResolution(IslandSDFBlockRes[1],IslandSDFBlockRes[2],IslandSDFBlockRes[3])
	sdfModel:SetRenderShader(DrawSDFShader)
	sdfModel:SetSDFShaderPath("heightmap_to_sdf.cs")
	sdfModel:SetMaterialEntity(e)
	sdfModel:SetOctreeDepth(7)
	sdfModel:SetLOD(3,2000)
	sdfModel:SetLOD(4,1000)
	sdfModel:SetLOD(5,500)
	sdfModel:SetLOD(6,200)
	currentIsland = e
end

function MakeIsland()
	e = World.AddEntity()
	local t = World.AddComponent_Tags(e)
	t:AddTag(Tag.new("Island"))
	local m = World.AddComponent_Material(e)
	m:SetVec4("MeshUVOffsetScale", vec4.new(0,0,1,1))
	m:SetFloat("MaxHeight",500)
	m:SetSampler("DiffuseTexture", Graphics.LoadTexture("Rock_028_SD/Rock_028_COLOR.jpg"))
	m:SetSampler("BeachTexture", Graphics.LoadTexture("sand.jpg"))
	m:SetSampler("WaterTexture", Graphics.LoadTexture("textures/water/Water_001_COLOR.jpg"))
	m:SetSampler("GrassTexture", Graphics.LoadTexture("grass2.jpg"))
	m:SetSampler("Heightmap2D", Graphics.LoadTexture("Mt Ruapehu and Mt Ngauruhoe Heightmap_0.png"))
	m:SetFloat("GrassMinHeight",146.6)
	m:SetFloat("GrassAngleMin",0.84)
	m:SetFloat("GrassAngleMul",6.55)
	m:SetFloat("BeachHeight",143.3)
	m:SetFloat("BeachAngleMin",0.0)
	m:SetFloat("BeachAngleMul",7.41)
	m:SetFloat("BeachThickness",0.19)
	m:SetFloat("WaterHeight",138.9)
	m:SetFloat("NormalSampleBias",1)
	m:SetVec4("WaterColour",vec4.new(0.05,0.1,0.12,1))
	local transform = World.AddComponent_Transform(e)
	transform:SetPosition(0,0,0)
	local sdfModel = World.AddComponent_SDFMesh(e)
	sdfModel:SetBounds(vec3.new(0,0,0),vec3.new(2000,512,2000))
	sdfModel:SetResolution(IslandSDFBlockRes[1],IslandSDFBlockRes[2],IslandSDFBlockRes[3])
	sdfModel:SetRenderShader(DrawSDFShader)
	sdfModel:SetSDFShaderPath("heightmap_to_sdf.cs")
	sdfModel:SetMaterialEntity(e)
	sdfModel:SetOctreeDepth(7)
	sdfModel:SetLOD(3,2200)
	sdfModel:SetLOD(4,1200)
	sdfModel:SetLOD(5,600)
	sdfModel:SetLOD(6,300)
	currentIsland = e
end

function Island.Init()
	Graphics.SetClearColour(0.3,0.55,0.8)
	MakeSunEntity()
	MakeBlighty()
end

function Island.Tick(deltaTime)	
	local keepOpen=true
	DebugGui.BeginWindow(keepOpen,"Island")
	if(DebugGui.Button("Blighty")) then 
		if(currentIsland ~= nil) then 
			World.RemoveEntity(currentIsland)
		end 
		MakeBlighty()
	end
	if(DebugGui.Button("Island")) then 
		if(currentIsland ~= nil) then 
			World.RemoveEntity(currentIsland)
		end 
		MakeIsland()
	end
	DebugGui.EndWindow()
end 

return Island