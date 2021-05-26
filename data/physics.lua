CreatureTest = {}

local WSM = 0.1

local SDFDiffuseShader = Graphics.LoadShader("sdf_diffuse", "sdf_model.vs", "sdf_model_diffuse.fs")
local SDFShadowShader = Graphics.LoadShader("sdf_shadows", "sdf_shadow.vs", "sdf_shadow.fs")
local ModelDiffuseShader = Graphics.LoadShader("model_diffuse", "simplediffuse.vs", "simplediffuse.fs")
local ModelShadowShader = Graphics.LoadShader("model_shadow", "simpleshadow.vs", "simpleshadow.fs");
local ModelNoLighting = Graphics.LoadShader("model_nolight",  "basic.vs", "basic.fs")
Graphics.SetShadowShader(SDFDiffuseShader, SDFShadowShader)
Graphics.SetShadowShader(ModelDiffuseShader, ModelShadowShader)

local CubeModel = Graphics.LoadModel("cube2.fbx")
local SphereModel = Graphics.LoadModel("sphere_low.fbx")
local FloorTexture = Graphics.LoadTexture("white.bmp")

-- list of entity id
local MaterialEntities = {}
local RandomTextures = {"sponza_roof_diff.png", "sponza_floor_a_diff.png", "sponza_ceiling_a_diff.png"}

