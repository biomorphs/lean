require "survivors/weapon_explode_nova"
require "survivors/weapon_barrel_bomb"

local isFirstRun = true
local playerStartPosition = {5000,0,5000}
local playerBaseMoveSpeed = 24
local playerXpIncreasePerLevel = 1.2

-- game state
local isWaitingOnLevelUp = false

-- monster spawning
local spawnRadiusMin = 360
local spawnRadiusMax = 40
local zombieChadsPerSecond = 0.0
local zombieChadsPerSecondIncrement = 0.000002
local zombieChadSpawnTimer = 0
local zombieChadSpawnEnabled = false
local zombiesPerSecond = 0.0
local zombiesPerSecondIncrement = 0.001
local zombieSpawnTimer = 0
local zombieSpawnEnabled = false
local skeletonsPerSecond = 0.0
local skeletonsPerSecondIncrement = 0.0002
local skeletonsSpawnTimer = 0
local skeletonsSpawnEnabled = false
local bossesPerSecond = 0
local bossesPerSecondIncrement = 0.000001
local bossesSpawnTimer = 0
local bossesSpawnEnabled = false
local godMode = false

-- weapons
local explodeNovaActive = false
local barrelBombActive = false

-- hud
local barBg = vec4.new(0.2,0.2,0.2,1)
local barBorder = 4
local wizardThumb = Graphics.LoadTexture("wizard_thumbnail.png")
local deadImg = Graphics.LoadTexture("dead.png")

function DoExplosionAt(pos, radius, damage)
	local template = World.GetFirstEntityWithTag(Tag.new("ExplosionTemplate"))
	local newEntity = World.CloneEntity(template)
	local newTags = World.GetComponent_Tags(newEntity)
	newTags:ClearTags()
	newTags:AddTag(Tag.new("ExplosionInstance"))
	local newTransform = World.GetComponent_Transform(newEntity)
	local newPhysics = World.GetComponent_Physics(newEntity)
	if(newPhysics ~= nil) then 
		newPhysics:AddSphereCollider(vec3.new(0,0,0), radius)
		newPhysics:Rebuild()
	end
	if(newTransform ~= nil) then 
		newTransform:SetPosition(pos.x, pos.y, pos.z)
		newTransform:SetScale(radius,radius,radius)
	end
	local newExplosion = World.AddComponent_ExplosionComponent(newEntity)
	newExplosion:SetDamageRadius(radius)
	newExplosion:SetDamageAtCenter(damage)
	newExplosion:SetDamageAtEdge(damage / 2)
	newExplosion:SetFadeoutSpeed(2)
end

function SpawnBossAt(pos)
	
	local template = World.GetFirstEntityWithTag(Tag.new("SkullBossTemplate"))
	local damageTemplate = World.GetFirstEntityWithTag(Tag.new("SkullBossDamaged"))
	local newBoss = World.CloneEntity(template)
	local newTags = World.GetComponent_Tags(newBoss)
	newTags:ClearTags()
	newTags:AddTag(Tag.new("BossInstance"))
	local newTransform = World.GetComponent_Transform(newBoss)
	if(newTransform ~= nil) then 
		newTransform:SetPosition(pos.x, pos.y, pos.z)
	end
	local newMonsterCmp = World.AddComponent_MonsterComponent(newBoss)
	newMonsterCmp:SetSpeed(20.0 + math.random() * 20.0)
	newMonsterCmp:SetCollideRadius(18)
	newMonsterCmp:SetRagdollChance(0.0)
	newMonsterCmp:SetDamagedMaterialEntity(damageTemplate)
	newMonsterCmp:SetAttackFrequency(0.25)
	newMonsterCmp:SetAttackMinValue(15)
	newMonsterCmp:SetAttackMaxValue(30)
	newMonsterCmp:SetXPOnDeath(1000)
	newMonsterCmp:SetCurrentHealth(1000)
	
	local lightTemplate0 = World.GetFirstEntityWithTag(Tag.new("SkullBossTemplateLight0"))
	local lightTemplate1 = World.GetFirstEntityWithTag(Tag.new("SkullBossTemplateLight1"))
	local newLight0 = World.CloneEntity(lightTemplate0)
	local newLight1 = World.CloneEntity(lightTemplate1)
	local transformLight0 = World.GetComponent_Transform(newLight0)
	local transformLight1 = World.GetComponent_Transform(newLight1)
	transformLight0:SetParent(newBoss)
	transformLight1:SetParent(newBoss)
	
	print("A boss has spawned!")
