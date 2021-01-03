-- Playground expects a script to create a global object named g_playground with the following functions:
-- Init
-- Tick
-- Shutdown

Playground = {}

local LightShader = Graphics.LoadShader("light",  "basic.vs", "basic.fs")
local DiffuseShader = Graphics.LoadShader("diffuse", "simplediffuse.vs", "simplediffuse.fs")
local ShadowShader = Graphics.LoadShader("shadow", "simpleshadow.vs", "simpleshadow.fs");
local Sponza = Graphics.LoadModel("sponza.obj")
local LightModel = Graphics.LoadModel("sphere.fbx")
local IslandModel = Graphics.LoadModel("islands_low.fbx")
local Container = Graphics.LoadModel("container.fbx")

local Lights = {}
local lightCount = 1
local lightMaxCount = 32
local lightBoxMin = {-284,1,-125}
local lightBoxMax = {256,100,113}
local lightRadiusRange = {96,128}
local lightGravity = -4096.0
local lightBounceMul = 0.5
local lightFriction = 0.95
local lightHistoryMaxValues = 32
local lightHistoryMaxDistance = 0.25		-- distance between history points
local lightXZSpeed = 200
local lightYSpeed = 200
local lightBrightness = 8.0
local lightSphereSize = 1.0
local SunMulti = 0.2
local SunPosition = {200,800,-40}
local SunColour = {0.55, 0.711, 1.0}
local SunAmbient = 0.3
local timeMulti = 0.2

-- ~distance, const, linear, quad
local lightAttenuationTable = {
	{7, 1.0, 0.7,1.8},
	{13,1.0,0.35 ,0.44},
	{20,1.0,0.22 ,0.20},
	{32,1.0,0.14,0.07},
	{50,1.0,0.09,0.032},
	{65,1.0,0.07,0.017},
	{100,1.0,0.045,0.0075},
	{160,1.0,0.027,0.0028},
	{200,1.0,0.022,0.0019},
	{325,1.0,0.014,0.0007},
	{600,1.0,0.007,0.0002},
	{3250,1.0,0.0014,0.000007}
}

function Playground:Vec3Length(v)
	return math.sqrt((v[1] * v[1]) + (v[2] * v[2]) + (v[3] * v[3]));
end

