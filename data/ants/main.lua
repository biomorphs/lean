function SpawnRock()
	local template = World.GetFirstEntityWithTag(Tag.new("RockSmall"))
	local newEntity = World.CloneEntity(template)
	local newTransform = World.GetComponent_Transform(newEntity)
	local newTags = World.GetComponent_Tags(newEntity)
	newTags:ClearTags()
	newTags:AddTag(Tag.new("RockInstance"))
	if(newTransform ~= nil) then 
		newTransform:SetPosition(-512 + math.random(1024), 2.0, -512 + math.random(1024))
	end
end

function AntsMain(entity)
	DebugGui.BeginWindow(true, 'Ants')
	if(DebugGui.Button('Spawn rocks')) then 
		for i=1,1000 do 
			SpawnRock()
		end
	end
	DebugGui.EndWindow()
end

return AntsMain
