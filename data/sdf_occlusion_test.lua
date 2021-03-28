SDFTest = {}

ProFi = require 'ProFi'

local ModelDiff = Graphics.LoadShader("simplediffuse", "simplediffuse.vs", "simplediffuse.fs")
local ModelShadow = Graphics.LoadShader("model_shadow", "simpleshadow.vs", "simpleshadow.fs")
local BasicShader = Graphics.LoadShader("model_basic",  "basic.vs", "basic.fs")
local DiffuseShader = Graphics.LoadShader("sdf_diffuse", "sdf_model.vs", "sdf_model_diffuse.fs")
local ShadowShader = Graphics.LoadShader("sdf_shadows", "sdf_shadow.vs", "sdf_shadow.fs")
Graphics.SetShadowShader(DiffuseShader, ShadowShader)
Graphics.SetShadowShader(ModelDiff, ModelShadow)
Graphics.SetShadowShader(BasicShader, ModelShadow)

function Vec3Length(v)
	return math.sqrt((v[1] * v[1]) + (v[2] * v[2]) + (v[3] * v[3]));
end

function Vec3LengthSq(v)
	return((v[1] * v[1]) + (v[2] * v[2]) + (v[3] * v[3]));
end

function Vec3Dot(v1,v2)
	return (v1[1] * v2[1]) + (v1[2] * v2[2]) + (v1[3] * v2[3])
end

function Vec2Length(v)
	return math.sqrt((v[1] * v[1]) + (v[2] * v[2]))
end

function Vec3Min(v1,v2)
	return (math.min(v1[1],v2[1], math.min(v1[2],v2[2]), math.min(v1[3],v2[3])))
end

function Plane( p, n, h )
  return Vec3Dot(p,n) + h;
end

function Sphere(p, radius)
	return Vec3Length(p)-radius;
end

function Torus( p, t ) -- pos(3), torus wid/leng?(2)
  local q = { Vec2Length({p[1],p[3]})-t[1], p[2] }
  return Vec2Length(q)-t[2];
end

function TriPrism( p, h )	-- pos, w/h
  local q = { math.abs(p[1]), math.abs(p[2]), math.abs(p[3]) };
  return math.max(q[3]-h[2],math.max(q[1]*0.866025+p[2]*0.5,-p[2])-h[1]*0.5);
end

function OpUnion( d1, d2 ) 
	return math.min(d1,d2)
end

local a = 0
local sdfEntities = {}
local blockSize = {16,16,16}	-- dimensions in meters
local res = {64,64,64}		-- grid resolution
local blockCounts = {1,1,1}	-- blocks in the scene
local remeshPerFrame = 1
local debugMeshing = true
local meshMode = "SurfaceNet"	-- Blocky/SurfaceNet/DualContour
local useLuaSampleFn = false
local normalSmoothness = 1

function TestSampleFn(x,y,z)
	local d = OpUnion(Sphere({x,y-0.1,z}, 0.2 + (1.0 + math.cos(a * 0.7 + 1.2)) * 0.5),Plane({x,y,z}, {0.0,2.0 - math.cos(x + a * 4) * 1.0 + 1.0 + math.sin(z + a * 0.3),0.0}, 1.0))
	d = OpUnion(Torus({x-3,y-0.8,z-1},{1.5,0.1 + (1.0 + math.cos(a)) * 0.25}), d)
	d = OpUnion(TriPrism({x,z-2.0,y-2},{0.5,1.0}), d)
	return d, 10
end

