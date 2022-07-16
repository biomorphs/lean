
function FollowPlayer(entity)
	local myTransform = World.GetComponent_Transform(entity)
	if(myTransform == nil) then
		return
	end 
	
	local foundPlayer = World.GetFirstEntityWithTag(Tag.new("PlayerCharacter"))
	local playerTransform = World.GetComponent_Transform(foundPlayer)
	if(foundPlayer == nil) then 
		return
	end
	local playerPos = playerTransform:GetPosition()
	local myPos = myTransform:GetPosition()
	myTransform:SetPosition(playerPos.x, playerPos.y + 184.5, playerPos.z - 184.75)
end

return FollowPlayer