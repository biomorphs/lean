local lastNovaTime = -1
local novaFrequency = 2.5
local novaArea = 40
local novaDamage = 40

function UpdateWeaponExplodeNova(playerCmp, playerTransform)
	local currentTime = Time.GetElapsedTime()
	if(playerTransform ~= nil) then 
		if(currentTime - lastNovaTime > (novaFrequency * playerCmp:GetCooldownMultiplier())) then 
			Particles.StartEmitter('survivorsnovasmall.emit', playerTransform:GetPosition())
			local dmg = novaDamage * playerCmp:GetDamageMultiplier()
			Survivors.DoDamageInArea(playerTransform:GetPosition(), novaArea * playerCmp:GetAreaMultiplier(), dmg, dmg)
			lastNovaTime = currentTime
		end
	end
end