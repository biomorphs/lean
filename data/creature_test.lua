CreatureTest = {}

local SDFDiffuseShader = Graphics.LoadShader("sdf_diffuse", "sdf_model.vs", "sdf_model_diffuse.fs")
local SDFShadowShader = Graphics.LoadShader("sdf_shadows", "sdf_shadow.vs", "sdf_shadow.fs")
local ModelDiffuseShader = Graphics.LoadShader("model_diffuse", "simplediffuse.vs", "simplediffuse.fs")
local ModelShadowShader = Graphics.LoadShader("model_shadow", "simpleshadow.vs", "simpleshadow.fs");
Graphics.SetShadowShader(SDFDiffuseShader, SDFShadowShader)
Graphics.SetShadowShader(ModelDiffuseShader, ModelShadowShader)

local SphereModel = Graphics.LoadModel("sphere_low.fbx")
local TreeModel = Graphics.LoadModel("trees/tree_1.fbx")
local DeadTreeModel = Graphics.LoadModel("trees/DeadTree_1.fbx")
local BunnyModel = Graphics.LoadModel("bunny/bunny_superlow.obj")
local GraveModel = Graphics.LoadModel("kenney_graveyardkit_3/gravestoneCross.fbx")

local c_floorSize = {768,768}
local g_simDeltaTime = 0.016
local g_simTotalPlants = 0
local g_simTotalDeadPlants = 0
local g_simTotalBunnies = 0
local g_simTotalDeadBunnies = 0

function MakeSunEntity()
	local newEntity = World.AddEntity()
	local transform = World.AddComponent_Transform(newEntity)
	transform:SetPosition(48.5,220.5,134)
	transform:SetRotation(38.7,21.1,-18.4)
	local light = World.AddComponent_Light(newEntity)
	light:SetDirectional();
	light:SetColour(0.917,0.788,0.607)
	light:SetAmbient(0.2)
	light:SetBrightness(0.267)
	light:SetDistance(607)
	light:SetCastsShadows(true)
	light:SetShadowmapSize(8192,8192)
	light:SetShadowBias(0.001)
	light:SetShadowOrthoScale(700)
end

function Vec3Length(v)
	return math.sqrt((v[1] * v[1]) + (v[2] * v[2]) + (v[3] * v[3]));
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
	sdfModel:SetBoundsMin(-c_floorSize[1],-1,-c_floorSize[2])
	sdfModel:SetBoundsMax(c_floorSize[1],1,c_floorSize[2])
	sdfModel:SetResolution(8,4,8)
	sdfModel:SetShader(SDFDiffuseShader)
	sdfModel:SetNormalSmoothness(0)
	sdfModel:SetSampleFunction(FloorPlaneSDF)
	sdfModel:SetMeshBlocky()
	sdfModel:Remesh()
end

function MakePlant(model, pos, scale, stateBehaviours)
	local newEntity = World.AddEntity()
	local transform = World.AddComponent_Transform(newEntity)
	transform:SetPosition(pos[1],pos[2],pos[3])
	transform:SetScale(scale[1],scale[2],scale[3])

	local newModel = World.AddComponent_Model(newEntity)
	newModel:SetModel(model)
	newModel:SetShader(ModelDiffuseShader)
	
	local newCreature = World.AddComponent_Creature(newEntity)
	newCreature:SetVisionRadius(-1)	-- plants can't see!
	newCreature:SetEnergy(math.random(100,200))
	
	for state,behaviours in pairs(stateBehaviours) do 
		for b=1,#behaviours do
			newCreature:AddBehaviour(state, behaviours[b])
		end
	end
	newCreature:SetState("idle")
	g_simTotalPlants = g_simTotalPlants + 1
end

function MakeCreature(model, pos, scale, stateBehaviours)
	local newEntity = World.AddEntity()
	local transform = World.AddComponent_Transform(newEntity)
	transform:SetPosition(pos[1],pos[2],pos[3])
	transform:SetScale(scale[1],scale[2],scale[3])

	local newModel = World.AddComponent_Model(newEntity)
	newModel:SetModel(model)
	newModel:SetShader(ModelDiffuseShader)
	
	local newCreature = World.AddComponent_Creature(newEntity)
	newCreature:SetVisionRadius(math.random(128,256))
	newCreature:SetMoveSpeed(math.random(32,128))
	newCreature:SetHungerThreshold(math.random(30,70))
	newCreature:SetEnergy(math.random(80,120))
	newCreature:SetFullThreshold(math.random(70,150))
	newCreature:SetMovementCost(math.random(1,5))
	newCreature:SetEatingSpeed(math.random(10,50))
	newCreature:SetWanderDistance(math.random(64,128))
	
	for state,behaviours in pairs(stateBehaviours) do 
		for b=1,#behaviours do
			newCreature:AddBehaviour(state, behaviours[b])
		end
	end
	newCreature:SetState("idle")
	g_simTotalBunnies = g_simTotalBunnies + 1
