EntityTest = {}

function EntityTest.Init()
	print("Entity Test")
	newEntity = World.AddEntity()
	print(newEntity:GetID())
	
	newEntity2 = World.AddEntity()
	print(newEntity2:GetID())
	
	testComponent = World.AddComponent_TestComponent(newEntity)
	print(testComponent:GetType())
	print(testComponent:GetString())
	testComponent:SetString("Balls")
	print(testComponent:GetString())

	World.AddComponent_ScriptedComponent(newEntity)
	World.AddComponent_ScriptedComponent(newEntity2)
end

function EntityTest.Tick(deltaTime)
end

return EntityTest