end

function SpawnSkeletonAt(pos)
	local template = World.GetFirstEntityWithTag(Tag.new("SkeletonTemplate"))
	local damageTemplate = World.GetFirstEntityWithTag(Tag.new("SkeletonDamaged"))
	local newSkele = World.CloneEntity(template)
	local newTags = World.GetComponent_Tags(newSkele)
	newTags:ClearTags()
	newTags:AddTag(Tag.new("SkeletonInstance"))
	local newTransform = World.GetComponent_Transform(newSkele)
	if(newTransform ~= nil) then 
		newTransform:SetPosition(pos.x, pos.y, pos.z)
	end
	local newMonsterCmp = World.AddComponent_MonsterComponent(newSkele)
	newMonsterCmp:SetSpeed(5.0 + math.random() * 15.0)
	newMonsterCmp:SetCollideRadius(5.5)
	newMonsterCmp:SetRagdollChance(0.1)
	newMonsterCmp:SetDamagedMaterialEntity(damageTemplate)
	newMonsterCmp:SetAttackFrequency(0.4)
	newMonsterCmp:SetAttackMinValue(5)
	newMonsterCmp:SetAttackMaxValue(10)
	newMonsterCmp:SetXPOnDeath(10)
	newMonsterCmp:SetCurrentHealth(120)
end

function SpawnZombieChadAt(pos)
	local zombieTemplate = World.GetFirstEntityWithTag(Tag.new("ZombieChadTemplate"))
	local damageTemplate = World.GetFirstEntityWithTag(Tag.new("ZombieChadDamaged"))
	local newZombie = World.CloneEntity(zombieTemplate)
	local newTags = World.GetComponent_Tags(newZombie)
	newTags:ClearTags()
	newTags:AddTag(Tag.new("ZombieChadInstance"))
	local newTransform = World.GetComponent_Transform(newZombie)
	if(newTransform ~= nil) then 
		newTransform:SetPosition(pos.x, pos.y, pos.z)
	end
	local newMonsterCmp = World.AddComponent_MonsterComponent(newZombie)
	newMonsterCmp:SetSpeed(6.0 + math.random() * 10.0)
	newMonsterCmp:SetCollideRadius(8.5)
	newMonsterCmp:SetRagdollChance(0.0)
	newMonsterCmp:SetDamagedMaterialEntity(damageTemplate)
	newMonsterCmp:SetAttackFrequency(1.2)
	newMonsterCmp:SetAttackMinValue(10)
	newMonsterCmp:SetAttackMaxValue(15)
	newMonsterCmp:SetXPOnDeath(50)
	newMonsterCmp:SetCurrentHealth(400)
end

function SpawnZombieAt(pos)
	local zombieTemplate = World.GetFirstEntityWithTag(Tag.new("ZombieTemplate"))
	local damageTemplate = World.GetFirstEntityWithTag(Tag.new("ZombieDamaged"))
	local newZombie = World.CloneEntity(zombieTemplate)
	local newTags = World.GetComponent_Tags(newZombie)
	newTags:ClearTags()
	newTags:AddTag(Tag.new("ZombieInstance"))
	local newTransform = World.GetComponent_Transform(newZombie)
	if(newTransform ~= nil) then 
		newTransform:SetPosition(pos.x, pos.y, pos.z)
	end
	local newMonsterCmp = World.AddComponent_MonsterComponent(newZombie)
	newMonsterCmp:SetSpeed(4.0 + math.random() * 8.0)
	newMonsterCmp:SetCollideRadius(4.5)
	newMonsterCmp:SetRagdollChance(0.05)
	newMonsterCmp:SetDamagedMaterialEntity(damageTemplate)
	newMonsterCmp:SetAttackFrequency(0.8)
	newMonsterCmp:SetAttackMinValue(3)
	newMonsterCmp:SetAttackMaxValue(15)
	newMonsterCmp:SetXPOnDeath(5)
	newMonsterCmp:SetDamageEffectPath("zombiedamagedeffect.emit")
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