end

function MoveToTarget(owner,creature)
	local transform = World.GetComponent_Transform(owner)
	local p = transform:GetPosition()
	local t = creature:GetMoveTarget()
	local d = {t.x - p.x, t.y - p.y, t.z - p.z}
	local l = Vec3Length(d)
	local speed = creature:GetMoveSpeed()
	if(l < creature:GetMoveSpeed() * g_simDeltaTime) then 	-- close to target!
		creature:SetState("idle")
		return false
	end
	d = {d[1]/l,d[2]/l,d[3]/l}	-- normalise dir
	p.x = p.x + d[1] * speed * g_simDeltaTime
	p.y = p.y + d[2] * speed * g_simDeltaTime
	p.z = p.z + d[3] * speed * g_simDeltaTime
	transform:SetPosition(p.x, p.y, p.z)
	Graphics.DebugDrawLine(p.x,1.0,p.z,t.x,1.0,t.z,1.0,1.0,0.0,1.0,1.0,1.0,0.0,1.0)
	creature:SetEnergy(creature:GetEnergy() - creature:GetMovementCost() * g_simDeltaTime)
	return true
end

function RandomWander(owner,creature)
	local transform = World.GetComponent_Transform(owner)
	local p = transform:GetPosition()
	local dir = {-1.0 + math.random(0,100)*0.02, 0, -1.0 + math.random(0,100)*0.02}
	local dl = Vec3Length(dir)
	dir = {dir[1]/dl,0,dir[3]/dl}	-- normalise
	p.x = p.x + dir[1] * creature:GetWanderDistance()
	p.y = 0.0
	p.z = p.z + dir[3] * creature:GetWanderDistance()
	if(p.x < (-c_floorSize[1]/2)) then
		p.x = -c_floorSize[1]/2
	end
	if(p.x>(c_floorSize[1]/2)) then
		p.x = c_floorSize[1]/2
	end
	if(p.z < (-c_floorSize[2]/2)) then
		p.z = -c_floorSize[2]/2
	end
	if(p.z>(c_floorSize[2]/2)) then
		p.z = c_floorSize[2]/2
	end
	creature:SetMoveTarget(p)
	creature:SetState("moveto")
	return false
end

function EatFood(owner,creature)
	local target = creature:GetFoodTarget()
	local targetCreature = World.GetComponent_Creature(target)		-- should food source be a different component? or are tags enough?
	if(targetCreature ~= nil) then
		if(targetCreature:GetEnergy() <= 0) then 
			creature:SetState("idle")
			return false
		end
		local foodAmount = creature:GetEatingSpeed() * g_simDeltaTime
		local e = creature:GetEnergy()
		creature:SetEnergy(e + foodAmount)
		targetCreature:SetEnergy(targetCreature:GetEnergy() - foodAmount)	-- do damage?
		if(e > creature:GetFullThreshold()) then 
			creature:SetState("idle")
			return false
		end
	end
	return true
end

function FindFood(owner,creature)
	if(creature:GetEnergy() <= creature:GetHungerThreshold()) then
		local mytransform = World.GetComponent_Transform(owner)
		local mypos = mytransform:GetPosition()
		local closestDist = 100000
		local closestVisible = -1
		
		-- find the closest food source
		for visible = 1, #creature:GetVisibleEntities() do
			handle = creature:GetVisibleEntities()[visible]
			local transform = World.GetComponent_Transform(handle)
			local c = World.GetComponent_Creature(handle)
			if(c:GetVisionRadius() <= 0 and c:GetEnergy() > 0) then 
				local foodpos = transform:GetPosition()
				local d = {mypos.x-foodpos.x,mypos.y-foodpos.y,mypos.z-foodpos.z}
				local l = Vec3Length(d)
				if(l<closestDist) then
					closestDist = l
					closestVisible = visible
				end
			end
		end
		
		if(closestDist < creature:GetMoveSpeed() * 0.1) then
			creature:SetFoodTarget(creature:GetVisibleEntities()[closestVisible])
			creature:SetState("eat")
			return false
		end
		
		if(closestVisible > 0) then
			handle = creature:GetVisibleEntities()[closestVisible]
			local transform = World.GetComponent_Transform(handle)
		 	creature:SetMoveTarget(transform:GetPosition())
			creature:SetState("moveto")
			return false
		end
	end
	return true
