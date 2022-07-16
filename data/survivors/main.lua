function SpawnWorldTiles_Basic(tilePos_ivec2)
	if(math.random(0,100) > 99) then
		return "survivors/basic_white_middlepillar.scn"
	else
		return "survivors/basic_white.scn"
	end
end

function SpawnWorldTiles_BasicHorizontal(tilePos_ivec2)
	if(tilePos_ivec2.y < -2 or tilePos_ivec2.y > 2) then 
		return ""
	end
	if(tilePos_ivec2.y == -2) then 
		return "survivors/basic_white_top.scn"
	end
	if(tilePos_ivec2.y == 2) then 
		return "survivors/basic_white_bottom.scn"
	end
	if(math.random(0,100) > 80) then
		return "survivors/basic_white_middlepillar.scn"
	else
		return "survivors/basic_white.scn"
	end
end

local firstRun = true
function SurvivorsMain(entity)
	local windowOpen = true
	DebugGui.BeginWindow(true, 'Survivors!')
	local tileLoadRadius = Survivors.GetWorldTileLoadRadius()
	tileLoadRadius = DebugGui.DragInt('Tile load radius', tileLoadRadius, 1, 0, 64)
	Survivors.SetWorldTileLoadRadius(tileLoadRadius)
	if(DebugGui.Button('Basic tile spawning')) then 
		Survivors.SetWorldTileSpawnFn(SpawnWorldTiles_Basic)
	end
	if(DebugGui.Button('Basic horizontal spawning')) then 
		Survivors.SetWorldTileSpawnFn(SpawnWorldTiles_BasicHorizontal)
	end
	if(DebugGui.Button('Disable tile spawning')) then 
		Survivors.SetWorldTileSpawnFn(nil)
	end
	if(DebugGui.Button('Remove all tiles')) then 
		Survivors.RemoveAllTiles()
	end
	DebugGui.EndWindow()
end

return SurvivorsMain