function SpawnBosses()
	if(bossesSpawnEnabled == false or bossesPerSecond <= 0) then 
		return 
	end
	bossesSpawnTimer = bossesSpawnTimer + Scripts.GetTimeDelta()
	if(bossesSpawnTimer > (1.0 / bossesPerSecond)) then 
		SpawnEnemy(SpawnBossAt)
		bossesSpawnTimer = 0
	end
	bossesPerSecond = bossesPerSecond + bossesPerSecondIncrement
end

function SpawnSkeletons()
	if(skeletonsSpawnEnabled == false or skeletonsPerSecond <= 0) then 
		return 
	end
	skeletonsSpawnTimer = skeletonsSpawnTimer + Scripts.GetTimeDelta()
	if(skeletonsSpawnTimer > (1.0 / skeletonsPerSecond)) then 
		SpawnEnemy(SpawnSkeletonAt)
		skeletonsSpawnTimer = 0
	end
	skeletonsPerSecond = skeletonsPerSecond + skeletonsPerSecondIncrement
end

function SpawnZombies()
	if(zombieSpawnEnabled == false or zombiesPerSecond <= 0) then 
		return 
	end
	zombieSpawnTimer = zombieSpawnTimer + Scripts.GetTimeDelta()
	if(zombieSpawnTimer > (1.0 / zombiesPerSecond)) then 
		SpawnEnemy(SpawnZombieAt)
		zombieSpawnTimer = 0
	end
	zombiesPerSecond = zombiesPerSecond + zombiesPerSecondIncrement
end

function SpawnZombieChads()
	if(zombieChadSpawnEnabled == false or zombieChadsPerSecond <= 0) then 
		return 
	end
	zombieChadSpawnTimer = zombieChadSpawnTimer + Scripts.GetTimeDelta()
	if(zombieChadSpawnTimer > (1.0 / zombieChadsPerSecond)) then 
		SpawnEnemy(SpawnZombieChadAt)
		zombieChadSpawnTimer = 0
	end
	zombieChadsPerSecond = zombieChadsPerSecond + zombieChadsPerSecondIncrement
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

function ResetPlayer()
	local foundPlayer = World.GetFirstEntityWithTag(Tag.new("PlayerCharacter"))
	local playerCmp = World.GetComponent_PlayerComponent(foundPlayer)
	playerCmp:SetCurrentHealth(100)
	playerCmp:SetMaximumHealth(100)
	
	playerCmp:SetCurrentXP(0)
	playerCmp:SetThisLevelXP(0)
	playerCmp:SetNextLevelXP(50)
	
	playerCmp:SetPickupAreaMultiplier(1)
	playerCmp:SetAreaMultiplier(1)
	playerCmp:SetDamageMultiplier(1)
	playerCmp:SetCooldownMultiplier(1)
	playerCmp:SetMoveSpeedMultiplier(1)
	playerCmp:SetProjectileCount(1)
	
	local playerTransform = World.GetComponent_Transform(foundPlayer)
	playerTransform:SetPosition(playerStartPosition[1], playerStartPosition[2], playerStartPosition[3])
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
	bossesSpawnEnabled = true 
	bossesPerSecond = 0.000001
	explodeNovaActive = true
	barrelBombActive = true
	ResetPlayer()
	
	for i=0,100 do
		SpawnEnemy(SpawnZombieAt)
	end
end

function DoStopGame()
	zombieChadsPerSecond = 0.0
	zombieChadSpawnEnabled = false
	zombiesPerSecond = 0.0
	zombieSpawnEnabled = false
	skeletonsPerSecond = 0.0
	skeletonsSpawnEnabled = false
	bossesPerSecond = 0.0
	bossesSpawnEnabled = false
	Survivors.StopGame()
	explodeNovaActive = false
	barrelBombActive = false
end

