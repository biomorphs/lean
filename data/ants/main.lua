-- todo 
-- rocks should have a separate component
-- (avoids searching all tagged objects)

local collectorSpawnCount = 1
local builderSpawnCount = 1

function SpawnRocks()
	local template = World.GetFirstEntityWithTag(Tag.new("RockSmall"))
	for i=1,1000 do 
		local newEntity = World.CloneEntity(template)
		local newTransform = World.GetComponent_Transform(newEntity)
		local newTags = World.GetComponent_Tags(newEntity)
		newTags:ClearTags()
		newTags:AddTag(Tag.new("RockInstance"))
		if(newTransform ~= nil) then 
			newTransform:SetPosition(-512 + math.random(1024), 2.0, -512 + math.random(1024))
		end
		local newPickup = World.AddComponent_AntPickupComponent(newEntity)
	end
end

function SpawnMushrooms()
	local template = World.GetFirstEntityWithTag(Tag.new("MushroomTemplate"))
	for i=1,200 do 
		local newEntity = World.CloneEntity(template)
		local newTransform = World.GetComponent_Transform(newEntity)
		local newTags = World.GetComponent_Tags(newEntity)
		newTags:ClearTags()
		newTags:AddTag(Tag.new("AntFood"))
		newTags:AddTag(Tag.new("MushroomInstance"))
		if(newTransform ~= nil) then 
			newTransform:SetPosition(-512 + math.random(1024), 0.0, -512 + math.random(1024))
		end
		local newAntFood = World.AddComponent_AntFoodComponent(newEntity)
		if(newAntFood ~= nil) then 
			newAntFood.m_foodAmount = 100 + math.random(100)
		end
		local newPickup = World.AddComponent_AntPickupComponent(newEntity)
	end
end

function SpawnCollectorAnts()
	local template = World.GetFirstEntityWithTag(Tag.new("AntTemplate"))
	for i=1,collectorSpawnCount do 
		local newEntity = World.CloneEntity(template)
		local newTransform = World.GetComponent_Transform(newEntity)
		local newTags = World.GetComponent_Tags(newEntity)
		newTags:ClearTags()
		newTags:AddTag(Tag.new("AntInstance"))
		newTags:AddTag(Tag.new("Collector"))
		if(newTransform ~= nil) then 
			newTransform:SetPosition(-512 + math.random(1024), 0.0, -512 + math.random(1024))
		end
		local newAnt = World.AddComponent_AntComponent(newEntity)
		if(newAnt ~= nil) then 
			newAnt.m_behaviourTree = "ants/ant_brain.beht"
			newAnt.m_minEnergyForHunger = 25 + math.random(25)
		end
	end
end

function SpawnBuilderAnts()
	local template = World.GetFirstEntityWithTag(Tag.new("AntTemplate"))
	for i=1,builderSpawnCount do 
		local newEntity = World.CloneEntity(template)
		local newTransform = World.GetComponent_Transform(newEntity)
		local newTags = World.GetComponent_Tags(newEntity)
		newTags:ClearTags()
		newTags:AddTag(Tag.new("AntInstance"))
		newTags:AddTag(Tag.new("Builder"))
		if(newTransform ~= nil) then 
			newTransform:SetPosition(-512 + math.random(1024), 0.0, -512 + math.random(1024))
		end
		local newAnt = World.AddComponent_AntComponent(newEntity)
		if(newAnt ~= nil) then 
			newAnt.m_behaviourTree = "ants/ant_brain_builder.beht"
			newAnt.m_minEnergyForHunger = 25 + math.random(25)
		end
	end
end

function AntsMain(entity)
	Physics.SetSimulationEnabled(true)
	DebugGui.BeginWindow(true, 'Ants')
	if(DebugGui.Button('Spawn rocks')) then 
		SpawnRocks()
	end
	if(DebugGui.Button('Spawn mushrooms')) then 
		SpawnMushrooms()
	end
	collectorSpawnCount = DebugGui.DragInt('Collector spawn count', collectorSpawnCount, 1, 1, 1000)
	if(DebugGui.Button('Spawn collector ants')) then 
		SpawnCollectorAnts()
	end
	builderSpawnCount = DebugGui.DragInt('Builder spawn count', builderSpawnCount, 1, 1, 1000)
	if(DebugGui.Button('Spawn builder ants')) then 
		SpawnBuilderAnts()
	end
	if(DebugGui.Button('Reset')) then 
		local rockTag = Tag.new("RockInstance")
		local antTag = Tag.new("AntInstance")
		local foodTag = Tag.new("AntFood")
		World.RemoveEntitiesWithTag(rockTag)
		World.RemoveEntitiesWithTag(antTag)
		World.RemoveEntitiesWithTag(foodTag)
	end
	DebugGui.EndWindow()
end

return AntsMain
