local firstRun = true

local spawnRadiusMin = 360
local spawnRadiusMax = 40

local zombieChadsPerSecond = 0.0
local zombieChadsPerSecondIncrement = 0.000001
local zombieChadSpawnTimer = 0
local zombieChadSpawnEnabled = false
local zombiesPerSecond = 0.0
local zombiesPerSecondIncrement = 0.0005
local zombieSpawnTimer = 0
local zombieSpawnEnabled = false
local skeletonsPerSecond = 0.0
local skeletonsPerSecondIncrement = 0.0001
local skeletonsSpawnTimer = 0
local skeletonsSpawnEnabled = false

function SpawnSkeletonAt(pos)
	local template = World.GetFirstEntityWithTag(Tag.new("SkeletonTemplate"))
	local newSkele = World.CloneEntity(template)
	local newTransform = World.GetComponent_Transform(newSkele)
	if(newTransform ~= nil) then 
		newTransform:SetPosition(pos.x, pos.y, pos.z)
	end
	local newMonsterCmp = World.AddComponent_MonsterComponent(newSkele)
	newMonsterCmp:SetSpeed(3.0 + math.random() * 6.0)
	newMonsterCmp:SetCollideRadius(4.5)
end

function SpawnZombieChadAt(pos)
	local zombieTemplate = World.GetFirstEntityWithTag(Tag.new("ZombieChadTemplate"))
	local newZombie = World.CloneEntity(zombieTemplate)
	local newTransform = World.GetComponent_Transform(newZombie)
	if(newTransform ~= nil) then 
		newTransform:SetPosition(pos.x, pos.y, pos.z)
	end
	local newMonsterCmp = World.AddComponent_MonsterComponent(newZombie)
	newMonsterCmp:SetSpeed(6.0 + math.random() * 10.0)
	newMonsterCmp:SetCollideRadius(8.5)
end

function SpawnZombieAt(pos)
	local zombieTemplate = World.GetFirstEntityWithTag(Tag.new("ZombieTemplate"))
	local newZombie = World.CloneEntity(zombieTemplate)
	local newTransform = World.GetComponent_Transform(newZombie)
	if(newTransform ~= nil) then 
		newTransform:SetPosition(pos.x, pos.y, pos.z)
	end
	local newMonsterCmp = World.AddComponent_MonsterComponent(newZombie)
	newMonsterCmp:SetSpeed(4.0 + math.random() * 8.0)
	newMonsterCmp:SetCollideRadius(3.5)
end

function SpawnEnemy(SpawnAtFn)
	local foundPlayer = World.GetFirstEntityWithTag(Tag.new("PlayerCharacter"))	-- can be cached
	local playerTransform = World.GetComponent_Transform(foundPlayer)
	if(playerTransform ~= nil) then
		local playerPos = playerTransform:GetPosition()
		local theta = math.random() * 2.0 * 3.14
		local spawnDistance = spawnRadiusMin + math.random() * spawnRadiusMax
		local rx = playerPos.x + math.sin(theta) * spawnDistance
		local ry = 0.5
		local rz = playerPos.z + math.cos(theta) * spawnDistance
		SpawnAtFn(vec3.new(rx,ry,rz))
	end
end

function SpawnSkeletons()
	if(skeletonsSpawnEnabled == false or skeletonsPerSecond <= 0) then 
		return 
	end
	skeletonsSpawnTimer = skeletonsSpawnTimer + 0.066	-- todo delta
	if(skeletonsSpawnTimer > (1.0 / skeletonsPerSecond)) then 
		SpawnEnemy(SpawnSkeletonAt)
		skeletonsSpawnTimer = 0
	end
end

function SpawnZombies()
	if(zombieSpawnEnabled == false or zombiesPerSecond <= 0) then 
		return 
	end
	zombieSpawnTimer = zombieSpawnTimer + 0.066	-- todo delta
	if(zombieSpawnTimer > (1.0 / zombiesPerSecond)) then 
		SpawnEnemy(SpawnZombieAt)
		zombieSpawnTimer = 0
	end
end

