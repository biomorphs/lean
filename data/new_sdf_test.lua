SDFTest = {}

local DrawShader = Graphics.LoadShader("SDF Draw Basic",  "sdf_model_basic.vs", "sdf_model_basic.fs")
local DrawShaderFancy = Graphics.LoadShader("SDF Diffuse Lighting",  "sdf_model_basic.vs", "sdf_mesh_diffuse.fs")
local ShadowShader = Graphics.LoadShader("SDF Shadows", "sdf_model_shadow.vs", "sdf_shadow.fs")
Graphics.SetShadowShader(DrawShaderFancy, ShadowShader)
local SDFShader = Graphics.LoadComputeShader("SDF Volume Test",  "compute_test_write_volume.cs")

local sdfEntities = {}
local blockSize = {64,64,64}	-- dimensions in meters
local res = {64,64,64}		-- grid resolution
local blockCounts = {32,1,32}	-- blocks in the scene
local sharedMaterial = {}

function MakeSDFMaterial()
	local e = World.AddEntity()
	local m = World.AddComponent_Material(e)
	m:SetFloat("Time",0)
	m:SetSampler("DiffuseTexture", Graphics.LoadTexture("white.bmp"))
	sharedMaterial = e
end

function MakeSunEntity()
	local newEntity = World.AddEntity()
	local transform = World.AddComponent_Transform(newEntity)
	transform:SetPosition(-40.75,125.5,60)
	local light = World.AddComponent_Light(newEntity)
	light:SetPointLight();
	light:SetColour(0.917,0.788,0.607)
	light:SetAmbient(0.3)
	light:SetAttenuation(3)
	light:SetBrightness(0.85)
	light:SetDistance(500)
	light:SetCastsShadows(true)
	light:SetShadowmapSize(2048,2048)
	light:SetShadowBias(0.5)
end

function MakeSDFEntity(pos,scale,bmin,bmax,res,fn)
	sdf_entity = World.AddEntity()
	local transform = World.AddComponent_Transform(sdf_entity)
	transform:SetPosition(pos[1],pos[2],pos[3])
	transform:SetScale(scale[1],scale[2],scale[3])
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
	
	local cellDims = {blockSize[1] / res[1],blockSize[2] / res[2],blockSize[3] / res[3]}
	local overlap = {cellDims[1]*2.5,cellDims[2]*2.5,cellDims[3]*2.5}
	local offset = {-blockCounts[1]*(blockSize[1]-cellDims[1]*2)*0.5,-1,-blockCounts[3]*(blockSize[3]-cellDims[3]*2)*0.5}
	for x=0, blockCounts[1]-1 do 
		for z=0, blockCounts[3]-1 do 
	 		for y=0, blockCounts[2]-1 do 
				local p = {(x*blockSize[1]) - (x*overlap[1]) + offset[1],y*blockSize[2] - (y*overlap[2]) + offset[2],z*blockSize[3] - (z*overlap[3]) + offset[3]}
				MakeSDFEntity({0,0,0},{1,1,1},p,{p[1]+blockSize[1],p[2]+blockSize[2],p[3]+blockSize[3]},res,TestSampleFn)
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
		model:Remesh()
		
		lastRemeshed = lastRemeshed + 1
	end
end 

return SDFTest