local firstRun = true
local zombiesPerSecond = 2
local zombieSpawnTimer = 0
local zombieSpawnEnabled = false

function SpawnZombieAt(pos)
	local zombieTemplate = World.GetFirstEntityWithTag(Tag.new("ZombieTemplate"))
	local newZombie = World.CloneEntity(zombieTemplate)
	local newTransform = World.GetComponent_Transform(newZombie)
	if(newTransform ~= nil) then 
		newTransform:SetPosition(pos.x, pos.y, pos.z)
	end
	local newMonsterCmp = World.AddComponent_MonsterComponent(newZombie)
	newMonsterCmp:SetSpeed(2.0 + math.random() * 4.0)
end

function SpawnZombies()
	if(zombieSpawnEnabled == false or zombiesPerSecond <= 0) then 
		return 
	end
	zombieSpawnTimer = zombieSpawnTimer + 0.066	-- todo delta
	if(zombieSpawnTimer > (1.0 / zombiesPerSecond)) then 
		local foundPlayer = World.GetFirstEntityWithTag(Tag.new("PlayerCharacter"))
		local playerTransform = World.GetComponent_Transform(foundPlayer)
		local playerPos = playerTransform:GetPosition()
		local spawnRadiusMin = 350
		local spawnRadiusMax = 100
		local spawnCount = 1
		for s=0,spawnCount do
			local theta = math.random() * 2.0 * 3.14
			local spawnDistance = spawnRadiusMin + math.random() * spawnRadiusMax
			local rx = playerPos.x + math.sin(theta) * spawnDistance
			local ry = 0.5
			local rz = playerPos.z + math.cos(theta) * spawnDistance
			SpawnZombieAt(vec3.new(rx,ry,rz))
		end
		zombieSpawnTimer = 0
	end
end

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
	if(mr > 998) then
		return "survivors/grass_camp.scn"
	elseif(mr > 900) then 
		return "survivors/grass_basic_darker.scn"
	elseif(mr > 850) then 
		return "survivors/grass_basic_dark.scn"
	elseif(mr > 800) then 
		return "survivors/grass_basic_crates.scn"
	elseif(mr > 750) then 
		return "survivors/grass_trees.scn"
	elseif(mr > 700) then 
		return "survivors/grass_trees_dense.scn"
	else
		return "survivors/grass_basic.scn"
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

function DoStartGame()
	local foundCamera = World.GetFirstEntityWithTag(Tag.new("PlayerFollowCamera"))
	Graphics.SetActiveCamera(foundCamera)
	Survivors.SetWorldTileSpawnFn(SpawnWorldTiles_BasicFields)
	Survivors.RemoveAllTiles()
	Survivors.StartGame()
end

function SurvivorsMain(entity)
	PlayerUpdate();
	SpawnZombies();
	
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
		DoStartGame()
	end
	if(DebugGui.Button('Stop Game')) then 
		Survivors.StopGame()
	end
	if(DebugGui.Button('Spawn Zombies')) then 
		zombieSpawnEnabled = true
	end
	zombiesPerSecond = DebugGui.DragInt('Zombies/second', zombiesPerSecond, 1, 0, 128)
	if(DebugGui.Button('Reload Main Script')) then 
		local myScriptCmp = World.GetComponent_Script(entity)
		if(myScriptCmp ~= nil) then 
			myScriptCmp:SetNeedsCompile()
		end
	end 
	DebugGui.EndWindow()
end

return SurvivorsMain