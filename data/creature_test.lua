CreatureTest = {}

local SDFDiffuseShader = Graphics.LoadShader("sdf_diffuse", "sdf_model.vs", "sdf_model_diffuse.fs")
local SDFShadowShader = Graphics.LoadShader("sdf_shadows", "sdf_shadow.vs", "sdf_shadow.fs")
local ModelDiffuseShader = Graphics.LoadShader("model_diffuse", "simplediffuse.vs", "simplediffuse.fs")
local ModelShadowShader = Graphics.LoadShader("model_shadow", "simpleshadow.vs", "simpleshadow.fs");
Graphics.SetShadowShader(SDFDiffuseShader, SDFShadowShader)
Graphics.SetShadowShader(ModelDiffuseShader, ModelShadowShader)

local SphereModel = Graphics.LoadModel("sphere_low.fbx")
local TreeModel = Graphics.LoadModel("treefixed.fbx")
local DeadTreeModel = Graphics.LoadModel("trees/DeadTree_1.fbx")
local BunnyModel = Graphics.LoadModel("bunny/bunny_superlow.obj")
local GraveModel = Graphics.LoadModel("kenney_graveyardkit_3/gravestoneCross.fbx")
local MonsterModel = Graphics.LoadModel("udim-monster.fbx")
local FloorTexture = Graphics.LoadTexture("grass01.jpg")

local c_floorSize = {1000,1000}
local g_simLivingPlants = 0
local g_simLivingBunnies = 0

function RandomFloat(minv,maxv) 
	return minv + (math.random() * (maxv-minv))
end

function MakeSunEntity()
	local newEntity = World.AddEntity()
	local transform = World.AddComponent_Transform(newEntity)
	transform:SetPosition(197.75,403.75,279.5)
	transform:SetRotation(38.7,21.1,-18.4)
	local light = World.AddComponent_Light(newEntity)
	light:SetDirectional();
	light:SetColour(0.917,0.788,0.607)
	light:SetAmbient(0.2)
	light:SetBrightness(0.267)
	light:SetDistance(1000)
	light:SetCastsShadows(true)
	light:SetShadowmapSize(4096,4096)
	light:SetShadowBias(0.001)
	light:SetShadowOrthoScale(700)
	
	newEntity = World.AddEntity()
	transform = World.AddComponent_Transform(newEntity)
	transform:SetRotation(-56,-17,1.2)
	local light = World.AddComponent_Light(newEntity)
	light:SetDirectional();
	light:SetColour(1,1,1)
	light:SetAmbient(0.0)
	light:SetBrightness(0.01)
end

function MakePlant(model, pos)
	local newEntity = World.AddEntity()
	local transform = World.AddComponent_Transform(newEntity)
	transform:SetPosition(pos[1],pos[2],pos[3])
	local scale = RandomFloat(4,8)
	transform:SetScale(scale,scale,scale)
	
	local newTags = World.AddComponent_Tags(newEntity)
	newTags:AddTag("plant")

	local newModel = World.AddComponent_Model(newEntity)
	newModel:SetModel(model)
	newModel:SetShader(ModelDiffuseShader)
	
	local newCreature = World.AddComponent_Creature(newEntity)
	newCreature:SetVisionRadius(-1)	-- plants can't see!
	newCreature:SetEnergy(RandomFloat(100,200))
	newCreature:SetMaxAge(RandomFloat(30,60))
	
	local plantBehaviour = {}
	plantBehaviour["idle"] = { "die_at_max_age", "die_at_zero_energy", "photosynthesize", "plant_reproduction" }
	plantBehaviour["dying"] = { "kill_creature" }
	plantBehaviour["dead"] = { "remove_corpse" }
	
	for state,behaviours in pairs(plantBehaviour) do 
		for b=1,#behaviours do
			newCreature:AddBehaviour(state, behaviours[b])
		end
	end
	newCreature:SetState("idle")
	g_simLivingPlants = g_simLivingPlants + 1
end

