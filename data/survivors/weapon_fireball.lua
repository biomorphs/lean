local lastSpawnTime = -1
local baseSpawnFrequency = 2.1
local baseDamageArea = 24
local baseDamage = 50

function RandomFloat(minv,maxv) 
	return minv + (math.random() * (maxv-minv))
end

function SpawnFireball(playerCmp, playerTransform, currentTime)
	local newEntity = World.AddEntity()
	local newTags = World.AddComponent_Tags(newEntity)
	newTags:AddTag(Tag.new("FireballInstance"))
	
	local newTransform = World.AddComponent_Transform(newEntity)
	local playerPos = playerTransform:GetPosition()
	newTransform:SetPosition(playerPos.x, playerPos.y + 4, playerPos.z)
	
	local newProjectile = World.AddComponent_ProjectileComponent(newEntity)
	newProjectile:SetMaxDistance(200 * playerCmp:GetAreaMultiplier())
	local dir = { -1 + math.random() * 2, -1 + math.random() * 2 }
	local speed = 150 + math.random() * 150
	newProjectile:SetVelocity(vec3.new(dir[1] * speed,0,dir[2] * speed))
	newProjectile:SetDamageRange(baseDamage * playerCmp:GetDamageMultiplier(), baseDamage * 1.1 * playerCmp:GetDamageMultiplier())
	newProjectile:SetRadius(4 * playerCmp:GetAreaMultiplier())

	local projPhysics = World.AddComponent_Physics(newEntity)
	projPhysics:SetStatic(false)
	projPhysics:SetKinematic(true)
	projPhysics:AddSphereCollider(vec3.new(0,0,0), 3 * playerCmp:GetAreaMultiplier())
	projPhysics:Rebuild()

	local particles = World.AddComponent_ComponentParticleEmitter(newEntity);
	particles:SetEmitter("survivorsfireball.emit")
end

function UpdateWeaponFireball(playerCmp, playerTransform)
	local currentTime = Time.GetElapsedTime()
	if(playerTransform ~= nil) then 
		if(currentTime - lastSpawnTime > (baseSpawnFrequency * playerCmp:GetCooldownMultiplier())) then 
			local ballCount = playerCmp:GetProjectileCount()
			for b=1,ballCount do
				SpawnFireball(playerCmp, playerTransform, currentTime)
			end
			lastSpawnTime = currentTime
		end
	end
end