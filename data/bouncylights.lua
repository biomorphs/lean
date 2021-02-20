BouncyLights = {}

local LightShader = {}
local LightModel = {}

local Lights = {}
local lightCount = 1
local lightMaxCount = 32
local lightBoxMin = {-284,1,-125}
local lightBoxMax = {256,100,113}
local lightRadiusRange = {64,256}
local lightGravity = -4096.0
local lightBounceMul = 0.5
local lightFriction = 0.95
local lightHistoryMaxValues = 32
local lightHistoryMaxDistance = 0.25		-- distance between history points
local lightXZSpeed = 200
local lightYSpeed = 200
local lightBrightness = 8.0
local lightSphereSize = 1.0
local timeMulti = 0.2

function BouncyLights.Vec3Length(self,v)
	return math.sqrt((v[1] * v[1]) + (v[2] * v[2]) + (v[3] * v[3]));
end

function BouncyLights.pushLightHistory(self,i,x,y,z)
	local historySize = #Lights[i].History
	if(historySize == lightHistoryMaxValues) then
		table.remove(Lights[i].History,1)
	end
	Lights[i].History[#Lights[i].History + 1] = {x,y,z}
end

function BouncyLights.GenerateLightCol(self,i)
	local colour = {0.0,0.0,0.0}
	while colour[1] == 0 and colour[2] == 0 and colour[3] == 0 do
		colour = { lightBrightness * math.random(0,10) / 10, lightBrightness * math.random(0,10) / 10, lightBrightness * math.random(0,10) / 10 }
	end
	Lights[i].Colour = colour
end

function BouncyLights.InitLight(self,i)
	Lights[i] = {}
	Lights[i].Position = {math.random(lightBoxMin[1],lightBoxMax[1]),math.random(lightBoxMin[2],lightBoxMax[2]),math.random(lightBoxMin[3],lightBoxMax[3])}
	BouncyLights:GenerateLightCol(i)
	Lights[i].Ambient = 0.0
	Lights[i].Velocity = {0.0,0.0,0.0}
	Lights[i].Distance = math.random(lightRadiusRange[1],lightRadiusRange[2])
	Lights[i].Attenuation = 1.5
	Lights[i].History = {}
	BouncyLights:pushLightHistory(i,Lights[i].Position[1], Lights[i].Position[2], Lights[i].Position[3])
end

function BouncyLights.Init()
	LightShader = Graphics.LoadShader("light",  "basic.vs", "basic.fs")
	LightModel = Graphics.LoadModel("sphere.fbx")

	for i=1,lightMaxCount do
		BouncyLights:InitLight(i)
	end

	Graphics.SetClearColour(0.3,0.55,0.8)
end

function BouncyLights.Tick(DeltaTime)
	local isOpen = true
	DebugGui.BeginWindow(isOpen, "Bouncy Lights")
	timeMulti = DebugGui.DragFloat("Time Multi", timeMulti, 0.002, 0.0, 10.0)
	lightCount = DebugGui.DragFloat("Light Count", lightCount, 1,1,lightMaxCount)
	DebugGui.EndWindow()
	
	local timeDelta = DeltaTime * timeMulti
	for i=1,lightCount do
		if(BouncyLights:Vec3Length(Lights[i].Velocity) < 16.0) then
			Lights[i].Velocity[1] = (math.random(-200,200) / 100.0)  * lightXZSpeed
			Lights[i].Velocity[2] = (math.random(200,400) / 100.0)  * lightYSpeed
			Lights[i].Velocity[3] = (math.random(-200,200) / 100.0)  * lightXZSpeed
			BouncyLights:GenerateLightCol(i)
		end

		local lastHistoryPos = Lights[i].History[#Lights[i].History]
		local travelled = {}
		travelled[1] = Lights[i].Position[1] - lastHistoryPos[1]
		travelled[2] = Lights[i].Position[2] - lastHistoryPos[2]
		travelled[3] = Lights[i].Position[3] - lastHistoryPos[3]
		local lastHistoryDistance = BouncyLights:Vec3Length(travelled)
		if(lastHistoryDistance > lightHistoryMaxDistance) then
			BouncyLights:pushLightHistory(i,Lights[i].Position[1], Lights[i].Position[2], Lights[i].Position[3])
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
		
		Graphics.PointLight(Lights[i].Position[1],Lights[i].Position[2],Lights[i].Position[3],Lights[i].Colour[1],Lights[i].Colour[2],Lights[i].Colour[3], Lights[i].Ambient, Lights[i].Distance, Lights[i].Attenuation)
		Graphics.DrawModel(Lights[i].Position[1],Lights[i].Position[2],Lights[i].Position[3],Lights[i].Colour[1],Lights[i].Colour[2],Lights[i].Colour[3],1.0,lightSphereSize,LightModel,LightShader)

		for h=1,#Lights[i].History-1 do
			local alpha = (1.0 - (((#Lights[i].History - h) / #Lights[i].History)))  * 0.5
			Graphics.DebugDrawLine(Lights[i].History[h][1],Lights[i].History[h][2],Lights[i].History[h][3],
								   Lights[i].History[h+1][1],Lights[i].History[h+1][2],Lights[i].History[h+1][3],
								   Lights[i].Colour[1],Lights[i].Colour[2],Lights[i].Colour[3],alpha,
								   Lights[i].Colour[1],Lights[i].Colour[2],Lights[i].Colour[3],alpha)
		end
	end
end

return BouncyLights