function MakeMaterials()
	-- make a bunch of random materials
	for m=0,50 do 
		local e = World.AddEntity()
		local t = World.AddComponent_Tags(e)
		t:AddTag(Tag.new("Material Proxy"))
		local m = World.AddComponent_Material(e)
		m:SetVec4("MeshDiffuseOpacity", vec4.new(math.random(0,255)/125.0,math.random(0,255)/125.0,math.random(0,255)/125.0,1.0))
		m:SetSampler("DiffuseTexture", Graphics.LoadTexture(RandomTextures[math.random(1, #RandomTextures)]))
		table.insert(MaterialEntities, e)
	end
end

function RandomMaterial()
	local i = math.random(1, #MaterialEntities)
	return MaterialEntities[i]
end

function MakeSunEntity()
	local newEntity = World.AddEntity()
	local newTags = World.AddComponent_Tags(newEntity)
	newTags:AddTag(Tag.new("Sunlight"))
	
	local transform = World.AddComponent_Transform(newEntity)
	transform:SetPosition(1151.25 * WSM,2052.5 * WSM,1473.75 * WSM)
	transform:SetRotation(30,-40.2,0)
	local light = World.AddComponent_Light(newEntity)
	light:SetDirectional();
	light:SetColour(1,1,1)
	light:SetAmbient(0.05)
	light:SetBrightness(0.267)
	light:SetDistance(3000 * WSM)
	light:SetCastsShadows(true)
	light:SetShadowmapSize(8192,8192)
	light:SetShadowBias(0.001)
	light:SetShadowOrthoScale(2200 * WSM)
	
	newEntity = World.AddEntity()
	local newTags = World.AddComponent_Tags(newEntity)
	newTags:AddTag(Tag.new("Backlight"))
	
	transform = World.AddComponent_Transform(newEntity)
	transform:SetRotation(-56,-17,1.2)
	local light = World.AddComponent_Light(newEntity)
	light:SetDirectional();
	light:SetColour(1,1,1)
	light:SetAmbient(0.0)
	light:SetBrightness(0.01)
end

function Vec3Dot(v1,v2)
	return (v1[1] * v2[1]) + (v1[2] * v2[2]) + (v1[3] * v2[3])
end

function MakeFloorEntity()
	e = World.AddEntity()
	local newTags = World.AddComponent_Tags(e)
	newTags:AddTag(Tag.new("Floor"))
	
	local transform = World.AddComponent_Transform(e)
	transform:SetPosition(0,-1,0)
	transform:SetScale(2000 * WSM,1.0,2000 * WSM)
	
	local material = World.AddComponent_Material(e)
	material:SetVec4("MeshDiffuseOpacity", vec4.new(1.0,1.0,1.0,1.0))
	
	local newModel = World.AddComponent_Model(e)
	newModel:SetModel(CubeModel)
	newModel:SetShader(ModelDiffuseShader)
	newModel:SetMaterialEntity(e)
	
	local physics = World.AddComponent_Physics(e)
	physics:SetStatic(true)
	physics:AddBoxCollider(vec3.new(0.0,0.0,0.0),vec3.new(2000 * WSM,1.0,2000 * WSM))
	physics:AddPlaneCollider(vec3.new(1.0,0.0,0.0),vec3.new(-1000.0 * WSM,0.0,0.0))
	physics:AddPlaneCollider(vec3.new(-1.0,0.0,0.0),vec3.new(1000.0 * WSM,0.0,0.0))
	physics:AddPlaneCollider(vec3.new(0.0,0.0,1.0),vec3.new(0.0,0.0,-1000.0 * WSM))
	physics:AddPlaneCollider(vec3.new(0.0,0.0,-1.0),vec3.new(0.0,0.0,1000.0 * WSM))
	physics:Rebuild()
end

function MakeSphereEntity(p, radius, dynamic, matEntity)
	e = World.AddEntity()
	local newTags = World.AddComponent_Tags(e)
	newTags:AddTag(Tag.new("Sphere"))
	
	local transform = World.AddComponent_Transform(e)
	transform:SetPosition(p[1],p[2],p[3])
	transform:SetScale(radius,radius,radius)
	
	local newModel = World.AddComponent_Model(e)
	newModel:SetModel(SphereModel)
	newModel:SetShader(ModelDiffuseShader)
	newModel:SetMaterialEntity(matEntity)
	
	local physics = World.AddComponent_Physics(e)
	physics:SetStatic(not dynamic)
	physics:AddSphereCollider(vec3.new(0.0,0.0,0.0),radius)
	physics:Rebuild()
	
	if(math.random(0,1000)<5) then 
		newTags:AddTag(Tag.new("Lit"))
		local light = World.AddComponent_Light(e)
		light:SetPointLight();
		if(math.random(0,1000)<150) then 
			light:SetColour(0,math.random(5,20)/100.0,math.random(90,100)/100.0)
			light:SetAmbient(0.01)
			light:SetBrightness(10.0)
			light:SetDistance(256 * WSM)
			light:SetAttenuation(32.0)
			light:SetCastsShadows(false)
			light:SetShadowmapSize(256,256)
			light:SetShadowBias(4.0)
			newTags:AddTag(Tag.new("Blue"))
		else
			light:SetColour(math.random(90,100)/100.0,math.random(5,30)/100.0,0)
			light:SetAmbient(0.04)
			light:SetBrightness(6.0)
			light:SetDistance(48 * WSM)
			light:SetAttenuation(12.0)
			light:SetCastsShadows(false)
		end
		newModel:SetShader(ModelNoLighting)
	end
	
	return e
end

function MakeBoxEntity(p, dims, dynamic, kinematic, matEntity)
	e = World.AddEntity()
	local newTags = World.AddComponent_Tags(e)
	newTags:AddTag(Tag.new("Box"))
	
	local transform = World.AddComponent_Transform(e)
	transform:SetPosition(p[1],p[2],p[3])
	transform:SetScale(dims[1],dims[2],dims[3])
	
	local newModel = World.AddComponent_Model(e)
	newModel:SetModel(CubeModel)
	newModel:SetShader(ModelDiffuseShader)
	newModel:SetMaterialEntity(matEntity)
	
	local physics = World.AddComponent_Physics(e)
	physics:SetStatic(not dynamic)
	physics:SetKinematic(kinematic)
	physics:AddBoxCollider(vec3.new(0.0,0.0,0.0),vec3.new(dims[1],dims[2],dims[3]))
	physics:Rebuild()
	
	if(math.random(0,1000)<5) then 
		newTags:AddTag(Tag.new("Lit"))
		local light = World.AddComponent_Light(e)
		light:SetPointLight();
		light:SetColour(math.random(90,100)/100.0,math.random(5,30)/100.0,0)
		light:SetAmbient(0.05)
		light:SetAttenuation(12.0)
		light:SetBrightness(6.0)
		light:SetDistance(64 * WSM)
		light:SetCastsShadows(false)
		light:SetShadowmapSize(512,512)
		light:SetShadowBias(0.5)
		newModel:SetShader(ModelNoLighting)
	end
	
	return e
end

local Spinner = {}

function CreatureTest.Init()	
	Graphics.SetClearColour(0.3,0.55,0.8)
	
	MakeSunEntity()
	MakeFloorEntity()
	
	MakeMaterials()
	
	Spinner = MakeBoxEntity({0,70 * WSM,128 * WSM}, {150 * WSM,8 * WSM,8 * WSM}, true, true, RandomMaterial())
	World.GetComponent_Physics(Spinner):SetDensity(100.0)
	World.GetComponent_Tags(Spinner):AddTag(Tag.new("Spinner"))
	
	for x=1,5000 do 
		MakeSphereEntity({math.random(0,100) * WSM,200 * WSM + math.random(0,1000) * WSM, math.random(0,100) * WSM},0.5 * WSM + math.random(1,10)/3.0 * WSM, true, RandomMaterial())
		local boxSize = WSM * 4 + math.random(1,10)/1.5 * WSM * 1.5
		MakeBoxEntity({math.random(0,100) * WSM,200 * WSM + math.random(0,1000) * WSM, math.random(0,100) * WSM},{boxSize * WSM * 4,boxSize * WSM * 4,boxSize * WSM * 4}, true, false,  RandomMaterial())
	end
	
	MakeSphereEntity({32*WSM,0,32*WSM},64*WSM,false, RandomMaterial())
	MakeSphereEntity({128*WSM,32.5*WSM,32*WSM},32*WSM,false, RandomMaterial())
	MakeBoxEntity({128*WSM,16.5*WSM,128*WSM},{32*WSM,32*WSM,32*WSM},true,false, RandomMaterial())
	MakeBoxEntity({64*WSM,16.5*WSM,128*WSM},{32*WSM,32*WSM,32*WSM},true,false, RandomMaterial())
	local tallBoy = MakeBoxEntity({0,32.5*WSM,128*WSM},{16*WSM,64*WSM,16*WSM},true,false, RandomMaterial())
	World.GetComponent_Physics(tallBoy):SetDensity(1.0)
	MakeBoxEntity({32*WSM,128*WSM,128*WSM},{64*WSM,8*WSM,8*WSM},true,false, RandomMaterial())
end

function CreatureTest.Tick(deltaTime)
	if(Spinner ~= nil) then 
		local transform = World.GetComponent_Transform(Spinner)
		if(transform ~= nil) then
			transform:RotateEuler(vec3.new(0,-2.5 * deltaTime,0))
		end
	end
end

	
return CreatureTest