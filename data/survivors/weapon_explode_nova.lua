local lastNovaTime = -1
local novaFrequency = 2
local novaArea = 48
local novaDamage = 40

function UpdateWeaponExplodeNova(playerCmp, playerTransform)
	local currentTime = Time.GetElapsedTime()
	if(playerTransform ~= nil) then 
		if(currentTime - lastNovaTime > (novaFrequency * playerCmp:GetCooldownMultiplier())) then 
			DoExplosionAt(playerTransform:GetPosition(), novaArea * playerCmp:GetAreaMultiplier(), novaDamage * playerCmp:GetDamageMultiplier())
			lastNovaTime = currentTime
		end
	end
end