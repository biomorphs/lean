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

function SpawnWorldTiles_BasicFields(tilePos_ivec2)
	local mr = math.random(0,1000)
	if(mr > 995) then
		return "survivors/grass_camp.scn"
	elseif(mr > 900) then 
		return "survivors/grass_basic_darker.scn"
	elseif(mr > 850) then 
		return "survivors/grass_basic_dark.scn"
	elseif(mr > 830) then 
		return "survivors/grass_basic_crates.scn"
	elseif(mr > 800) then 
		return "survivors/grass_trees.scn"
	elseif(mr > 780) then 
		return "survivors/grass_trees_dense.scn"
	else
		return "survivors/grass_basic_crates.scn"
	end
end

function PlayerUpdate()
	local playerSpeed = 0.2
	local foundPlayer = World.GetFirstEntityWithTag(Tag.new("PlayerCharacter"))
	local playerTransform = World.GetComponent_Transform(foundPlayer)
	local cct = World.GetComponent_CharacterController(foundPlayer)
	if(playerTransform == nil or cct == nil) then 
		return
	end
	
	local myPos = playerTransform:GetPosition()
	if(Input.IsKeyPressed('KEY_l')) then 
		myPos.x = myPos.x - playerSpeed
		playerTransform:SetRotation(0,-90,0)
	end
	if(Input.IsKeyPressed('KEY_j')) then 
		myPos.x = myPos.x + playerSpeed
		playerTransform:SetRotation(0,90,0)
	end
	if(Input.IsKeyPressed('KEY_k')) then 
		myPos.z = myPos.z - playerSpeed
		playerTransform:SetRotation(0,180,0)
	end
	if(Input.IsKeyPressed('KEY_i')) then 
		myPos.z = myPos.z + playerSpeed
		playerTransform:SetRotation(0,0,0)
	end
	playerTransform:SetPosition(myPos.x,myPos.y,myPos.z)
end

local firstRun = true
function SurvivorsMain(entity)
	local windowOpen = true
	PlayerUpdate();
	DebugGui.BeginWindow(true, 'Survivors!')
	local tileLoadRadius = Survivors.GetWorldTileLoadRadius()
	tileLoadRadius = DebugGui.DragInt('Tile load radius', tileLoadRadius, 1, 0, 64)
	Survivors.SetWorldTileLoadRadius(tileLoadRadius)
	if(DebugGui.Button('Spawn fields')) then 
		Survivors.SetWorldTileSpawnFn(SpawnWorldTiles_BasicFields)
	end
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
	if(DebugGui.Button('Start Game')) then 
		Survivors.StartGame()
	end
	if(DebugGui.Button('Stop Game')) then 
		Survivors.StopGame()
	end
	if(DebugGui.Button('Reload Main Script')) then 
		local myScriptCmp = World.GetComponent_Script(entity)
		if(myScriptCmp ~= nil) then 
			myScriptCmp:SetNeedsCompile()
		end
	end 
	DebugGui.EndWindow()
end

return SurvivorsMain