function SpawnZombieChads()
	if(zombieChadSpawnEnabled == false or zombieChadsPerSecond <= 0) then 
		return 
	end
	zombieChadSpawnTimer = zombieChadSpawnTimer + 0.066	-- todo delta
	if(zombieChadSpawnTimer > (1.0 / zombieChadsPerSecond)) then 
		SpawnEnemy(SpawnZombieChadAt)
		zombieChadSpawnTimer = 0
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

function DoStartGame()
	local foundCamera = World.GetFirstEntityWithTag(Tag.new("PlayerFollowCamera"))
	Graphics.SetActiveCamera(foundCamera)
	Survivors.SetWorldTileSpawnFn(SpawnWorldTiles_BasicFields)
	Survivors.RemoveAllTiles()
	Survivors.StartGame()
	zombieChadSpawnEnabled = true
	zombieChadsPerSecond = 0.001
	zombieSpawnEnabled = true
	zombiesPerSecond = 1.0
	skeletonsSpawnEnabled = true
	skeletonsPerSecond = 0.2
end

function DoStopGame()
	zombieChadsPerSecond = 0.0
	zombieChadSpawnEnabled = false
	zombiesPerSecond = 0.0
	zombieSpawnEnabled = false
	skeletonsPerSecond = 0.0
	skeletonsSpawnEnabled = false
	Survivors.StopGame()
end

function PlayerUpdate()
	local playerSpeed = 0.3
	local foundPlayer = World.GetFirstEntityWithTag(Tag.new("PlayerCharacter"))
	local playerTransform = World.GetComponent_Transform(foundPlayer)
	if(playerTransform == nil) then 
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

function SurvivorsMain(entity)
	PlayerUpdate();
	SpawnZombies();
	SpawnZombieChads();
	SpawnSkeletons();
	
	if(zombieSpawnEnabled==true) then 
		zombiesPerSecond = zombiesPerSecond + zombiesPerSecondIncrement
	end
	if(zombieChadSpawnEnabled==true) then 
		zombieChadsPerSecond = zombieChadsPerSecond + zombieChadsPerSecondIncrement
	end
	if(skeletonsSpawnEnabled==true) then
		skeletonsPerSecond = skeletonsPerSecond + skeletonsPerSecondIncrement
	end
	
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
		DoStopGame()
	end
	if(DebugGui.Button('Spawn Zombies')) then 
		zombieSpawnEnabled = true
	end
	if(DebugGui.Button('200 zombies')) then 
		for i=0,200 do
			SpawnEnemy(SpawnZombieAt)
		end
	end
	if(DebugGui.Button('Spawn Zombie Chad')) then 
		zombieChadSpawnEnabled = true
	end
	if(DebugGui.Button('20 zombie chads')) then 
		for i=0,20 do
			SpawnEnemy(SpawnZombieChadAt)
		end
	end
	if(DebugGui.Button('Spawn Skelies')) then 
		skeletonsSpawnEnabled = true
	end
	if(DebugGui.Button('200 skelies')) then 
		for i=0,200 do
			SpawnEnemy(SpawnSkeletonAt)
		end
	end
	spawnRadiusMin = DebugGui.DragFloat('Spawn min radius', spawnRadiusMin, 1.0, 2.0, 800)
	spawnRadiusMax = DebugGui.DragFloat('Spawn ring size', spawnRadiusMax, 1.0, 0, 800.0)
	zombiesPerSecond = DebugGui.DragFloat('Zombies/second', zombiesPerSecond, 1.0, 0.0, 128.0)
	zombieChadsPerSecond = DebugGui.DragFloat('Zombie chads/second', zombieChadsPerSecond, 1.0, 0.0, 128.0)
	skeletonsPerSecond = DebugGui.DragFloat('Skeletons/second', skeletonsPerSecond, 1.0, 0.0, 128.0)
	
	if(DebugGui.Button('Reload Main Script')) then 
		local myScriptCmp = World.GetComponent_Script(entity)
		if(myScriptCmp ~= nil) then 
			myScriptCmp:SetNeedsCompile()
		end
	end 
	DebugGui.EndWindow()
end

return SurvivorsMain