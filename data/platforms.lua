Platforms = {}

local DiffuseShader = {}
local Floor4x4Model = {}
local Wall1x4x4Model = {}

local PlatformCount = 6
local SunMulti = 2.0
local SunPosition = {250,400,-200}
local SunColour = {0.55, 0.711, 1.0}
local SunAmbient = 0.3

function Platforms.Floor(x,y,z)
	local newEntity = World.AddEntity()
	local transform = World.AddComponent_Transform(newEntity)
	transform:SetPosition(x,y,z)
	local newModel = World.AddComponent_Model(newEntity)
	newModel:SetModel(Floor4x4Model)
	newModel:SetShader(DiffuseShader)
end

function Platforms.Wall(x,y,z)
end

function Platforms.Init()
	ShadowShader = Graphics.LoadShader("shadow", "simpleshadow.vs", "simpleshadow.fs");
	DiffuseShader = Graphics.LoadShader("diffuse", "simplediffuse.vs", "simplediffuse.fs")
	Floor4x4Model = Graphics.LoadModel("floor_4x4.fbx")
	Wall1x4x4Model = Graphics.LoadModel("wall_1x4x4.fbx")
	Graphics.SetShadowShader(DiffuseShader, ShadowShader)
	Graphics.SetClearColour(0.3,0.55,0.8)
	
	local PlatformHeight = 80
	local PlatformWidth = 400
	local startPosY = 1 + -PlatformCount * (PlatformHeight/2)
	local startPosX = 1 + -PlatformCount * (PlatformWidth/2)
	
	for XOffset = startPosX,-startPosX,PlatformWidth do 
		for FloorOffset = startPosY,-startPosY,PlatformHeight do 
		
			for z=0,4 do
				for x=0,4 do
					Platforms.Floor(XOffset + 32 + x * 8,-16 + FloorOffset, -32 + z * 8)
				end
				Graphics.DrawModel(XOffset + 32,-16 + FloorOffset, -32 + z * 8,1.0,1.0,1.0,1.0,1.0,Wall1x4x4Model,DiffuseShader)
				Graphics.DrawModel(XOffset + 64,-16 + FloorOffset, -32 + z * 8,1.0,1.0,1.0,1.0,1.0,Wall1x4x4Model,DiffuseShader)
			end
	
			for x=0,3 do
				for z=0,3 do
					Platforms.Floor(XOffset + x * 8,0.0 + FloorOffset,z * 8)
				end
			end
	
			for x=0,3 do
				for z=0,3 do
					Platforms.Floor(XOffset + 80 + x * 8,0.0 + FloorOffset,z * 8)
				end
			end
	
			for x=0,11 do
				for z=0,1 do
					Platforms.Floor(XOffset + 8 + x * 8,-16.0 + FloorOffset,8 + z * 8)
				end
			end
	
			for x=0,32 do
				for z=0,16 do
					Platforms.Floor(XOffset + -64 + x * 8,-24.0 + FloorOffset,-64 + z * 8)
				end
			end
	
			for y=0,32 do
				Platforms.Floor(XOffset + 16,y * 2 + FloorOffset,8)
			end
	
			for y=0,32 do
				Platforms.Floor(XOffset + 96,y * 2 + FloorOffset,8)
			end
	
			for y=-8,0 do
				Platforms.Floor(XOffset + 16,y * 2 + FloorOffset,12)
			end
	
			for y=-8,0 do
				Platforms.Floor(XOffset + 96,y * 2 + FloorOffset,12)
			end
	
			for y=0,4 do
				Platforms.Floor(XOffset + 48,-24 + y * 2 + FloorOffset,12)
			end
			
		end
	end
end