function OnPlayerDead()
	DoStopGame()
	
	local windowDims = Graphics2D.GetDimensions()
	local imgDims = {1280,256}
	local offset = {(windowDims.x - imgDims[1]) / 2, (windowDims.y - imgDims[2]) / 2}
	
	Graphics2D.DrawSolidQuad(0,0,0,windowDims.x,windowDims.y,0,0,0,0.8)
	Graphics2D.DrawSolidQuad(0,offset[2],0,windowDims.x,imgDims[2],0,0,0,1)
	Graphics2D.DrawTexturedQuad(offset[1], offset[2], 0, imgDims[1], imgDims[2], 0,0,1,1, 1,1,1,1, deadImg)
	
	if(Input.IsKeyPressed('KEY_SPACE')) then 
		DoStartGame()
	end
end

function OnLevelUp(playerCmp)
	Survivors.SetEnemiesEnabled(false)
	Survivors.SetAttractorsEnabled(false)
	Physics.SetSimulationEnabled(false)
	DebugGui.BeginWindow(true, 'Level Up!')
	local hasLeveledUp = false
	if(DebugGui.Button("AREA++")) then 
		playerCmp:SetAreaMultiplier(playerCmp:GetAreaMultiplier() + 0.08)
		hasLeveledUp = true
	end
	if(DebugGui.Button("DAMAGE++")) then 
		playerCmp:SetDamageMultiplier(playerCmp:GetDamageMultiplier() + 0.08)
		hasLeveledUp = true
	end
	if(DebugGui.Button("COOLDOWN++")) then 
		playerCmp:SetCooldownMultiplier(playerCmp:GetCooldownMultiplier() * 0.92)
		hasLeveledUp = true
	end
	if(DebugGui.Button("PROJECTILES++")) then 
		playerCmp:SetProjectileCount(playerCmp:GetProjectileCount() + 1)
		hasLeveledUp = true
	end
	if(DebugGui.Button("MOVEMENT++")) then 
		playerCmp:SetMoveSpeedMultiplier(playerCmp:GetMoveSpeedMultiplier() + 0.15)
		hasLeveledUp = true
	end
	if(DebugGui.Button("PICKUP AREA++")) then 
		playerCmp:SetPickupAreaMultiplier(playerCmp:GetPickupAreaMultiplier() + 0.12)
		hasLeveledUp = true
	end
	if(DebugGui.Button("MAX HP++")) then 
		playerCmp:SetMaximumHealth(playerCmp:GetMaximumHealth() * 1.1)
		hasLeveledUp = true
	end
	if(playerCmp:GetCurrentHealth() < playerCmp:GetMaximumHealth()) then 
		if(DebugGui.Button("HP++")) then 
			playerCmp:SetCurrentHealth(math.min(playerCmp:GetMaximumHealth(), playerCmp:GetCurrentHealth() + playerCmp:GetMaximumHealth() * 0.25))
			hasLeveledUp = true
		end
	end
	DebugGui.EndWindow()
	if(hasLeveledUp) then 
		local nextLvlIncrease = (playerCmp:GetNextLevelXP() - playerCmp:GetThisLevelXP()) * playerXpIncreasePerLevel
		playerCmp:SetThisLevelXP(playerCmp:GetNextLevelXP())
		playerCmp:SetNextLevelXP(playerCmp:GetNextLevelXP() + nextLvlIncrease)
		Survivors.SetEnemiesEnabled(true)
		Survivors.SetAttractorsEnabled(true)
		Physics.SetSimulationEnabled(true)
	end
end

function PlayerUpdate(playerCmp, playerTransform)
	local playerSpeed = playerCmp:GetMoveSpeedMultiplier() * playerBaseMoveSpeed * Scripts.GetTimeDelta()
	
	local myPos = playerTransform:GetPosition()
	if(Input.IsKeyPressed('KEY_d')) then 
		myPos.x = myPos.x - playerSpeed
		playerTransform:SetRotation(0,-90,0)
	end
	if(Input.IsKeyPressed('KEY_a')) then 
		myPos.x = myPos.x + playerSpeed
		playerTransform:SetRotation(0,90,0)
	end
	if(Input.IsKeyPressed('KEY_s')) then 
		myPos.z = myPos.z - playerSpeed
		playerTransform:SetRotation(0,180,0)
	end
	if(Input.IsKeyPressed('KEY_w')) then 
		myPos.z = myPos.z + playerSpeed
		playerTransform:SetRotation(0,0,0)
	end
	playerTransform:SetPosition(myPos.x,myPos.y,myPos.z)
