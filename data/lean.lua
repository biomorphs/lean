-- lots of misc utility stuff

-- Entity helpers
function MakeCameraEntity(name, pos, rot)
	e = World.AddEntity()
	local newTags = World.AddComponent_Tags(e)
	newTags:AddTag(Tag.new(name))
	local transform = World.AddComponent_Transform(e)
	transform:SetPosition(pos[1],pos[2],pos[3])
	transform:SetRotation(rot[1],rot[2],rot[3])
	local camera = World.AddComponent_Camera(e)
	return e
end

function MakeModelEntity(name,pos,rot,scale,model,shader)
	local newEntity = World.AddEntity()
	local newTags = World.AddComponent_Tags(e)
	newTags:AddTag(Tag.new(name))
	local transform = World.AddComponent_Transform(newEntity)
	transform:SetPosition(pos[1],pos[2],pos[3])
	transform:SetRotation(rot[1],rot[2],rot[3])
	transform:SetScale(scale[1],scale[2],scale[3])
	local newModel = World.AddComponent_Model(newEntity)
	newModel:SetModel(model)
	newModel:SetShader(shader)
	return newEntity
end

function MakePointLightEntity(name,pos,radius,colour,ambient,brightness,castShadows,shadowMapSize)
	local newEntity = World.AddEntity()
	local newTags = World.AddComponent_Tags(e)
	newTags:AddTag(Tag.new(name))
	local transform = World.AddComponent_Transform(newEntity)
	transform:SetPosition(pos[1],pos[2],pos[3])
	local light = World.AddComponent_Light(newEntity)
	light:SetPointLight();
	light:SetColour(colour[1],colour[2],colour[3])
	light:SetAmbient(ambient or 0)
	light:SetDistance(radius)
	light:SetAttenuation(2.6)
	light:SetCastsShadows(castShadows or false)
	light:SetShadowmapSize(shadowMapSize or 256,shadowMapSize or 256)
	light:SetShadowBias(2.0)
	light:SetBrightness(brightness or 1.0)
	return newEntity
end

-- Engine stuff 

function MakeTag(str)
	return Tag.new(str)
end

-- Randomisation
function RandomFloat(minv,maxv) 
	return minv + (math.random() * (maxv-minv))
end

-- Vector math

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

-- Distance functions

function PlaneSDF( p, n, h )
  return Vec3Dot(p,n) + h;
end

function SphereSDF(p, radius)
	return Vec3Length(p)-radius;
end

function TorusSDF( p, t ) -- pos(3), torus wid/leng?(2)
  local q = { Vec2Length({p[1],p[3]})-t[1], p[2] }
  return Vec2Length(q)-t[2];
end

function TriPrismSDF( p, h )	-- pos, w/h
  local q = { math.abs(p[1]), math.abs(p[2]), math.abs(p[3]) };
  return math.max(q[3]-h[2],math.max(q[1]*0.866025+p[2]*0.5,-p[2])-h[1]*0.5);
end

function OpUnionSDF( d1, d2 ) 
	return math.min(d1,d2)
end