function Platforms.Tick(deltaTime)
	Graphics.DirectionalLight(SunPosition[1],SunPosition[2],SunPosition[3], SunMulti * SunColour[1], SunMulti * SunColour[2], SunMulti * SunColour[3], SunAmbient)
	Graphics.DebugDrawAxis(0.0,8.0,0.0,8.0)
	
	-- local isOpen = true
	-- DebugGui.BeginWindow(isOpen, "Platforms")
	-- PlatformCount = DebugGui.DragFloat("Count", PlatformCount, 1,1,8)
	-- DebugGui.EndWindow()
	
	-- local PlatformHeight = 80
	-- local PlatformWidth = 400
	-- local startPosY = 1 + -PlatformCount * (PlatformHeight/2)
	-- local startPosX = 1 + -PlatformCount * (PlatformWidth/2)
	-- 
	-- for XOffset = startPosX,-startPosX,PlatformWidth do 
	-- 	for FloorOffset = startPosY,-startPosY,PlatformHeight do 
	-- 	
	-- 		for z=0,4 do
	-- 			for x=0,4 do
	-- 				Graphics.DrawModel(XOffset + 32 + x * 8,-16 + FloorOffset, -32 + z * 8,1.0,1.0,1.0,1.0,1.0,Floor4x4Model,DiffuseShader)
	-- 			end
	-- 			Graphics.DrawModel(XOffset + 32,-16 + FloorOffset, -32 + z * 8,1.0,1.0,1.0,1.0,1.0,Wall1x4x4Model,DiffuseShader)
	-- 			Graphics.DrawModel(XOffset + 64,-16 + FloorOffset, -32 + z * 8,1.0,1.0,1.0,1.0,1.0,Wall1x4x4Model,DiffuseShader)
	-- 		end
	-- 
	-- 		for x=0,3 do
	-- 			for z=0,3 do
	-- 				Graphics.DrawModel(XOffset + x * 8,0.0 + FloorOffset,z * 8,1.0,1.0,1.0,1.0,1.0,Floor4x4Model,DiffuseShader)
	-- 			end
	-- 		end
	-- 
	-- 		for x=0,3 do
	-- 			for z=0,3 do
	-- 				Graphics.DrawModel(XOffset + 80 + x * 8,0.0 + FloorOffset,z * 8,1.0,1.0,1.0,1.0,1.0,Floor4x4Model,DiffuseShader)
	-- 			end
	-- 		end
	-- 
	-- 		for x=0,11 do
	-- 			for z=0,1 do
	-- 				Graphics.DrawModel(XOffset + 8 + x * 8,-16.0 + FloorOffset,8 + z * 8,1.0,1.0,1.0,1.0,1.0,Floor4x4Model,DiffuseShader)
	-- 			end
	-- 		end
	-- 
	-- 		for x=0,32 do
	-- 			for z=0,16 do
	-- 				Graphics.DrawModel(XOffset + -64 + x * 8,-24.0 + FloorOffset,-64 + z * 8,1.0,1.0,1.0,1.0,1.0,Floor4x4Model,DiffuseShader)
	-- 			end
	-- 		end
	-- 
	-- 		for y=0,32 do
	-- 			Graphics.DrawModel(XOffset + 16,y * 2 + FloorOffset,8,1.0,1.0,1.0,1.0,1.0,Floor4x4Model,DiffuseShader)
	-- 		end
	-- 
	-- 		for y=0,32 do
	-- 			Graphics.DrawModel(XOffset + 96,y * 2 + FloorOffset,8,1.0,1.0,1.0,1.0,1.0,Floor4x4Model,DiffuseShader)
	-- 		end
	-- 
	-- 		for y=-8,0 do
	-- 			Graphics.DrawModel(XOffset + 16,y * 2 + FloorOffset,12,1.0,1.0,1.0,1.0,1.0,Floor4x4Model,DiffuseShader)
	-- 		end
	-- 
	-- 		for y=-8,0 do
	-- 			Graphics.DrawModel(XOffset + 96,y * 2 + FloorOffset,12,1.0,1.0,1.0,1.0,1.0,Floor4x4Model,DiffuseShader)
	-- 		end
	-- 
	-- 		for y=0,4 do
	-- 			Graphics.DrawModel(XOffset + 48,-24 + y * 2 + FloorOffset,12,1.0,1.0,1.0,1.0,1.0,Floor4x4Model,DiffuseShader)
	-- 		end
	-- 		
	-- 	end
	-- end
end

return Platforms