function MakeBunny()
	local newEntity = World.AddEntity()
	local transform = World.AddComponent_Transform(newEntity)
	local p = {RandomFloat(-c_floorSize[1]/2,c_floorSize[1]/2), 0, RandomFloat(-c_floorSize[2]/2,c_floorSize[2]/2)}
	transform:SetPosition(p[1],p[2],p[3])
	transform:SetPosition(0,0,0)
	local scale = 4 + math.random() * 8.0
	transform:SetScale(scale,scale,scale)

	local newTags = World.AddComponent_Tags(newEntity)
	newTags:AddTag("bunny")

	local newModel = World.AddComponent_Model(newEntity)
	newModel:SetModel(BunnyModel)
	newModel:SetShader(ModelDiffuseShader)
	
	local newCreature = World.AddComponent_Creature(newEntity)
	newCreature:SetVisionRadius(200)
	newCreature:SetMaxVisibleEntities(32)
	newCreature:AddFoodSourceTag("plant")
	newCreature:AddVisionTag("plant")
	newCreature:AddVisionTag("monster")
	newCreature:AddFleeFromTag("monster")
	newCreature:SetState("idle")
	
	newCreature:SetMoveSpeed(RandomFloat(100,250))
	newCreature:SetHungerThreshold(RandomFloat(20,70))
	newCreature:SetEnergy(RandomFloat(80,120))
	newCreature:SetFullThreshold(RandomFloat(100,200))
	newCreature:SetMovementCost(RandomFloat(3,10))
	newCreature:SetEatingSpeed(RandomFloat(20,70))
	newCreature:SetWanderDistance(RandomFloat(32,128))
	newCreature:SetMaxAge(100000)
	
	local bunnyBehaviour = {}
	bunnyBehaviour["idle"] = { "die_at_max_age", "die_at_zero_energy", "flee_enemy", "find_food", "eat_if_hungry", "random_wander" }
	bunnyBehaviour["moveto"] = { "die_at_max_age", "die_at_zero_energy", "move_to_target" }
	bunnyBehaviour["eat"] = { "die_at_max_age", "die_at_zero_energy", "flee_enemy", "eat_target_food" }
	bunnyBehaviour["dying"] = { "kill_creature" }
	for state,behaviours in pairs(bunnyBehaviour) do 
		for b=1,#behaviours do
			newCreature:AddBehaviour(state, behaviours[b])
		end
	end
	
	g_simLivingBunnies = g_simLivingBunnies + 1
end

function MakeMonster()
	local newEntity = World.AddEntity()
	local transform = World.AddComponent_Transform(newEntity)
	local p = {RandomFloat(-c_floorSize[1]/2,c_floorSize[1]/2), 0, RandomFloat(-c_floorSize[2]/2,c_floorSize[2]/2)}
	transform:SetPosition(p[1],p[2],p[3])
	transform:SetScale(16,16,16)

	local newTags = World.AddComponent_Tags(newEntity)
	newTags:AddTag("monster")

	local newModel = World.AddComponent_Model(newEntity)
	newModel:SetModel(MonsterModel)
	newModel:SetShader(ModelDiffuseShader)
	
	local newCreature = World.AddComponent_Creature(newEntity)
	newCreature:SetVisionRadius(300)
	newCreature:AddVisionTag("bunny")
	newCreature:SetMaxVisibleEntities(32)
	newCreature:SetMoveSpeed(200)
	newCreature:SetHungerThreshold(30000000)
	newCreature:SetEnergy(500)
	newCreature:SetFullThreshold(500)
	newCreature:SetMovementCost(10)
	newCreature:SetEatingSpeed(10000)
	newCreature:SetWanderDistance(RandomFloat(64,128))
	newCreature:SetMaxAge(10000000000)
	newCreature:AddFoodSourceTag("bunny")
	newCreature:SetState("idle")
	
	local monsterBehaviour = {}
	monsterBehaviour["idle"] = { "find_food", "eat_if_hungry", "random_wander", "die_at_zero_energy" }
	monsterBehaviour["moveto"] = { "move_to_target", "die_at_zero_energy" }
	monsterBehaviour["eat"] = { "eat_target_food", "die_at_zero_energy" }
	monsterBehaviour["dying"] = { "kill_creature" }
	for state,behaviours in pairs(monsterBehaviour) do 
		for b=1,#behaviours do
			newCreature:AddBehaviour(state, behaviours[b])
		end
	end
end

