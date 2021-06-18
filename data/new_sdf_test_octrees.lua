SDFTest = {}

local DrawShader = Graphics.LoadShader("SDF Draw Basic",  "sdf_model_basic.vs", "sdf_model_basic.fs")
local DrawShaderFancy = Graphics.LoadShader("SDF Diffuse Lighting",  "sdf_model_basic.vs", "sdf_mesh_diffuse.fs")
local ShadowShader = Graphics.LoadShader("SDF Shadows", "sdf_model_shadow.vs", "sdf_shadow.fs")
Graphics.SetShadowShader(DrawShaderFancy, ShadowShader)
local SDFShader = Graphics.LoadComputeShader("SDF Volume Test",  "compute_test_write_volume.cs")

local sdfEntities = {}
local blockSize = {4096,512,4096}	-- dimensions in meters
local res = {32,32,32}				-- grid resolution
local blockCounts = {1,1,1}			-- blocks in the scene
local sharedMaterial = {}

function MakeSDFMaterial()
	local e = World.AddEntity()
	local m = World.AddComponent_Material(e)
	m:SetFloat("Time",164)
	m:SetFloat("NormalSampleBias",1.5)
	m:SetVec4("MeshUVOffsetScale", vec4.new(0,0,1,1))
	m:SetSampler("DiffuseTexture", Graphics.LoadTexture("white.bmp"))
	sharedMaterial = e
end

function MakeSunEntity()
	local newEntity = World.AddEntity()
	local transform = World.AddComponent_Transform(newEntity)
	transform:SetPosition(2445,1752,2356)
	transform:SetRotation(46.5,42.3,-5.4)
	local light = World.AddComponent_Light(newEntity)
	light:SetDirectional();
	light:SetColour(1,1,1)
	light:SetAmbient(0.3)
	light:SetAttenuation(3)
	light:SetBrightness(0.724)
	light:SetDistance(4000)
	light:SetCastsShadows(true)
	light:SetShadowmapSize(4096,4096)
	light:SetShadowOrthoScale(1541)
	light:SetShadowBias(0.001)
end

function MakeSDFEntity(pos,bmin,bmax,res)
	sdf_entity = World.AddEntity()
	local transform = World.AddComponent_Transform(sdf_entity)
	transform:SetPosition(pos[1],pos[2],pos[3])
	local sdfModel = World.AddComponent_SDFMesh(sdf_entity)
	sdfModel:SetBounds(vec3.new(bmin[1],bmin[2],bmin[3]),vec3.new(bmax[1],bmax[2],bmax[3]))
	sdfModel:SetResolution(res[1],res[2],res[3])
	sdfModel:SetRenderShader(DrawShaderFancy)
	sdfModel:SetSDFShader(SDFShader)
	sdfModel:SetMaterialEntity(sharedMaterial)
	sdfModel:SetOctreeDepth(7)
	sdfModel:SetLOD(4,3000)
	sdfModel:SetLOD(5,2000)
	sdfModel:SetLOD(6,1000)
	table.insert(sdfEntities,sdf_entity)
end

function SDFTest.Init()
	Graphics.SetClearColour(0.15,0.26,0.4)
	MakeSunEntity()
	MakeSDFMaterial()
	
	local p = {0,0,0}
	MakeSDFEntity({0,0,0},p,{p[1]+blockSize[1],p[2]+blockSize[2],p[3]+blockSize[3]},res)
end

local lastRemeshed = 1
local keepRemeshing = true

function SDFTest.Tick(deltaTime)	
	local windowOpen = true
	
	if keepRemeshing then
		if(lastRemeshed > #sdfEntities) then 
			lastRemeshed = 1
		end
		
		local maxindex = math.min(lastRemeshed + 2,#sdfEntities)
		for i=lastRemeshed,#sdfEntities do
			local model = World.GetComponent_SDFMesh(sdfEntities[i])
			--model:Remesh()
		end
		
		lastRemeshed = lastRemeshed + 1
	end
end 

return SDFTest