function MakeSunEntity()
	local newEntity = World.AddEntity()
	local transform = World.AddComponent_Transform(newEntity)
	transform:SetPosition(4,6.25,10.5)
	transform:SetRotation(67.8,21.1,-18.5)
	local light = World.AddComponent_Light(newEntity)
	light:SetDirectional();
	light:SetColour(0.917,0.788,0.607)
	light:SetAmbient(0.092)
	light:SetBrightness(0.267)
	light:SetDistance(27.6)
	light:SetCastsShadows(true)
	light:SetShadowmapSize(4096,4096)
	light:SetShadowBias(0.008)
	light:SetShadowOrthoScale(10.4)
	
	local ne2 = World.AddEntity()
	transform = World.AddComponent_Transform(ne2)
	transform:SetPosition(-15,64.75,-107.25)
	transform:SetRotation(0,-95.5,44.7)
	light = World.AddComponent_Light(ne2)
	light:SetDirectional();
	light:SetColour(0.003,0.083,0.275)
	light:SetAmbient(0.0)
	light:SetBrightness(0.048)
	light:SetDistance(100)
	light:SetCastsShadows(false)
end

function MakeSDFEntity(pos,scale,bmin,bmax,res,fn)
	sdf_entity = World.AddEntity()
	local transform = World.AddComponent_Transform(sdf_entity)
	transform:SetPosition(pos[1],pos[2],pos[3])
	transform:SetScale(scale[1],scale[2],scale[3])
	local sdfModel = World.AddComponent_SDFModel(sdf_entity)
	sdfModel:SetBoundsMin(bmin[1],bmin[2],bmin[3])
	sdfModel:SetBoundsMax(bmax[1],bmax[2],bmax[3])
	sdfModel:SetResolution(res[1],res[2],res[3])
	sdfModel:SetShader(DiffuseShader)
	sdfModel:SetNormalSmoothness(normalSmoothness)
	if(useLuaSampleFn) then
		sdfModel:SetSampleFunction(fn)
	end
	if(meshMode == "Blocky") then 
		sdfModel:SetMeshBlocky()
	elseif meshMode == "SurfaceNet" then
		sdfModel:SetMeshSurfaceNet()
	else
		sdfModel:SetMeshDualContour()
	end
	sdfModel:SetDebugEnabled(debugMeshing)
	sdfModel:Remesh()
	table.insert(sdfEntities,sdf_entity)
end

function SDFTest.Init()
	Graphics.SetClearColour(0.15,0.26,0.4)
	MakeSunEntity()
	
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

local keepRemeshing = false
local meshAll = false
local windowOpen = true
local lastRemeshed = 1
local currentTime = 0
local remeshStart = 0

function SDFTest.Tick(deltaTime)	
	DebugGui.BeginWindow(windowOpen, "SDF Test")
		keepRemeshing = DebugGui.Checkbox("Build every frame", keepRemeshing)
		if(DebugGui.Button("Remesh all")) then 
			if(meshMode ~= "Blocky" and meshMode ~= "SurfaceNet" and meshMode ~= "DualContour") then
				meshMode = "Blocky"
			end
			for m=1, #sdfEntities do
				local model = World.GetComponent_SDFModel(sdfEntities[m])
				model:SetNormalSmoothness(normalSmoothness)
				if(meshMode == "Blocky") then 
					model:SetMeshBlocky()
				elseif meshMode == "SurfaceNet" then
					model:SetMeshSurfaceNet()
				else
					model:SetMeshDualContour()
				end
				model:Remesh()
			end
		end
		meshMode = DebugGui.TextInput("Mesh Mode",meshMode,20)
		normalSmoothness = DebugGui.DragFloat("Normal Smoothness",normalSmoothness,0,0,100)
		
		remeshPerFrame = DebugGui.DragFloat("Remesh per frame", remeshPerFrame, 1, 0, 64)
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
	DebugGui.EndWindow()
	
	currentTime = currentTime + deltaTime
	if keepRemeshing then
		if(lastRemeshed >= #sdfEntities) then 
			lastRemeshed = 1
			a = a + (currentTime - remeshStart) / remeshPerFrame
			remeshStart = currentTime
		end
		local firstMesh = lastRemeshed
		local lastMesh = math.min(#sdfEntities,lastRemeshed+remeshPerFrame)
		if meshAll then
			firstMesh = 1
			lastMesh = #sdfEntities
		end
		for m=firstMesh, lastMesh do
			local model = World.GetComponent_SDFModel(sdfEntities[m])
			model:Remesh()
			lastRemeshed = m
		end
	end
end 

return SDFTest