function Vec3Length(v)
	return math.sqrt((v[1] * v[1]) + (v[2] * v[2]) + (v[3] * v[3]));
end

function Vec3Dot(v1,v2)
	return (v1[1] * v2[1]) + (v1[2] * v2[2]) + (v1[3] * v2[3])
end

function Vec3Normalize(v)
	local l = Vec3Length({v.x,v.y,v.z})
	local out = v
	out.x = v.x / l
	out.y = v.y / l
	out.z = v.z / l
	return out
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
	sdfModel:SetBoundsMin(-c_floorSize[1],-1,-c_floorSize[2])
	sdfModel:SetBoundsMax(c_floorSize[1],1,c_floorSize[2])
	sdfModel:SetResolution(8,4,8)
	sdfModel:SetShader(SDFDiffuseShader)
	sdfModel:SetNormalSmoothness(0)
	sdfModel:SetSampleFunction(FloorPlaneSDF)
	sdfModel:SetMeshBlocky()
	sdfModel:SetDiffuseTexture(FloorTexture)
	sdfModel:Remesh()
end

function MoveCreatureTo(owner,creature,pos)
	creature:GetBlackboard():SetVector("MoveToTarget", pos.x, pos.y, pos.z)
	creature:SetState("moveto")
	
	-- rotate to face target
	local t = World.GetComponent_Transform(owner)
	if(t ~= nil) then 
		local mypos = t:GetPosition()
		local dir = pos
		dir.x = pos.x - mypos.x
		dir.y = 0
		dir.z = pos.z - mypos.z
		dir = Vec3Normalize(dir)
		local angle = math.acos(Vec3Dot({dir.x,dir.y,dir.z},{1,0,0}))
		local rot = t:GetRotationDegrees()
		rot.y = math.deg(angle) + 90
		t:SetRotation(rot.x,rot.y,rot.z)
	end
end

function MoveToTarget(owner,creature,delta)
	local transform = World.GetComponent_Transform(owner)
	local p = transform:GetPosition()
	local t = creature:GetBlackboard():GetVector("MoveToTarget")
	local d = {t.x - p.x, t.y - p.y, t.z - p.z}
	local l = Vec3Length(d)
	local speed = creature:GetMoveSpeed()
	if(l < creature:GetMoveSpeed() * delta) then 	-- close to target!
		creature:SetState("idle")
		return false
	end
	d = {d[1]/l,d[2]/l,d[3]/l}	-- normalise dir
	p.x = p.x + d[1] * speed * delta
	p.y = p.y + d[2] * speed * delta
	p.z = p.z + d[3] * speed * delta
	transform:SetPosition(p.x, p.y, p.z)
	Graphics.DebugDrawLine(p.x,1.0,p.z,t.x,1.0,t.z,1.0,1.0,0.0,1.0,1.0,1.0,0.0,1.0)
	creature:SetEnergy(creature:GetEnergy() - creature:GetMovementCost() * delta)
	return true
end

function IsOutOfBounds(p)
	local outOfBounds = false
	if(p[1] < (-c_floorSize[1]/2)) then
		outOfBounds = true
	end
	if(p[1]>(c_floorSize[1]/2)) then
		outOfBounds = true
	end
	if(p[3] < (-c_floorSize[2]/2)) then
		outOfBounds = true
	end
	if(p[3]>(c_floorSize[2]/2)) then
		outOfBounds = true
	end
	return outOfBounds
end

function RandomPointNearPos(pos,maxDistance)
	local originalPos = pos
	local p = pos
	local failsafe = 0
	while(failsafe < 8) do  
		local dir = {-1.0 + math.random() * 2.0, 0, -1.0 + math.random() * 2.0}
		local dl = Vec3Length(dir)
		dir = {dir[1]/dl,0,dir[3]/dl}	-- normalise
		local newpos = {}
		newpos[1] = originalPos.x + dir[1] * maxDistance
		newpos[2] = 0.0
		newpos[3] = originalPos.z + dir[3] * maxDistance
		if(IsOutOfBounds(newpos) == false) then
			p.x = newpos[1]
			p.y = newpos[2]
			p.z = newpos[3]
			return p
		end
		failsafe = failsafe + 1
	end
	
	p.x = (-1.0 + math.random() * 2.0) * c_floorSize[1]/2
	p.z = (-1.0 + math.random() * 2.0) * c_floorSize[2]/2
	return p
