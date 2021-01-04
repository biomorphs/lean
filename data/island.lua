Island = {}

local DiffuseShader = {}
local IslandModel = {}

local SunMulti = 0.8
local SunPosition = {200,800,-40}
local SunColour = {0.55, 0.711, 1.0}
local SunAmbient = 0.3

function Island.Init()
	ShadowShader = Graphics.LoadShader("shadow", "simpleshadow.vs", "simpleshadow.fs");
	DiffuseShader = Graphics.LoadShader("diffuse", "simplediffuse.vs", "simplediffuse.fs")
	IslandModel = Graphics.LoadModel("islands_low.fbx")
	Graphics.SetShadowShader(DiffuseShader, ShadowShader)
	Graphics.SetClearColour(0.3,0.55,0.8)
end

function Island.Tick(deltaTime)
	Graphics.DirectionalLight(SunPosition[1],SunPosition[2],SunPosition[3], SunMulti * SunColour[1], SunMulti * SunColour[2], SunMulti * SunColour[3], SunAmbient)
	Graphics.DrawModel(0.0,1.5,0.0,1.0,1.0,1.0,1.0,0.4,IslandModel,DiffuseShader)
end

return Island