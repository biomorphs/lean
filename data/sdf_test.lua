SDFTest = {}

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
	s.distance = OpUnion(Sphere({x,y-0.1,z}, 0.75),Plane({x,y,z}, {0.0,1.0,0.0}, 1.0))
	s.material = 10
end

function SDFTest.Init()
	local entity = World.AddEntity()
	local transform = World.AddComponent_Transform(entity)
	transform:SetScale(4.0,4.0,4.0)
	local sdfModel = World.AddComponent_SDFModel(entity)
	sdfModel:SetResolution(17,17,17)
	--sdfModel:SetSampleFunction(TestSampleFn)
end

function SDFTest.Tick(deltaTime)
end 

return SDFTest