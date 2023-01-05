
function DoLightAt(pos)
	local template = World.GetFirstEntityWithTag(Tag.new("LightTemplate"))
	local newEntity = World.CloneEntity(template)
	local newTags = World.GetComponent_Tags(newEntity)
	newTags:ClearTags()
	local newTransform = World.GetComponent_Transform(newEntity)
	if(newTransform ~= nil) then 
		newTransform:SetPosition(pos.x, pos.y, pos.z)
	end
end

function Main(entity)
	DebugGui.BeginWindow(true, 'Lights!')
	
	if(DebugGui.Button('Spawn lights')) then 
		for x=0,64 do
			for z=0,64 do
				local pos = vec3.new(x * 64, 32, z * 64)
				DoLightAt(pos)
			end
		end
	end
	DebugGui.EndWindow()
end

return Main