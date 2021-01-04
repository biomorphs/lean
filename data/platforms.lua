Platforms = {}

local DiffuseShader = {}
local Floor4x4Model = {}

local SunMulti = 0.8
local SunPosition = {250,400,-200}
local SunColour = {0.55, 0.711, 1.0}
local SunAmbient = 0.3

function Platforms.Init()
	ShadowShader = Graphics.LoadShader("shadow", "simpleshadow.vs", "simpleshadow.fs");
	DiffuseShader = Graphics.LoadShader("diffuse", "simplediffuse.vs", "simplediffuse.fs")
	Floor4x4Model = Graphics.LoadModel("floor_4x4.fbx")
	Graphics.SetShadowShader(DiffuseShader, ShadowShader)
	Graphics.SetClearColour(0.3,0.55,0.8)
end

function Platforms.Tick(deltaTime)
	Graphics.DirectionalLight(SunPosition[1],SunPosition[2],SunPosition[3], SunMulti * SunColour[1], SunMulti * SunColour[2], SunMulti * SunColour[3], SunAmbient)
	Graphics.DebugDrawAxis(0.0,8.0,0.0,8.0)

	for x=0,3 do
		for z=0,3 do
			Graphics.DrawModel(x * 8,0.0,z * 8,1.0,1.0,1.0,1.0,1.0,Floor4x4Model,DiffuseShader)
		end
	end

	for x=0,3 do
		for z=0,3 do
			Graphics.DrawModel(80 + x * 8,0.0,z * 8,1.0,1.0,1.0,1.0,1.0,Floor4x4Model,DiffuseShader)
		end
	end

	for x=0,11 do
		for z=0,1 do
			Graphics.DrawModel(8 + x * 8,-16.0,8 + z * 8,1.0,1.0,1.0,1.0,1.0,Floor4x4Model,DiffuseShader)
		end
	end

	for x=0,32 do
		for z=0,16 do
			Graphics.DrawModel(-64 + x * 8,-24.0,-64 + z * 8,1.0,1.0,1.0,1.0,1.0,Floor4x4Model,DiffuseShader)
		end
	end

	for y=0,32 do
		Graphics.DrawModel(16,y * 2,8,1.0,1.0,1.0,1.0,1.0,Floor4x4Model,DiffuseShader)
	end

	for y=0,32 do
		Graphics.DrawModel(96,y * 2,8,1.0,1.0,1.0,1.0,1.0,Floor4x4Model,DiffuseShader)
	end

	for y=-8,0 do
		Graphics.DrawModel(16,y * 2,12,1.0,1.0,1.0,1.0,1.0,Floor4x4Model,DiffuseShader)
	end

	for y=-8,0 do
		Graphics.DrawModel(96,y * 2,12,1.0,1.0,1.0,1.0,1.0,Floor4x4Model,DiffuseShader)
	end

	for y=0,4 do
		Graphics.DrawModel(48,-24 + y * 2,12,1.0,1.0,1.0,1.0,1.0,Floor4x4Model,DiffuseShader)
	end

end

return Platforms