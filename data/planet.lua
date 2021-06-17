Planet = {}

local DrawPlanetShader = Graphics.LoadShader("Planet Shader",  "sdf_model_basic.vs", "planet_shader.fs")
local DrawWaterShader = Graphics.LoadShader("Water Shader",  "sdf_model_basic.vs", "sdf_mesh_diffuse.fs")
local ShadowShader = Graphics.LoadShader("SDF Shadows", "sdf_model_shadow.vs", "sdf_shadow.fs")
Graphics.SetShadowShader(DrawPlanetShader, ShadowShader)
Graphics.SetShadowShader(DrawWaterShader, ShadowShader)
local SDFPlanetShader = Graphics.LoadComputeShader("Planet volume",  "planet_write_volume.cs")
local SDFWaterShader = Graphics.LoadComputeShader("Planet water",  "planet_write_water_volume.cs")

local blockSize = {1024,1024,1024}	-- dimensions in meters
local res = {32,32,32}				-- mesh resolution

function MakePlanetMaterial()
	local e = World.AddEntity()
	
	return e
end

function MakeWaterMaterial()
	local e = World.AddEntity()
	local m = World.AddComponent_Material(e)
	m:SetFloat("OceanRadius", 455)
	m:SetVec4("PlanetCenter", vec4.new(512,512,512,0))
	m:SetVec4("MeshUVOffsetScale", vec4.new(0,0,1,1))
	m:SetIsTransparent(true)
	m:SetCastShadows(false)
	m:SetVec4("MeshDiffuseOpacity", vec4.new(0,0.37,0.54,0.32))
	m:SetSampler("DiffuseTexture", Graphics.LoadTexture("white.bmp"))
	m:SetVec4("MeshSpecular", vec4.new(0.76,0.89,1,1.68))
	m:SetFloat("MeshShininess", 21)
	return e
end

function MakeSunEntity()
	local newEntity = World.AddEntity()
	local transform = World.AddComponent_Transform(newEntity)
	transform:SetPosition(1000,1050,1000)
	transform:SetRotation(46.5,42.3,-5.4)
	local light = World.AddComponent_Light(newEntity)
	light:SetDirectional();
	light:SetColour(1,1,1)
	light:SetAmbient(0.02)
	light:SetAttenuation(3)
	light:SetBrightness(0.724)
	light:SetDistance(1800)
	light:SetCastsShadows(true)
	light:SetShadowmapSize(4096,4096)
	light:SetShadowOrthoScale(605)
	light:SetShadowBias(0.001)
	
	newEntity = World.AddEntity()
	transform = World.AddComponent_Transform(newEntity)
	transform:SetPosition(0,0,0)
	transform:SetRotation(87.4,0,41.8)
	light = World.AddComponent_Light(newEntity)
	light:SetDirectional();
	light:SetColour(0.05,0.125,0.2)
	light:SetAmbient(0.0)
	light:SetBrightness(1)
end

function MakePlanet()
	e = World.AddEntity()
	local m = World.AddComponent_Material(e)
	m:SetFloat("PlanetRadius", 450)
	m:SetVec4("PlanetCenter", vec4.new(512,512,512,0))
	m:SetVec4("MeshUVOffsetScale", vec4.new(0,0,1,1))
	m:SetSampler("GrassTexture", Graphics.LoadTexture("grass2.jpg"))
	m:SetSampler("RockTexture", Graphics.LoadTexture("Stylized_Rocks_001_SD/Stylized_Rocks_001_basecolor.jpg"))
	local transform = World.AddComponent_Transform(e)
	transform:SetPosition(0,0,0)
	local sdfModel = World.AddComponent_SDFMesh(e)
	sdfModel:SetBounds(vec3.new(0,0,0),vec3.new(blockSize[1],blockSize[2],blockSize[3]))
	sdfModel:SetResolution(res[1],res[2],res[3])
	sdfModel:SetRenderShader(DrawPlanetShader)
	sdfModel:SetSDFShader(SDFPlanetShader)
	sdfModel:SetMaterialEntity(e)
	sdfModel:SetOctreeDepth(6)
	sdfModel:SetLOD(5,256)
	sdfModel:SetLOD(4,512)
	sdfModel:SetLOD(3,1024)
end

function MakeOcean()
	e = World.AddEntity()
	local m = World.AddComponent_Material(e)
	m:SetFloat("OceanRadius", 455)
	m:SetVec4("PlanetCenter", vec4.new(512,512,512,0))
	m:SetVec4("MeshUVOffsetScale", vec4.new(0,0,1,1))
	m:SetIsTransparent(true)
	m:SetCastShadows(false)
	m:SetVec4("MeshDiffuseOpacity", vec4.new(0,0.37,0.54,0.32))
	m:SetSampler("DiffuseTexture", Graphics.LoadTexture("white.bmp"))
	m:SetVec4("MeshSpecular", vec4.new(0.76,0.89,1,1.68))
	m:SetFloat("MeshShininess", 21)
	local transform = World.AddComponent_Transform(e)
	transform:SetPosition(0,0,0)
	local sdfModel = World.AddComponent_SDFMesh(e)
	sdfModel:SetBounds(vec3.new(0,0,0),vec3.new(blockSize[1],blockSize[2],blockSize[3]))
	sdfModel:SetResolution(res[1],res[2],res[3])
	sdfModel:SetRenderShader(DrawWaterShader)
	sdfModel:SetSDFShader(SDFWaterShader)
	sdfModel:SetMaterialEntity(e)
	sdfModel:SetOctreeDepth(3)
end

function Planet.Init()
	Graphics.SetClearColour(0.0,0.0,0.002)
	MakeSunEntity()
	
	MakePlanet()
	MakeOcean()
end

function Planet.Tick(deltaTime)	
end 

return Planet