end

function RandomWander(owner,creature,delta)
	local transform = World.GetComponent_Transform(owner)
	local p = RandomPointNearPos(transform:GetPosition(), creature:GetWanderDistance())
	MoveCreatureTo(owner,creature,p)
	return false
end

function EatFood(owner,creature,delta)
	if(creature:GetBlackboard():ContainsEntity("ClosestFood") == false) then 
		creature:SetState("idle")
		return false
	end
	
	local target = creature:GetBlackboard():GetEntity("ClosestFood")
	local targetCreature = World.GetComponent_Creature(target)	
	if(targetCreature ~= nil) then
		local foodAmount = creature:GetEatingSpeed() * delta
		if(foodAmount > targetCreature:GetEnergy()) then 
			foodAmount = targetCreature:GetEnergy()
		end
		local e = creature:GetEnergy()
		creature:SetEnergy(e + foodAmount)
		targetCreature:SetEnergy(targetCreature:GetEnergy() - foodAmount)	-- do damage?
		if(e > creature:GetFullThreshold()) then 
			creature:SetState("idle")
			return false
		end
		if(targetCreature:GetEnergy() <= 0) then 
			creature:SetState("idle")
			return false
		end
	else
		creature:SetState("idle")
		return false
	end
	return true
end

function FindFood(owner,creature,delta)
	if(creature:GetEnergy() <= creature:GetHungerThreshold()) then
		local mytransform = World.GetComponent_Transform(owner)
		local mypos = mytransform:GetPosition()
		-- visible entity list is always sorted in order of distance to creature
		-- therefore just grab the first valid food on the list
		for visible = 1, #creature:GetVisibleEntities() do
			handle = creature:GetVisibleEntities()[visible]
			local c = World.GetComponent_Creature(handle)
			local tags = World.GetComponent_Tags(handle)
			if(c ~= nil and tags ~= nil) then 
				local isEdible = false
				if(tags ~= nil) then	
					for mytag = 1, #creature:GetFoodSourceTags() do 
						if(tags:ContainsTag(creature:GetFoodSourceTags()[mytag])) then 
							isEdible = true
						end
					end
				end
				if(isEdible and c:GetEnergy() > 0 and c:GetState() ~= "dead") then 
					creature:GetBlackboard():SetEntity("ClosestFood",handle)
					return true
				end
			end
		end
		-- nothing found
		creature:GetBlackboard():RemoveEntity("ClosestFood")
	end
	return true
end

function EatIfHungry(owner,creature,delta)
	if(creature:GetEnergy() > creature:GetHungerThreshold()) then
		return true
	end
	if(creature:GetBlackboard():ContainsEntity("ClosestFood")) then 
		local mytransform = World.GetComponent_Transform(owner)
		local mypos = mytransform:GetPosition()
		local handle = creature:GetBlackboard():GetEntity("ClosestFood")
		local transform = World.GetComponent_Transform(handle)
		if(transform ~= nil) then 
			local foodpos = transform:GetPosition()
			local d = {mypos.x-foodpos.x,mypos.y-foodpos.y,mypos.z-foodpos.z}
			local l = Vec3Length(d)
			if(l < creature:GetMoveSpeed() * 0.1) then
				creature:SetState("eat")
			else
				local p = transform:GetPosition()
				MoveCreatureTo(owner,creature,p)
			end
			return false
		end
	end
	return true;
end

function DieAtZeroEnergy(owner,creature,delta)
	if(creature:GetEnergy() <= 0) then
		creature:SetState("dying")
		return false
	end
	return true
end

function DieAtMaxAge(owner,creature,delta)
	local currentAge = creature:GetAge()
	if(currentAge > creature:GetMaxAge()) then
		creature:SetState("dying")
		return false
	end
	return true
end

function Photosynthesize(owner,creature,delta)
	creature:SetEnergy(creature:GetEnergy() + 1.0 * delta)
	return true
end

