CreatureTest = {}

local SDFDiffuseShader = Graphics.LoadShader("sdf_diffuse", "sdf_model.vs", "sdf_model_diffuse.fs")
local SDFShadowShader = Graphics.LoadShader("sdf_shadows", "sdf_shadow.vs", "sdf_shadow.fs")
local ModelDiffuseShader = Graphics.LoadShader("model_diffuse", "simplediffuse.vs", "simplediffuse.fs")
local ModelShadowShader = Graphics.LoadShader("model_shadow", "simpleshadow.vs", "simpleshadow.fs");
Graphics.SetShadowShader(SDFDiffuseShader, SDFShadowShader)
Graphics.SetShadowShader(ModelDiffuseShader, ModelShadowShader)

local SphereModel = Graphics.LoadModel("sphere_low.fbx")
local FloorTexture = Graphics.LoadTexture("grass01.jpg")

function MakeSunEntity()
	local newEntity = World.AddEntity()
	local transform = World.AddComponent_Transform(newEntity)
	transform:SetPosition(91.25,236.25,148.5)
	transform:SetRotation(38.7,21.1,-15.4)
	local light = World.AddComponent_Light(newEntity)
	light:SetDirectional();
	light:SetColour(1,1,1)
	light:SetAmbient(0.2)
	light:SetBrightness(0.267)
	light:SetDistance(400)
	light:SetCastsShadows(true)
	light:SetShadowmapSize(4096,4096)
	light:SetShadowBias(0.001)
	light:SetShadowOrthoScale(200)
	
	newEntity = World.AddEntity()
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

function PlaneSDF( p, n, h )
  return Vec3Dot(p,n) + h;
end

function FloorPlaneSDF(x,y,z)
	local d = PlaneSDF({x,y,z},{0.0,1.0,0.0}, 0.0)
	return d, 0
end

function MakeFloorEntity()
	sdf_entity = World.AddEntity()
	local transform = World.AddComponent_Transform(sdf_entity)
	transform:SetPosition(0,0,0)
	transform:SetScale(1,1,1)
	
	local sdfModel = World.AddComponent_SDFModel(sdf_entity)
	sdfModel:SetBoundsMin(-1000,-2,-1000)
	sdfModel:SetBoundsMax(1000,2,1000)
	sdfModel:SetResolution(8,8,8)
	sdfModel:SetShader(SDFDiffuseShader)
	sdfModel:SetNormalSmoothness(0)
	sdfModel:SetSampleFunction(FloorPlaneSDF)
	sdfModel:SetMeshBlocky()
	sdfModel:SetDiffuseTexture(FloorTexture)
	sdfModel:Remesh()
	
	local physics = World.AddComponent_Physics(sdf_entity)
	physics:SetStatic(true)
	physics:AddPlaneCollider(vec3.new(0.0,1.0,0.0),vec3.new(0.0,0.0,0.0))
	physics:Rebuild()
end

function MakeSphereEntity(p, radius)
	e = World.AddEntity()
	local transform = World.AddComponent_Transform(e)
	transform:SetPosition(p[1],p[2],p[3])
	transform:SetScale(radius,radius,radius)
	
	local newModel = World.AddComponent_Model(e)
	newModel:SetModel(SphereModel)
	newModel:SetShader(ModelDiffuseShader)
	
	local physics = World.AddComponent_Physics(e)
	physics:SetStatic(false)
	physics:AddSphereCollider(vec3.new(0.0,0.0,0.0),radius)
	physics:Rebuild()
end

function CreatureTest.Init()	
	Graphics.SetClearColour(0.3,0.55,0.8)
	
	MakeSunEntity()
	MakeFloorEntity()
	
	for x=1,5000 do 
		MakeSphereEntity({math.random(0,100),1 + math.random(0,100), math.random(0,100)},math.random(1,10)/3.0)
	end
	
end

function CreatureTest.Tick(deltaTime)
end

	
return CreatureTest