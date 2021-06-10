SDFTest = {}

local DrawShader = Graphics.LoadShader("SDF Draw Basic",  "sdf_model_basic.vs", "sdf_model_basic.fs")
local SDFShader = Graphics.LoadComputeShader("SDF Volume Test",  "compute_test_write_volume.cs")

local sdfEntities = {}
local blockSize = {64,64,64}	-- dimensions in meters
local res = {64,64,64}		-- grid resolution
local blockCounts = {2,1,1}	-- blocks in the scene
local sharedMaterial = {}

function MakeSDFMaterial()
	local e = World.AddEntity()
	local m = World.AddComponent_Material(e)
	m:SetFloat("Time",0)
	sharedMaterial = e
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
	sdfModel:SetRenderShader(DrawShader)
	sdfModel:SetSDFShader(SDFShader)
	sdfModel:SetMaterialEntity(sharedMaterial)
	sdfModel:Remesh()
	table.insert(sdfEntities,sdf_entity)
end

function SDFTest.Init()
	Graphics.SetClearColour(0.15,0.26,0.4)
	MakeSDFMaterial()
	
	local cellDims = {blockSize[1] / res[1],blockSize[2] / res[2],blockSize[3] / res[3]}
	local overlap = {cellDims[1]*2,cellDims[2]*2,cellDims[3]*2}
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
	
	local currentRes = {res[1],res[2],res[3]}
	res[1] = DebugGui.DragFloat("ResX", res[1], 1, 1, 128)
	res[2] = DebugGui.DragFloat("ResY", res[2], 1, 1, 128)
	res[3] = DebugGui.DragFloat("ResZ", res[3], 1, 1, 128)
	if(res[1] ~= currentRes[1] or res[2] ~= currentRes[2] or res[3] ~= currentRes[3]) then
		for m=1, #sdfEntities do
			local model = World.GetComponent_SDFModel(sdfEntities[m])
			model:SetResolution(res[1],res[2],res[3])
		end
	end
	
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