end

function DieAtZeroEnergy(owner,creature)
	if(creature:GetEnergy() <= 0) then
		creature:SetState("dying")
		return false
	end
	return true
end

function Photosynthesize(owner,creature)
	creature:SetEnergy(creature:GetEnergy() + 1.0 * g_simDeltaTime)
	return true
end

function KillCreature(owner,creature)
	local m = World.GetComponent_Model(owner)
	local t = World.GetComponent_Transform(owner)
	if( m ~= nil and t ~= nil ) then 
		if(creature:GetVisionRadius() <= 0)	then -- plant?
			m:SetModel(DeadTreeModel)
			t:SetScale(0.05,0.05,0.05)
			g_simTotalDeadPlants = g_simTotalDeadPlants + 1
		else
			m:SetModel(GraveModel)
			t:SetScale(2,2,2)
			g_simTotalDeadBunnies = g_simTotalDeadBunnies + 1
		end
	end
	creature:SetState("dead")
	return false
end

function CreatureTest.Init()	
	MakeSunEntity()
	MakeFloorEntity()
	
	-- behaviours are global
	Creatures.AddBehaviour("move_to_target", MoveToTarget)
	Creatures.AddBehaviour("random_wander", RandomWander)
	Creatures.AddBehaviour("find_food", FindFood)
	Creatures.AddBehaviour("eat_target_food", EatFood)
	Creatures.AddBehaviour("die_at_zero_energy", DieAtZeroEnergy)
	Creatures.AddBehaviour("kill_creature", KillCreature)
	Creatures.AddBehaviour("photosynthesize", Photosynthesize)
	
	-- creatures have states which contain behaviours
	local plantBehaviour = {}
	plantBehaviour["idle"] = { "photosynthesize", "die_at_zero_energy" }
	plantBehaviour["dying"] = { "kill_creature" }
	
	-- add some plants 
	for i=1,1000 do 
		local p = {math.random(-c_floorSize[1]/2,c_floorSize[1]/2), 0, math.random(-c_floorSize[2]/2,c_floorSize[2]/2)}
		MakePlant(TreeModel, p, {0.05,0.05,0.05}, plantBehaviour)
	end
	
	-- creatures have states which contain behaviours
	local bunnyBehaviour = {}
	bunnyBehaviour["idle"] = { "find_food", "random_wander", "die_at_zero_energy" }
	bunnyBehaviour["moveto"] = { "move_to_target", "die_at_zero_energy" }
	bunnyBehaviour["eat"] = { "eat_target_food", "die_at_zero_energy" }
	bunnyBehaviour["dying"] = { "kill_creature" }
	
	-- add some bunnies
	for i=1,500 do
		local p = {math.random(-c_floorSize[1]/2,c_floorSize[1]/2), 0, math.random(-c_floorSize[2]/2,c_floorSize[2]/2)}
		MakeCreature(BunnyModel, p, {8,8,8}, bunnyBehaviour)
	end
end

function CreatureTest.Tick(deltaTime)
	DebugGui.BeginWindow(true,"Simulation")
	g_simDeltaTime = DebugGui.DragFloat("Tick time", g_simDeltaTime, 0, 0.0, 10.0)
	DebugGui.DragFloat("Total Plants", g_simTotalPlants, 0.0, 0.0, 10.0)
	DebugGui.DragFloat("Living Plants", g_simTotalPlants - g_simTotalDeadPlants, 0.0, 0.0, 10.0)
	DebugGui.DragFloat("Total Bunnies", g_simTotalBunnies, 0.0, 0.0, 10.0)
	DebugGui.DragFloat("Living Bunnies", g_simTotalBunnies - g_simTotalDeadBunnies, 0.0, 0.0, 10.0)
	DebugGui.EndWindow()
end

	
return CreatureTest