end

function LerpColour(c0, c1, t)
	local cOut = vec4.new()
	cOut.x = (c0.x * t) + (c1.x * (1-t))
	cOut.y = (c0.y * t) + (c1.y * (1-t))
	cOut.z = (c0.z * t) + (c1.z * (1-t))
	cOut.w = (c0.w * t) + (c1.w * (1-t))
	return cOut
end

function DrawHUD(playerCmp)
	local windowDims = Graphics2D.GetDimensions()
	
	-- thumbnail
	local thumbOffset = {32,32}
	local thumbDims = {128,128}
	local thumbPos = {thumbOffset[1],windowDims.y - thumbOffset[2] - thumbDims[2]}
	Graphics2D.DrawSolidQuad(thumbPos[1],thumbPos[2],0,thumbDims[1],thumbDims[2],barBg.x,barBg.y,barBg.z,barBg.w)
	Graphics2D.DrawTexturedQuad(thumbPos[1] + barBorder, thumbPos[2] + barBorder, 0, thumbDims[1] - barBorder*2, thumbDims[2] - barBorder*2, 0,0,1,1, 1,1,1,1, wizardThumb)
	
	-- hp bar
	local healthBarOffset = {16 + thumbOffset[1] + thumbDims[1],32}
	local healthBarDims = {windowDims.x / 3,32}
	local healthBarPos = {healthBarOffset[1], windowDims.y - healthBarOffset[2] - healthBarDims[2]}
	local healthBarHealthy = vec4.new(0,0.8,0,1)
	local healthBarDead = vec4.new(1,0.0,0,1)
	Graphics2D.DrawSolidQuad(healthBarPos[1],healthBarPos[2],0,healthBarDims[1],healthBarDims[2],barBg.x,barBg.y,barBg.z,barBg.w)
	
	-- player text 
	Graphics2D.DrawText(healthBarPos[1], healthBarPos[2] - 44, 0, "CLASS: WIZARD", "MedievalSharp/MedievalSharp-Regular.ttf", 48, 1, 1, 1, 1)
	Graphics2D.DrawText(healthBarPos[1], healthBarPos[2] - 80, 0, "HP: " .. playerCmp:GetCurrentHealth() .. " / " .. playerCmp:GetMaximumHealth(), "MedievalSharp/MedievalSharp-Regular.ttf", 28, 1, 1, 1, 1)
	
	local hpRatio = playerCmp:GetCurrentHealth() / (playerCmp:GetMaximumHealth())
	local barLength = (healthBarDims[1] - barBorder*2) * hpRatio
	local barColour = LerpColour(healthBarHealthy, healthBarDead, hpRatio)
	Graphics2D.DrawSolidQuad(healthBarPos[1] + barBorder,healthBarPos[2] + barBorder,0,barLength,healthBarDims[2]-barBorder*2,barColour.x,barColour.y,barColour.z,barColour.w)
	
	-- xp bar
	local xpBarOffset = {16, 16}
	local xpBarDims = {windowDims.x - 32, 16}
	Graphics2D.DrawSolidQuad(xpBarOffset[1],xpBarOffset[2],0,xpBarDims[1],xpBarDims[2],barBg.x,barBg.y,barBg.z,barBg.w)
	
	local xpThisLvl = playerCmp:GetCurrentXP() - playerCmp:GetThisLevelXP()
	local xpNextLvl = playerCmp:GetNextLevelXP() - playerCmp:GetThisLevelXP()
	local xpRatio = math.min(xpThisLvl,xpNextLvl) / xpNextLvl
	local xpLowColour = vec4.new(0,0.49,1,1)
	local xpHighColour = vec4.new(0,0.8,1,1)
	local xpBarColour = LerpColour(xpLowColour, xpHighColour, xpRatio)
	local xpBarLength = xpRatio * (xpBarDims[1] - barBorder*2)
	Graphics2D.DrawSolidQuad(xpBarOffset[1] + barBorder,xpBarOffset[2] + barBorder,0,xpBarLength,xpBarDims[2]-barBorder*2,xpBarColour.x,xpBarColour.y,xpBarColour.z,xpBarColour.w)
