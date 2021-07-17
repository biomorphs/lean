Planet = {}

local DrawPlanetShader = Graphics.LoadShader("Planet Shader",  "sdf_model_basic.vs", "planet_shader.fs")
local DrawWaterShader = Graphics.LoadShader("Water Shader",  "sdf_model_basic.vs", "sdf_mesh_diffuse.fs")
local ShadowShader = Graphics.LoadShader("SDF Shadows", "sdf_model_shadow.vs", "sdf_shadow.fs")
local ModelDiffuseShader = Graphics.LoadShader("model_diffuse", "simplediffuse.vs", "simplediffuse.fs")
local ModelShadowShader = Graphics.LoadShader("model_shadow", "simpleshadow.vs", "simpleshadow.fs");
Graphics.SetShadowShader(DrawPlanetShader, ShadowShader)
Graphics.SetShadowShader(DrawWaterShader, ShadowShader)
Graphics.SetShadowShader(ModelDiffuseShader, ModelShadowShader)
local SphereModel = Graphics.LoadModel("sphere_low.fbx")

local blockSize = {1500,1500,1500}	-- dimensions in meters
local res = {32,32,32}				-- mesh resolution
local oceanHeight = 455
local planetRadius = 450
local drones = {}

function MakeDrone(pos)
	local newEntity = World.AddEntity()
	local t = World.AddComponent_Tags(newEntity)
	t:AddTag(Tag.new("Drone"))
	
	local transform = World.AddComponent_Transform(newEntity)
	transform:SetPosition(pos[1],pos[2],pos[3])
	transform:SetScale(4,4,4)
	
	local newModel = World.AddComponent_Model(newEntity)
	newModel:SetModel(SphereModel)
	newModel:SetShader(ModelDiffuseShader)
	
	table.insert(drones, newEntity)
end

function MakeSunEntity()
	local newEntity = World.AddEntity()
	local t = World.AddComponent_Tags(newEntity)
	t:AddTag(Tag.new("Sun"))
	
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

function MakePlanet()
	e = World.AddEntity()
	local t = World.AddComponent_Tags(e)
	t:AddTag(Tag.new("Planet"))
	local m = World.AddComponent_Material(e)
	m:SetFloat("PlanetRadius", planetRadius)
	m:SetFloat("OceanRadius", oceanHeight)
	m:SetVec4("PlanetCenter", vec4.new(512,512,512,0))
	m:SetVec4("MeshUVOffsetScale", vec4.new(0,0,1,1))
	m:SetSampler("GrassTexture", Graphics.LoadTexture("grass2.jpg"))
	m:SetSampler("RockTexture", Graphics.LoadTexture("Stylized_Rocks_001_SD/Stylized_Rocks_001_basecolor.jpg"))
	m:SetSampler("SandTexture", Graphics.LoadTexture("sand.jpg"))
	local transform = World.AddComponent_Transform(e)
	transform:SetPosition(0,0,0)
	local sdfModel = World.AddComponent_SDFMesh(e)
	sdfModel:SetBounds(vec3.new(0,0,0),vec3.new(blockSize[1],blockSize[2],blockSize[3]))
	sdfModel:SetResolution(res[1],res[2],res[3])
	sdfModel:SetRenderShader(DrawPlanetShader)
	sdfModel:SetSDFShaderPath("planet_write_volume.cs")
	sdfModel:SetMaterialEntity(e)
	sdfModel:SetOctreeDepth(6)
	sdfModel:SetLOD(5,400)
	sdfModel:SetLOD(4,800)
	sdfModel:SetLOD(3,2048)
	sdfModel:SetLOD(2,3000)
end

function MakeOcean()
	e = World.AddEntity()
	local t = World.AddComponent_Tags(e)
	t:AddTag(Tag.new("Ocean"))
	local m = World.AddComponent_Material(e)
	m:SetFloat("OceanRadius", 465)
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
	sdfModel:SetResolution(48,48,48)
	sdfModel:SetOctreeDepth(1)
	sdfModel:SetRenderShader(DrawWaterShader)
	sdfModel:SetSDFShaderPath("planet_write_water_volume.cs")
	sdfModel:SetMaterialEntity(e)
end

function Planet.Init()
	Graphics.SetClearColour(0.3,0.55,0.8)
	MakeSunEntity()
	MakePlanet()
	MakeOcean()
	MakeDrone({173.5,893.75,408.25})
	MakeDrone({900,100,807})
end

function RandomFloat(minv,maxv) 
	return minv + (math.random() * (maxv-minv))
end

function Vec3Length(v)
	return math.sqrt((v[1] * v[1]) + (v[2] * v[2]) + (v[3] * v[3]));
end

function Vec3Normalise(v)
	local vLength = Vec3Length(v)
	if(vLength > 0) then 
		return {v[1]/vLength,v[2]/vLength,v[3]/vLength}
	else
		return {0,0,0}
	end
end

function DroneRayResults(rayHits, rayMisses)
	for r=1,#rayHits do 
		local s = rayHits[r].start
		local pos = rayHits[r].hitPos
		local normal = rayHits[r].hitNormal
		local col = {(1.0+normal.x)/2.0,(1.0+normal.y)/2.0,(1.0+normal.z)/2.0}
		Graphics.DebugDrawLine(s.x,s.y,s.z,pos.x,pos.y,pos.z,col[1],col[2],col[3],1,col[1],col[2],col[3],1)
	end
end

function Planet.Tick(deltaTime)	
	for i=1,#drones do
		local t = World.GetComponent_Transform(drones[i])
		local s = t:GetPosition()
		local raysToCast = {}
		for r=1,1000 do 
			local dir = {RandomFloat(-1,1),RandomFloat(-1,1),RandomFloat(-1,1)}
			dir = Vec3Normalise(dir)
			local e = vec3.new(s.x + dir[1] * 256, s.y + dir[2] * 256, s.z + dir[3] * 256)
			table.insert(raysToCast,RayInput.new(s, e))
		end
		Raycast.DoAsync(raysToCast, DroneRayResults)
	end
end 

return Planet