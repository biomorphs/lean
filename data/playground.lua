Playground = {}

local DiffuseShader = {}
local ShadowShader = {}
local Sponza = {}
local Container = {}

local SunMulti = 0.3
local SunPosition = {200,800,-40}
local SunColour = {0.55, 0.711, 1.0}
local SunAmbient = 0.3

function Playground.Init()
	DiffuseShader = Graphics.LoadShader("diffuse", "simplediffuse.vs", "simplediffuse.fs")
	ShadowShader = Graphics.LoadShader("shadow", "simpleshadow.vs", "simpleshadow.fs");
	Sponza = Graphics.LoadModel("sponza.obj")
	Container = Graphics.LoadModel("container.fbx")

	Graphics.SetClearColour(0.3,0.55,0.8)
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

function Playground.Tick(DeltaTime)
	DrawGrid(-512,512,32,-512,512,32,-40.0)
	Graphics.DebugDrawAxis(0.0,32.0,0.0,8.0)

	Graphics.DirectionalLight(SunPosition[1],SunPosition[2],SunPosition[3], SunMulti * SunColour[1], SunMulti * SunColour[2], SunMulti * SunColour[3], SunAmbient)
	Graphics.DrawModel(0.0,0.5,0.0,0.2,Sponza,DiffuseShader)

	local attenua = {1.0,0.14,0.07}
	local brightness = 8
	Graphics.PointLight(-123, 32, -40, 1.0 * brightness, 0.61 * brightness, 0.17 * brightness, 0.01, attenua[1], attenua[2], attenua[3])
	Graphics.PointLight(98, 32, -40, 1.0 * brightness, 0.61 * brightness, 0.17 * brightness, 0.01, attenua[1], attenua[2], attenua[3])
	Graphics.PointLight(-123.9, 32.18, 28, 1.0 * brightness, 0.61 * brightness, 0.17 * brightness, 0.01, attenua[1], attenua[2], attenua[3])
	Graphics.PointLight(98.9, 32.18, 28, 1.0 * brightness, 0.61 * brightness, 0.17 * brightness, 0.01, attenua[1], attenua[2], attenua[3])
	
	Graphics.PointLight(-241.25, 33.5, -88.3, 0.16 * brightness, 0.73 * brightness, 1.0 * brightness, 0.01, attenua[1], attenua[2], attenua[3])
	Graphics.PointLight(-241.25, 33.5, 82.3, 0.16 * brightness, 0.73 * brightness, 1.0 * brightness, 0.01, attenua[1], attenua[2], attenua[3])
	Graphics.PointLight(226.25, 33.5, -88.3, 0.16 * brightness, 0.73 * brightness, 1.0 * brightness, 0.01, attenua[1], attenua[2], attenua[3])
	Graphics.PointLight(226.25, 33.5, 82.3, 0.16 * brightness, 0.73 * brightness, 1.0 * brightness, 0.01, attenua[1], attenua[2], attenua[3])

	local width = 320
	local numPerWidth = 8
	local scale = 6
	local halfWidth = width / 2.0
	local gap = width / numPerWidth
	for z=1,numPerWidth do
		for x=1,numPerWidth do
			Graphics.DrawModel(-halfWidth + (x * gap),0.0,-halfWidth + (z*gap) - 14,scale,Container,DiffuseShader)
		end
	end
end

return Playground