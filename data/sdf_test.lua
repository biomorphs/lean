SDFTest = {}

local DiffuseShader = Graphics.LoadShader("sdf_diffuse", "sdf_model.vs", "sdf_model_diffuse.fs")
local ShadowShader = Graphics.LoadShader("sdf_shadows", "sdf_shadow.vs", "sdf_shadow.fs")

function Vec3Length(v)
	return math.sqrt((v[1] * v[1]) + (v[2] * v[2]) + (v[3] * v[3]));
end

function Vec3LengthSq(v)
	return((v[1] * v[1]) + (v[2] * v[2]) + (v[3] * v[3]));
end

function Vec3Dot(v1,v2)
	return (v1[1] * v2[1]) + (v1[2] * v2[2]) + (v1[3] * v2[3])
end

function Vec3Min(v1,v2)
	return (math.min(v1[1],v2[1], math.min(v1[2],v2[2]), math.min(v1[3],v2[3])))
end

function Plane( p, n, h )
  return Vec3Dot(p,n) + h;
end

function Sphere(p, radius)
	return Vec3LengthSq(p)-(radius*radius);
end

function OpUnion( d1, d2 ) 
	return math.min(d1,d2)
end

function TestSampleFn(x,y,z,s)
	s.distance = OpUnion(Sphere({x,y-0.1,z}, 0.8),Plane({x,y,z}, {0.0,1.0,0.0}, 1.0))
	s.distance = OpUnion(Sphere({x-1,y-0.1,z}, 0.6),s.distance)
	s.distance = OpUnion(Sphere({x-1.8,y-0.1,z}, 0.4),s.distance)
	s.distance = OpUnion(Sphere({x-2.4,y-0.1,z}, 0.25),s.distance)
	s.material = 10
end

function MakeSunEntity()
	local newEntity = World.AddEntity()
	local transform = World.AddComponent_Transform(newEntity)
	transform:SetPosition(20,20,20)
	
	local light = World.AddComponent_Light(newEntity)
	light:SetPointLight();
	light:SetColour(0.93, 0.9, 0.9)
	light:SetAmbient(0.05)
	light:SetBrightness(1.0)
	light:SetDistance(100)
	light:SetAttenuation(2.5)
	light:SetCastsShadows(true)
	light:SetShadowmapSize(2048,2048)
	light:SetShadowBias(1.0)
end

function SDFTest.Init()
	Graphics.SetShadowShader(DiffuseShader, ShadowShader)
	MakeSunEntity()
	local entity = World.AddEntity()
	local transform = World.AddComponent_Transform(entity)
	transform:SetScale(4.0,4.0,4.0)
	local sdfModel = World.AddComponent_SDFModel(entity)
	sdfModel:SetBoundsMin(-4,-1,-4)
	sdfModel:SetBoundsMax(4,1,4)
	sdfModel:SetResolution(64,32,64)
	sdfModel:SetShader(DiffuseShader)
	sdfModel:SetSampleFunction(TestSampleFn)
end

function SDFTest.Tick(deltaTime)
end 

return SDFTest