end

function ShowEditor(entity)
	DebugGui.BeginWindow(true, 'Survivors!')
	local tileLoadRadius = Survivors.GetWorldTileLoadRadius()
	tileLoadRadius = DebugGui.DragInt('Tile load radius', tileLoadRadius, 1, 0, 64)
	Survivors.SetWorldTileLoadRadius(tileLoadRadius)
	if(DebugGui.Button('Spawn fields')) then 
		Survivors.SetWorldTileSpawnFn(SpawnWorldTiles_BasicFields)
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
	if(DebugGui.Button('Spawn Boss')) then
		SpawnEnemy(SpawnBossAt)
	end
	
	spawnRadiusMin = DebugGui.DragFloat('Spawn min radius', spawnRadiusMin, 1.0, 2.0, 800)
	spawnRadiusMax = DebugGui.DragFloat('Spawn ring size', spawnRadiusMax, 1.0, 0, 800.0)
	zombiesPerSecond = DebugGui.DragFloat('Zombies/second', zombiesPerSecond, 1.0, 0.0, 128.0)
	zombieChadsPerSecond = DebugGui.DragFloat('Zombie chads/second', zombieChadsPerSecond, 1.0, 0.0, 128.0)
	skeletonsPerSecond = DebugGui.DragFloat('Skeletons/second', skeletonsPerSecond, 1.0, 0.0, 128.0)
	bossesPerSecond = DebugGui.DragFloat('Bosses/second', bossesPerSecond, 1.0, 0.0, 128)
	godMode = DebugGui.Checkbox('God Mode', godMode)
	
	if(DebugGui.Button('Reload Main Script')) then 
		local myScriptCmp = World.GetComponent_Script(entity)
		if(myScriptCmp ~= nil) then 
			myScriptCmp:SetNeedsCompile()
		end
	end 
	DebugGui.EndWindow()
end

function OnFirstRun()
	local foundXPTemplate = World.GetFirstEntityWithTag(Tag.new("XPCrystalTemplate"))
	local foundMushroomTemplate = World.GetFirstEntityWithTag(Tag.new("MushroomTemplate"))
	Survivors.SetXPTemplateEntity(foundXPTemplate)
	Survivors.SetMushroomTemplateEntity(foundMushroomTemplate)
	if(Survivors.IsEditorActive() == false) then
		DoStartGame()
	end
end

function SurvivorsMain(entity)
	if(isFirstRun) then 
		OnFirstRun()
		isFirstRun = false
	end
	
	local foundPlayer = World.GetFirstEntityWithTag(Tag.new("PlayerCharacter"))
	local playerCmp = World.GetComponent_PlayerComponent(foundPlayer)
	local playerTransform = World.GetComponent_Transform(foundPlayer)
	if godMode then
		playerCmp:SetCurrentHealth(playerCmp:GetMaximumHealth())
	end
	
	if(playerCmp:GetCurrentHealth() <= 0) then 
		OnPlayerDead()
	else 
		if(playerCmp:GetCurrentXP() >= playerCmp:GetNextLevelXP()) then
			OnLevelUp(playerCmp)
		else
			PlayerUpdate(playerCmp, playerTransform);
			if(explodeNovaActive) then 
				UpdateWeaponExplodeNova(playerCmp, playerTransform)
			end
			if(barrelBombActive) then 
				UpdateWeaponBarrelBomb(playerCmp, playerTransform)
			end
			SpawnZombies();
			SpawnZombieChads();
			SpawnSkeletons();
			SpawnBosses();
		end
	end
	
	DrawHUD(playerCmp);	
	
	if(Survivors.IsEditorActive()) then
		ShowEditor(entity)
	end
end

return SurvivorsMain