function PlantReproduction(owner,creature,delta)
	if(creature:GetAge() > (creature:GetMaxAge() * 0.5) and creature:GetAge() < (creature:GetMaxAge() * 0.8)) then 
		if(math.random(0,1000) < 1) then
			local mytransform = World.GetComponent_Transform(owner)
			local mypos = mytransform:GetPosition()
			local p = {RandomFloat(mypos.x - 64, mypos.x + 64), 0, RandomFloat(mypos.z - 64, mypos.z + 64)}
			
			MakePlant(TreeModel, p)
		end
	end
	return true
end

function RemoveCorpse(owner,creature,delta)
	if(math.random(1,1000) < 5) then
		World.RemoveEntity(owner)		
	end
end

function FleeEnemy(owner,creature,delta)
	for visible = 1, #creature:GetVisibleEntities() do
		handle = creature:GetVisibleEntities()[visible]
		local tags = World.GetComponent_Tags(handle)
		if(tags ~= nil) then	
			local shouldFlee = false
			for mytag = 1, #creature:GetFleeFromTags() do 
				if(tags:ContainsTag(creature:GetFleeFromTags()[mytag])) then 
					shouldFlee = true
				end
			end
			if(shouldFlee) then 
				local mytransform = World.GetComponent_Transform(owner)
				local p = mytransform:GetPosition()
				local monstertransform = World.GetComponent_Transform(handle)
				local m = monstertransform:GetPosition()
				local dir = {p.x - m.x,0,p.z - m.z}
				local l = Vec3Length(dir)
				dir[1] = dir[1] / l
				dir[3] = dir[3] / l
				p.x = p.x + dir[1] * 32
				p.z = p.z + dir[3] * 32
				MoveCreatureTo(owner, creature,p)
				return false
			end
		end
	end
	return true
end

function KillCreature(owner,creature,delta)
	local m = World.GetComponent_Model(owner)
	local t = World.GetComponent_Transform(owner)
	local tags = World.GetComponent_Tags(owner)
	if( m ~= nil and t ~= nil and tags ~= nil ) then 
		if(tags:ContainsTag("plant"))	then -- plant?
			m:SetModel(DeadTreeModel)
			t:SetScale(0.05,0.05,0.05)
			g_simLivingPlants = g_simLivingPlants - 1
		else
			m:SetModel(GraveModel)
			t:SetScale(1,1,1)
			g_simLivingBunnies = g_simLivingBunnies - 1
		end
	end
	creature:SetState("dead")
	return false
end

function CreatureTest.Init()	
	MakeSunEntity()
	MakeFloorEntity()
	
	-- behaviours are global
	-- Creatures.AddBehaviour("move_to_target", MoveToTarget)
	-- Creatures.AddBehaviour("die_at_max_age", DieAtMaxAge)
	-- Creatures.AddBehaviour("die_at_zero_energy", DieAtZeroEnergy)
	Creatures.AddBehaviour("random_wander", RandomWander)
	Creatures.AddBehaviour("find_food", FindFood)
	Creatures.AddBehaviour("eat_target_food", EatFood)
	Creatures.AddBehaviour("kill_creature", KillCreature)
	--Creatures.AddBehaviour("flee_enemy", FleeEnemy)
	Creatures.AddBehaviour("plant_reproduction", PlantReproduction)
	-- Creatures.AddBehaviour("photosynthesize", Photosynthesize)
	Creatures.AddBehaviour("remove_corpse", RemoveCorpse)
	Creatures.AddBehaviour("eat_if_hungry", EatIfHungry)
	
	for i=1,2000 do
		MakeBunny()
	end
	
	for i=1,4000 do 
		local p = {RandomFloat(-c_floorSize[1]/2,c_floorSize[1]/2), 0, RandomFloat(-c_floorSize[2]/2,c_floorSize[2]/2)}
		MakePlant(TreeModel, p)
	end
	
	for i=1,20 do
		MakeMonster()
	end
end

function CreatureTest.Tick(deltaTime)
	DebugGui.BeginWindow(true,"Simulation")
	DebugGui.DragFloat("Living Plants", g_simLivingPlants, 0.0, 0.0, 10.0)
	DebugGui.DragFloat("Living Bunnies", g_simLivingBunnies, 0.0, 0.0, 10.0)
	DebugGui.EndWindow()
end

	
return CreatureTest