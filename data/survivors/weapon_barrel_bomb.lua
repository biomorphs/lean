local lastBarrelSpawnTime = -1
local baseSpawnFrequency = 1.8
local baseExplosionDelay = 3.5
local baseExplosionArea = 24
local baseExplosionDamage = 80
local baseBarrelForceXZ = 400000
local baseBarrelForceY = 100000
local barrelXZSpawnOffset = 12
local spawnedBarrels = {}	-- {entity, spawn time, time before explosion, explosion area, explosion damage, force to apply to body

function RandomFloat(minv,maxv) 
	return minv + (math.random() * (maxv-minv))
end

function SpawnBarrel(playerCmp, playerTransform, currentTime)
	local template = World.GetFirstEntityWithTag(Tag.new("BarrelBombTemplate"))
	local newBarrel = World.CloneEntity(template)
	local newTransform = World.GetComponent_Transform(newBarrel)
	local newTags = World.GetComponent_Tags(newBarrel)
	newTags:ClearTags()
	newTags:AddTag(Tag.new("BarrelBombInstance"))
	
	local spawnPosition = playerTransform:GetPosition()
	spawnPosition.x = spawnPosition.x + RandomFloat(-barrelXZSpawnOffset,barrelXZSpawnOffset)
	spawnPosition.y = spawnPosition.y + 16
	spawnPosition.z = spawnPosition.z + RandomFloat(-barrelXZSpawnOffset,barrelXZSpawnOffset)
	newTransform:SetPosition(spawnPosition.x, spawnPosition.y, spawnPosition.z)
	
	local newPhysics = World.GetComponent_Physics(newBarrel)
	newPhysics:SetStatic(false)
	newPhysics:SetKinematic(false)
	newPhysics:Rebuild()
	
	local explosionDelay = baseExplosionDelay * playerCmp:GetCooldownMultiplier()
	local explodeArea = baseExplosionArea * playerCmp:GetAreaMultiplier()
	local explodeDamage = baseExplosionDamage * playerCmp:GetDamageMultiplier()
	local forceToApply = {RandomFloat(-baseBarrelForceXZ,baseBarrelForceXZ), baseBarrelForceY, RandomFloat(-baseBarrelForceXZ,baseBarrelForceXZ)}
	spawnedBarrels[#spawnedBarrels+1] = {newBarrel, currentTime, explosionDelay, explodeArea, explodeDamage, forceToApply}
end

function UpdateSpawnedBarrels(playerCmp, currentTime)
	for i = #spawnedBarrels,1,-1 do 
		local entity = spawnedBarrels[i][1]
		local timeSpawned = spawnedBarrels[i][2]
		local explosionDelay = spawnedBarrels[i][3]
		if((currentTime - timeSpawned) > explosionDelay) then 
			local barrelTransform = World.GetComponent_Transform(entity)
			local explodeArea = spawnedBarrels[i][4]
			local explodeDamage = spawnedBarrels[i][5]
			DoExplosionAt(barrelTransform:GetPosition(), explodeArea, explodeDamage)
			World.RemoveEntity(entity)

			spawnedBarrels[i] = spawnedBarrels[#spawnedBarrels]
			spawnedBarrels[#spawnedBarrels] = nil
		else
			local forceToAdd = spawnedBarrels[i][6]
			if(forceToAdd ~= nil) then 
				local physicsCmp = World.GetComponent_Physics(entity)
				physicsCmp:AddForce(vec3.new(forceToAdd[1], forceToAdd[2], forceToAdd[3]))
				spawnedBarrels[i][6] = nil
			end
		end
	end
end

function UpdateWeaponBarrelBomb(playerCmp, playerTransform)
	local currentTime = Time.GetElapsedTime()
	if(playerTransform ~= nil) then 
		UpdateSpawnedBarrels(playerCmp, currentTime)
		if(currentTime - lastBarrelSpawnTime > (baseSpawnFrequency * playerCmp:GetCooldownMultiplier())) then 
			local barrelCount = playerCmp:GetProjectileCount()
			for b=1,barrelCount do
				SpawnBarrel(playerCmp, playerTransform, currentTime)
			end
			lastBarrelSpawnTime = currentTime
		end
	end
end