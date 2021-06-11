SDFTest = {}

local DrawShader = Graphics.LoadShader("SDF Draw Basic",  "sdf_model_basic.vs", "sdf_model_basic.fs")
local DrawShaderFancy = Graphics.LoadShader("SDF Diffuse Lighting",  "sdf_model_basic.vs", "sdf_mesh_diffuse.fs")
local ShadowShader = Graphics.LoadShader("SDF Shadows", "sdf_model_shadow.vs", "sdf_shadow.fs")
Graphics.SetShadowShader(DrawShaderFancy, ShadowShader)
local SDFShader = Graphics.LoadComputeShader("SDF Volume Test",  "compute_test_write_volume.cs")

local sdfEntities = {}
local blockSize = {64,64,64}	-- dimensions in meters
local res = {32,32,32}		-- grid resolution
local blockCounts = {2,1,2}	-- blocks in the scene
local sharedMaterial = {}

function MakeSDFMaterial()
	local e = World.AddEntity()
	local m = World.AddComponent_Material(e)
	m:SetFloat("Time",0)
	m:SetFloat("NormalSampleBias",1.5)
	m:SetSampler("DiffuseTexture", Graphics.LoadTexture("sand.jpg"))
	sharedMaterial = e
end

function MakeSunEntity()
	local newEntity = World.AddEntity()
	local transform = World.AddComponent_Transform(newEntity)
	transform:SetPosition(-40.75,160.5,60)
	local light = World.AddComponent_Light(newEntity)
	light:SetPointLight();
	light:SetColour(0.917,0.788,0.607)
	light:SetAmbient(0.3)
	light:SetAttenuation(3)
	light:SetBrightness(0.85)
	light:SetDistance(900)
	light:SetCastsShadows(true)
	light:SetShadowmapSize(2048,2048)
	light:SetShadowBias(2)
end

function MakeSDFEntity(pos,bmin,bmax,res)
	sdf_entity = World.AddEntity()
	local transform = World.AddComponent_Transform(sdf_entity)
	transform:SetPosition(pos[1],pos[2],pos[3])
	local sdfModel = World.AddComponent_SDFMesh(sdf_entity)
	sdfModel:SetBoundsMin(bmin[1],bmin[2],bmin[3])
	sdfModel:SetBoundsMax(bmax[1],bmax[2],bmax[3])
	sdfModel:SetResolution(res[1],res[2],res[3])
	sdfModel:SetRenderShader(DrawShaderFancy)
	sdfModel:SetSDFShader(SDFShader)
	sdfModel:SetMaterialEntity(sharedMaterial)
	sdfModel:Remesh()
	table.insert(sdfEntities,sdf_entity)
end

function SDFTest.Init()
	Graphics.SetClearColour(0.15,0.26,0.4)
	MakeSunEntity()
	MakeSDFMaterial()
	
	for x=0, blockCounts[1] do
		for y=0, blockCounts[2] do
			for z=0, blockCounts[3] do
				local p = {x * blockSize[1],y * blockSize[2],z * blockSize[3]}
				MakeSDFEntity({0,0,0},p,{p[1]+blockSize[1],p[2]+blockSize[2],p[3]+blockSize[3]},res)
			end
		end
	end
	
end

local lastRemeshed = 1
local keepRemeshing = true

function SDFTest.Tick(deltaTime)	
	local windowOpen = true
	
	if keepRemeshing then
		if(lastRemeshed > #sdfEntities) then 
			lastRemeshed = 1
		end
		
		local model = World.GetComponent_SDFMesh(sdfEntities[lastRemeshed])
		--model:Remesh()
		
		lastRemeshed = lastRemeshed + 1
	end
end 

return SDFTest