function Playground:pushLightHistory(i,x,y,z)
	local historySize = #Lights[i].History
	if(historySize == lightHistoryMaxValues) then
		table.remove(Lights[i].History,1)
	end
	Lights[i].History[#Lights[i].History + 1] = {x,y,z}
end

function Playground:GetAttenuation(lightRadius)
	local closestDistance = 10000
	local closestValue = {1,1,0.1,0.1}
	for i=1,#lightAttenuationTable do
		local distanceToRadInTable = math.abs(lightRadius - lightAttenuationTable[i][1])
		if(distanceToRadInTable<closestDistance) then
			closestValue = lightAttenuationTable[i];
			closestDistance = distanceToRadInTable
		end
	end
	return { closestValue[2], closestValue[3], closestValue[4] }
end

function Playground:GenerateLightCol(i)
	local colour = {0.0,0.0,0.0}
	while colour[1] == 0 and colour[2] == 0 and colour[3] == 0 do
		colour = { lightBrightness * math.random(0,10) / 10, lightBrightness * math.random(0,10) / 10, lightBrightness * math.random(0,10) / 10 }
	end
	Lights[i].Colour = colour
end

function Playground:InitLight(i)
	Lights[i] = {}
	Lights[i].Position = {math.random(lightBoxMin[1],lightBoxMax[1]),math.random(lightBoxMin[2],lightBoxMax[2]),math.random(lightBoxMin[3],lightBoxMax[3])}
	Playground:GenerateLightCol(i)
	Lights[i].Ambient = 0.0
	Lights[i].Velocity = {0.0,0.0,0.0}
	Lights[i].Attenuation = Playground:GetAttenuation(math.random(lightRadiusRange[1],lightRadiusRange[2]))
	Lights[i].History = {}
	Playground:pushLightHistory(i,Lights[i].Position[1], Lights[i].Position[2], Lights[i].Position[3])
end

function Playground:Init()
	local lightColourAccum = {0.0,0.0,0.0}
	for i=1,lightMaxCount do
		Playground:InitLight(i)
	end

	Graphics.SetClearColour(0.1,0.1,0.1)
	Graphics.SetShadowShader(DiffuseShader, ShadowShader)
end

function DrawGrid(startX,endX,stepX,startZ,endZ,stepZ,yAxis)
	for x=startX,endX,stepX do
		Graphics.DebugDrawLine(x,yAxis,startZ,x,yAxis,endZ,0.2,0.2,0.2,1.0,0.2,0.2,0.2,1.0)
	end
	for z=startZ,endZ,stepZ do
		Graphics.DebugDrawLine(startX,yAxis,z,endX,yAxis,z,0.2,0.2,0.2,1.0,0.2,0.2,0.2,1.0)
	end
end

function Playground:Tick()
	local isOpen = true
	DebugGui.BeginWindow(isOpen, "Script Stuff")
	timeMulti = DebugGui.DragFloat("Time Multi", timeMulti, 0.002, 0.0, 10.0)
	SunPosition[1] = DebugGui.DragFloat("Sun X", SunPosition[1], 0.25,-400,400)
	SunPosition[2] = DebugGui.DragFloat("Sun Y", SunPosition[2], 0.25,-200,800)
	SunPosition[3] = DebugGui.DragFloat("Sun Z", SunPosition[3], 0.25,-200,200)
	lightCount = DebugGui.DragFloat("Light Count", lightCount, 1,1,lightMaxCount)
	DebugGui.EndWindow()
	local timeDelta = Playground.DeltaTime * timeMulti

	Graphics.DrawModel(SunPosition[1],SunPosition[2],SunPosition[3],SunMulti * SunColour[1],SunMulti * SunColour[2], SunMulti * SunColour[3],1.0,20.0,LightModel,LightShader)
	Graphics.DirectionalLight(SunPosition[1],SunPosition[2],SunPosition[3], SunMulti * SunColour[1], SunMulti * SunColour[2], SunMulti * SunColour[3], SunAmbient)

	for i=1,lightCount do
		if(Playground:Vec3Length(Lights[i].Velocity) < 16.0) then
			Lights[i].Velocity[1] = (math.random(-100,100) / 100.0)  * lightXZSpeed
			Lights[i].Velocity[2] = (math.random(200,400) / 100.0)  * lightYSpeed
			Lights[i].Velocity[3] = (math.random(-100,100) / 100.0)  * lightXZSpeed
			Playground:GenerateLightCol(i)
		end

		local lastHistoryPos = Lights[i].History[#Lights[i].History]
		local travelled = {}
		travelled[1] = Lights[i].Position[1] - lastHistoryPos[1]
		travelled[2] = Lights[i].Position[2] - lastHistoryPos[2]
		travelled[3] = Lights[i].Position[3] - lastHistoryPos[3]
		local lastHistoryDistance = Playground:Vec3Length(travelled)
		if(lastHistoryDistance > lightHistoryMaxDistance) then
			Playground:pushLightHistory(i,Lights[i].Position[1], Lights[i].Position[2], Lights[i].Position[3])
		end

		Lights[i].Velocity[2] = Lights[i].Velocity[2] + (lightGravity * timeDelta);
		Lights[i].Position[1] = Lights[i].Position[1] + Lights[i].Velocity[1] * timeDelta
		Lights[i].Position[2] = Lights[i].Position[2] + Lights[i].Velocity[2] * timeDelta
		Lights[i].Position[3] = Lights[i].Position[3] + Lights[i].Velocity[3] * timeDelta

		if(Lights[i].Position[1] < lightBoxMin[1]) then
			Lights[i].Position[1] = lightBoxMin[1]
			Lights[i].Velocity[1] = -Lights[i].Velocity[1] * lightBounceMul
		end
		if(Lights[i].Position[2] < lightBoxMin[2]) then
			Lights[i].Position[2] = lightBoxMin[2]
			Lights[i].Velocity[1] = Lights[i].Velocity[1] * lightFriction
			Lights[i].Velocity[2] = -Lights[i].Velocity[2] * lightBounceMul
			Lights[i].Velocity[3] = Lights[i].Velocity[3] * lightFriction
		end
		if(Lights[i].Position[3] < lightBoxMin[3]) then
			Lights[i].Position[3] = lightBoxMin[3]
			Lights[i].Velocity[3] = -Lights[i].Velocity[3] * lightBounceMul
		end
		if(Lights[i].Position[1] > lightBoxMax[1]) then
			Lights[i].Position[1] = lightBoxMax[1]
			Lights[i].Velocity[1] = -Lights[i].Velocity[1] * lightBounceMul
		end
		if(Lights[i].Position[2] > lightBoxMax[2]) then
			Lights[i].Position[2] = lightBoxMax[2]
			Lights[i].Velocity[2] = -Lights[i].Velocity[2] * lightBounceMul
		end
		if(Lights[i].Position[3] > lightBoxMax[3]) then
			Lights[i].Position[3] = lightBoxMax[3]
			Lights[i].Velocity[3] = -Lights[i].Velocity[3] * lightBounceMul
		end

		Graphics.PointLight(Lights[i].Position[1],Lights[i].Position[2],Lights[i].Position[3],Lights[i].Colour[1],Lights[i].Colour[2],Lights[i].Colour[3], Lights[i].Ambient, Lights[i].Attenuation[1], Lights[i].Attenuation[2], Lights[i].Attenuation[3])
		Graphics.DrawModel(Lights[i].Position[1],Lights[i].Position[2],Lights[i].Position[3],Lights[i].Colour[1],Lights[i].Colour[2],Lights[i].Colour[3],1.0,lightSphereSize,LightModel,LightShader)

		for h=1,#Lights[i].History-1 do
			local alpha = (1.0 - (((#Lights[i].History - h) / #Lights[i].History)))  * 0.5
			Graphics.DebugDrawLine(Lights[i].History[h][1],Lights[i].History[h][2],Lights[i].History[h][3],
								   Lights[i].History[h+1][1],Lights[i].History[h+1][2],Lights[i].History[h+1][3],
								   Lights[i].Colour[1],Lights[i].Colour[2],Lights[i].Colour[3],alpha,
								   Lights[i].Colour[1],Lights[i].Colour[2],Lights[i].Colour[3],alpha)
		end
	end

	DrawGrid(-512,512,32,-512,512,32,-40.0)
	Graphics.DebugDrawAxis(0.0,32.0,0.0,8.0)

	Graphics.DrawModel(0.0,1.5,0.0,1.0,1.0,1.0,1.0,0.4,IslandModel,DiffuseShader)
	Graphics.DrawModel(0.0,0.5,0.0,1.0,1.0,1.0,1.0,0.2,Sponza,DiffuseShader)

	local attenua = Playground:GetAttenuation(32);
	local brightness = 8
	Graphics.PointLight(-123, 32, -40, 1.0 * brightness, 0.61 * brightness, 0.17 * brightness, 0.01, attenua[1], attenua[2], attenua[3])
	Graphics.PointLight(98, 32, -40, 1.0 * brightness, 0.61 * brightness, 0.17 * brightness, 0.01, attenua[1], attenua[2], attenua[3])
	Graphics.PointLight(-123.9, 32.18, 28, 1.0 * brightness, 0.61 * brightness, 0.17 * brightness, 0.01, attenua[1], attenua[2], attenua[3])
	Graphics.PointLight(98.9, 32.18, 28, 1.0 * brightness, 0.61 * brightness, 0.17 * brightness, 0.01, attenua[1], attenua[2], attenua[3])
	
	Graphics.PointLight(-241.25, 33.5, -88.3, 0.16 * brightness, 0.73 * brightness, 1.0 * brightness, 0.01, attenua[1], attenua[2], attenua[3])
	Graphics.PointLight(-241.25, 33.5, 82.3, 0.16 * brightness, 0.73 * brightness, 1.0 * brightness, 0.01, attenua[1], attenua[2], attenua[3])
	Graphics.PointLight(226.25, 33.5, -88.3, 0.16 * brightness, 0.73 * brightness, 1.0 * brightness, 0.01, attenua[1], attenua[2], attenua[3])
	Graphics.PointLight(226.25, 33.5, 82.3, 0.16 * brightness, 0.73 * brightness, 1.0 * brightness, 0.01, attenua[1], attenua[2], attenua[3])

	local width = 64
	local numPerWidth = 4
	local scale = 1
	local halfWidth = width / 2.0
	local gap = width / numPerWidth
	for z=1,numPerWidth do
		for x=1,numPerWidth do
			Graphics.DrawModel(-halfWidth + (x * gap),0.0,-halfWidth + (z*gap) - 14,1.0,1.0,1.0,1.0,1.0 * scale,Container,DiffuseShader)
		end
	end
end

function Playground:Shutdown()
end

function Playground:new()
	newPlayground = {}
	self.__index = self
	return setmetatable(newPlayground, self)
end
g_playground = Playground:new()