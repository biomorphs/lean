Marbles = {}

local AllMarbles = {}
local Spinners = {}
local MainCamera = {}
local DeathCam = {}
local WinCam = {}
local CameraSpeed = 1
local TargetAreas = {}	-- {pos, dims, marble}
local ShowYouWin = false
local ShowEndScreen = false

function Marbles.Init()
	Graphics.SetClearColour(0.3,0.55,0.8)
	Physics.SetGravity(0,-30,0)
	MakeSunEntity()
	
	MainCamera = MakeCamera("Main Camera")
	DeathCam = MakeCamera("Death Cam")
	World.GetComponent_Transform(DeathCam):SetPosition(0,-5,0)
	WinCam = MakeCamera("Win Cam")
	World.GetComponent_Transform(WinCam):SetPosition(-59,62,60)
	World.GetComponent_Transform(WinCam):SetRotation(-120,-30,140)
	
	Graphics.SetActiveCamera(MainCamera)
	
	MakeMaze()
	MakeMarble({54,1.5,50},1, {1.2,0,4})
	MakeMarble({-54,1.5,-50},1, {1.5,1.5,0})
	
	MakeMarbleTarget({51.3,-0.15,51.25}, {17,0.5,17}, {1.5,1.5,0}, AllMarbles[2])
	MakeMarbleTarget({-53.4,-0.15,-52.25}, {13,0.5,15}, {1.2,0,4}, AllMarbles[1])
end

function Marbles.Tick(deltaTime)
	UpdateCamera(deltaTime)

	local p = World.GetComponent_Physics(AllMarbles[1])
	if(p ~= nil) then 
		if(Input.IsKeyPressed("KEY_UP")) then 
			p:AddForce(vec3.new(0,0,-50))
		end
		if(Input.IsKeyPressed("KEY_DOWN")) then 
			p:AddForce(vec3.new(0,0,50))
		end
		if(Input.IsKeyPressed("KEY_LEFT")) then 
			p:AddForce(vec3.new(-50,0,0))
		end
		if(Input.IsKeyPressed("KEY_RIGHT")) then 
			p:AddForce(vec3.new(50,0,0))
		end
	end
	
	local p = World.GetComponent_Physics(AllMarbles[2])
	if(p ~= nil) then 
		if(Input.IsKeyPressed("KEY_w")) then 
			p:AddForce(vec3.new(0,0,-50))
		end
		if(Input.IsKeyPressed("KEY_s")) then 
			p:AddForce(vec3.new(0,0,50))
		end
		if(Input.IsKeyPressed("KEY_a")) then 
			p:AddForce(vec3.new(-50,0,0))
		end
		if(Input.IsKeyPressed("KEY_d")) then 
			p:AddForce(vec3.new(50,0,0))
		end
	end
	
	for i=1,#AllMarbles do 
		local t = World.GetComponent_Transform(AllMarbles[i])
		if(t ~= nil) then 
			local mp = t:GetPosition()
			if(mp.y < -10) then 
				ShowEndScreen = true
			end
		end
	end
	
	DebugGui.BeginWindow(true, "Instructions")
		DebugGui.Text("Your goal is to get the marbles to the matching boxes")
		DebugGui.Text("WSAD - Control Marble 1")
		DebugGui.Text("Up/Down/Left/Right arrows - Control Marble 2")
		DebugGui.Text("Good luck!")
	DebugGui.EndWindow()
	
	if(AllMarblesInTargets()) then 
		ShowYouWin=true
		Graphics.SetActiveCamera(WinCam)
	end
	
	if(ShowYouWin) then 
		DebugGui.BeginWindow(true, "You did it!")
		if(DebugGui.Button("Woo! Click to restart!")) then 
			Playground.ReloadScripts()
		end
		DebugGui.EndWindow()
	end
	
	if(ShowEndScreen) then 
		Graphics.SetActiveCamera(DeathCam)
		DebugGui.BeginWindow(true, "Game over, man! Game Over!")
		if(DebugGui.Button("You suck! Click to restart!")) then 
			Playground.ReloadScripts()
		end
		DebugGui.EndWindow()
	end
	
	for i=1,#Spinners do 
		local t = World.GetComponent_Transform(Spinners[i][1])
		if(t ~= nil) then 
			t:RotateEuler(vec3.new(0,Spinners[i][2] * deltaTime, 0))
		end
	end
end

function UpdateCamera(deltaTime)
	-- camera tries to keep equidistant from all marbles
	-- y axis based on distance between marbles
	
	local ct = World.GetComponent_Transform(MainCamera)
	local camerapos = ct:GetPosition()
	local averagepos = {0,0,0}
	local minpos = {1000,1000}
	local maxpos = {-1000,-1000}
	for i=1,#AllMarbles do 
		local t = World.GetComponent_Transform(AllMarbles[i])
		if(t ~= nil) then 
			local mp = t:GetPosition()
			averagepos[1] = averagepos[1] + mp.x
			averagepos[2] = averagepos[2] + mp.y
			averagepos[3] = averagepos[3] + mp.z
			if(mp.x < minpos[1]) then minpos[1] = mp.x end
			if(mp.z < minpos[2]) then minpos[2] = mp.z end
			if(mp.x > maxpos[1]) then maxpos[1] = mp.x end
			if(mp.z > maxpos[2]) then maxpos[2] = mp.z end
		end
	end
	averagepos[1] = averagepos[1] / #AllMarbles
	averagepos[2] = averagepos[2] / #AllMarbles
	averagepos[3] = averagepos[3] / #AllMarbles
	
	-- find the largest difference across an axis, use it to scale camera y
	local distanceX = math.abs(maxpos[1] - minpos[1])
	local distanceZ = math.abs(maxpos[2] - minpos[2])
	local distance = math.max(distanceX,distanceZ)
	local targetY = 64 + distance * 0.4
	
	local velocity = {averagepos[1] - camerapos.x, targetY - camerapos.y, averagepos[3] - camerapos.z}
	camerapos.x = camerapos.x + velocity[1] * CameraSpeed * deltaTime
	camerapos.y = camerapos.y + velocity[2] * 1.5 * CameraSpeed * deltaTime
	camerapos.z = camerapos.z + velocity[3] * CameraSpeed * deltaTime
	ct:SetPosition(camerapos.x,camerapos.y,camerapos.z)
end

local ModelDiffuseShader = Graphics.LoadShader("diffuse", "simplediffuse.vs", "simplediffuse.fs")
local ModelShadowShader = Graphics.LoadShader("model_shadow", "simpleshadow.vs", "simpleshadow.fs");
local ModelNoLighting = Graphics.LoadShader("model_nolight",  "basic.vs", "basic.fs")
-- Graphics.SetShadowShader(ModelDiffuseShader, ModelShadowShader)

local CubeModel = Graphics.LoadModel("cube2.fbx")
local SphereModel = Graphics.LoadModel("sphere_low.fbx")

function MakeCamera(name)
	e = World.AddEntity()
	local newTags = World.AddComponent_Tags(e)
	newTags:AddTag(Tag.new(name))
	
	local transform = World.AddComponent_Transform(e)
	transform:SetPosition(50,24,50)
	transform:SetRotation(-90,0,-180)
	
	local camera = World.AddComponent_Camera(e)
	
	return e
end

function MakeOuterWallMaterial()
	local e = World.AddEntity()
	local t = World.AddComponent_Tags(e)
	t:AddTag(Tag.new("Outer Wall Material"))
	local m = World.AddComponent_Material(e)
	m:SetVec4("MeshUVOffsetScale", vec4.new(0,0,1,1))
	m:SetFloat("MeshShininess", math.random(0,200))
	m:SetVec4("MeshSpecular", vec4.new(1,1,1,1))
	m:SetVec4("MeshDiffuseOpacity", vec4.new(0,0.8,1,1))
	return e
end

function MakeInnerWallMaterial()
	local e = World.AddEntity()
	local t = World.AddComponent_Tags(e)
	t:AddTag(Tag.new("Inner Wall Material"))
	local m = World.AddComponent_Material(e)
	m:SetVec4("MeshUVOffsetScale", vec4.new(0,0,1,1))
	m:SetFloat("MeshShininess", math.random(0,200))
	m:SetVec4("MeshSpecular", vec4.new(1,1,1,1))
	m:SetVec4("MeshDiffuseOpacity", vec4.new(0,1.0,0.2,0.9))
	m:SetIsTransparent(true)
	return e
end

function MakeRampMaterial()
	local e = World.AddEntity()
	local t = World.AddComponent_Tags(e)
	t:AddTag(Tag.new("Ramp Material"))
	local m = World.AddComponent_Material(e)
	m:SetVec4("MeshUVOffsetScale", vec4.new(0,0,1,1))
	m:SetFloat("MeshShininess", 128)
	m:SetVec4("MeshSpecular", vec4.new(1,1,1,1))
	m:SetVec4("MeshDiffuseOpacity", vec4.new(1,0.4,0,1))
	return e
end

local RampMaterial = MakeRampMaterial()
function MakeMaze()
	local OuterWallMaterial = MakeOuterWallMaterial()
	local InnerWallMaterial = MakeInnerWallMaterial()
	
	MakePit()
	
	MakeBox({-46.75,-2,0},{35,4,128},nil,"Floor left")
	
	MakeBox({-11.25,-2,54},{37,4,20},nil,"Floor Midleft/top")
	MakeBox({-11.25,-2,-54},{37,4,20},nil,"Floor Midleft/top")
	
	MakeBox({-3,-2,-21},{19.5,4,50},nil,"Floor middle long")
	MakeBox({-3,-2,33},{19.5,4,50},nil,"Floor middle long")
	
	MakeBox({43.5,-2,0},{41,4,128},nil,"Floor")
	
	MakeBox({-29.25,-2,31.7},{15,4,30},nil,"Floor nightmare")
	MakeBox({-17.25,-2,5.5},{12,4,30},nil,"Floor nightmare")
	MakeBox({-12.5,-2,-26.5},{12,4,36.5},nil,"Floor nightmare")
	
	MakeBox({15,-2,54},{16,4,20},nil,"Floor")
	MakeBox({15,-2,-54},{16,4,20},nil,"Floor")
	
	MakeBox({15,-2,0},{2,4,88},nil,"Floor")
	
	-- outer walls 
	MakeBox({62,8,0},{4,16,128},OuterWallMaterial,"Outer Wall")
	MakeBox({-62,8,0},{4,16,128},OuterWallMaterial,"Outer Wall")
	MakeBox({0,8,62},{120,16,4},OuterWallMaterial,"Outer Wall")
	MakeBox({0,8,-62},{120,16,4},OuterWallMaterial,"Outer Wall")
	
	-- sections
	MakeBoxScalePhys({42,3,6},{1.5,6,108}, InnerWallMaterial, "Inner Wall", {1,4,1})
	
	-- ramps
	MakeRamp({0,0,0},14,true)
	MakeRamp({-60,-2,30},4)
	MakeRamp({-54,-2,30},4)
	MakeRamp({-48,-2,30},4)
	MakeRamp({-54.5,-0.5,0},16)
	MakeRamp({-60,-2,-30},4)
	MakeRamp({-54,-2,-30},4)
	MakeRamp({-48,-2,-30},4)
	MakeRamp({-105,0,25},12)
	MakeRamp({-105,0,-25},12)
	
	MakeBoxScalePhys({24,3,-6},{1.5,6,108}, InnerWallMaterial, "Inner Wall", {1,4,1})
	
	-- spinner
	MakeSpinner({33,0.5,-35},{12,1,1}, 4)
	MakeSpinner({33,1.0,-35},{1,1,1}, 4)
	
	MakeSpinner({33,0.5,35},{12,1,1}, 5)
	MakeSpinner({33,1.0,35},{1,1,1}, 5)
	
	MakeSpinner({33,0.5,0},{12,1,1}, -6)
	MakeSpinner({33,1.0,0},{1,1,1}, -6)
	
	MakeSpinner({-34,1.0,0},{6,1,1}, 10)
	MakeSpinner({-42,1.0,-11},{6,1,1}, -11)
	
	MakeSpinner({-34,1.0,-25},{6,1,1}, 10)
	MakeSpinner({-42,1.0,-36},{6,1,1}, -11)
	
	MakeSpinner({-34,1.0,0},{6,1,1}, 10)
	MakeSpinner({-42,1.0,-11},{6,1,1}, -11)
	MakeSpinner({-42,1.0,11},{6,1,1}, -11)
	MakeSpinner({-34,1.0,25},{6,1,1}, 10)
	MakeSpinner({-42,1.0,36},{6,1,1}, -11)
	
	MakeBoxScalePhys({6,3,6},{1.5,6,108}, InnerWallMaterial, "Inner Wall", {1,4,1})
	
	MakeBoxScalePhys({-12,3,-6},{1.5,6,108}, InnerWallMaterial, "Inner Wall", {1,4,1})
	
	MakeBoxScalePhys({-30,3,6},{1.5,6,108}, InnerWallMaterial, "Inner Wall", {1,4,1})
	
	MakeBoxScalePhys({-46,3,-6},{1.5,6,108}, InnerWallMaterial, "Inner Wall", {1,4,1})
end

function MakeRamp(pos, width, hasmiddle)
	if(hasmiddle) then 
		MakeBox({pos[1]+51.5,pos[2]+2,pos[3]+6}, {width,4,8}, RampMaterial, "Ramp")
	end
	MakeOrientedBox({pos[1]+51.5,pos[2]+0.27,pos[3]+12.5}, {30,0,0}, {width,4,8}, RampMaterial, "Ramp")
	MakeOrientedBox({pos[1]+51.5,pos[2]+0.27,pos[3]+-0.5}, {-30,0,0}, {width,4,8}, RampMaterial, "Ramp")
end

function MakeSpinnerMaterial()
	local e = World.AddEntity()
	local t = World.AddComponent_Tags(e)
	t:AddTag(Tag.new("Spinner Material"))
	local m = World.AddComponent_Material(e)
	m:SetVec4("MeshUVOffsetScale", vec4.new(0,0,1,1))
	m:SetFloat("MeshShininess", 128)
	m:SetVec4("MeshSpecular", vec4.new(1,1,1,1))
	m:SetVec4("MeshDiffuseOpacity", vec4.new(1,0,0,1))
	return e
end

local SpinnerMaterial = nil
function MakeSpinner(pos, dims, speed)
	if(SpinnerMaterial == nil) then 
		SpinnerMaterial = MakeSpinnerMaterial()
	end

	e = World.AddEntity()
	local newTags = World.AddComponent_Tags(e)
	newTags:AddTag(Tag.new("Spinner"))
	
	local transform = World.AddComponent_Transform(e)
	transform:SetPosition(pos[1],pos[2],pos[3])
	transform:SetScale(dims[1],dims[2],dims[3])
	
	local newModel = World.AddComponent_Model(e)
	newModel:SetModel(CubeModel)
	newModel:SetShader(ModelDiffuseShader)
	-- newModel:SetMaterialEntity(SpinnerMaterial)
	
	local physics = World.AddComponent_Physics(e)
	physics:SetStatic(false)
	physics:SetKinematic(true)
	physics:AddBoxCollider(vec3.new(0.0,0.0,0.0),vec3.new(dims[1],dims[2],dims[3]))
	physics:Rebuild()
	
	table.insert(Spinners, {e,speed})
end

function MakePitMaterial()
	local e = World.AddEntity()
	local t = World.AddComponent_Tags(e)
	t:AddTag(Tag.new("Pit Material"))
	local m = World.AddComponent_Material(e)
	m:SetVec4("MeshUVOffsetScale", vec4.new(0,0,1,1))
	m:SetFloat("MeshShininess", 100)
	m:SetVec4("MeshSpecular", vec4.new(0,0,0,1))
	m:SetVec4("MeshDiffuseOpacity", vec4.new(0,0.0,0,1))
	return e
end

function MakePit()
	local mat = MakePitMaterial()
	MakeBox({62,-20,0},{4,32,128},mat,"Pit")
	MakeBox({-62,-20,0},{4,32,128},mat,"Pit")
	MakeBox({0,-20,62},{120,32,4},mat,"Pit")
	MakeBox({0,-20,-62},{120,32,4},mat,"Pit")
	MakeBox({0,-38,0},{128,4,128},mat,"Pit")
end

function AllMarblesInTargets()
	local inTargets = true
	for i=1,#TargetAreas do 
		local mt = World.GetComponent_Transform(TargetAreas[i][3])
		if(mt ~= nil) then 
			local mpos = mt:GetPosition()
			local tpos = TargetAreas[i][1]
			local tsize = TargetAreas[i][2]
			local dims = { tsize[1] / 2, tsize[2] / 2 }
			if(mpos.x < (tpos[1]-dims[1]) or mpos.x > (tpos[1] + dims[1]) or mpos.z < (tpos[2]-dims[2]) or mpos.z > (tpos[2] + dims[2])) then
				inTargets = false
			end
		end
	end
	return inTargets
end

function MakeMarbleTarget(pos, dims, colour, marble)
	local em = World.AddEntity()
	local tm = World.AddComponent_Tags(em)
	tm:AddTag(Tag.new("Target Material"))
	local mm = World.AddComponent_Material(em)
	mm:SetVec4("MeshUVOffsetScale", vec4.new(0,0,1,1))
	mm:SetFloat("MeshShininess", 128)
	mm:SetVec4("MeshSpecular", vec4.new(1,1,1,1))
	mm:SetVec4("MeshDiffuseOpacity", vec4.new(colour[1],colour[2],colour[3],1))

	e = World.AddEntity()
	local newTags = World.AddComponent_Tags(e)
	newTags:AddTag(Tag.new("Target"))
	
	local transform = World.AddComponent_Transform(e)
	transform:SetPosition(pos[1],pos[2],pos[3])
	transform:SetScale(dims[1],dims[2],dims[3])
	
	local newModel = World.AddComponent_Model(e)
	newModel:SetModel(CubeModel)
	newModel:SetShader(ModelDiffuseShader)
	-- newModel:SetMaterialEntity(em)
	
	table.insert(TargetAreas, {{pos[1],pos[3]}, {dims[1],dims[3]}, marble})
end

function MakeMarble(pos, size, colour)
	local em = World.AddEntity()
	local tm = World.AddComponent_Tags(em)
	tm:AddTag(Tag.new("Marble Material"))
	local mm = World.AddComponent_Material(em)
	mm:SetVec4("MeshUVOffsetScale", vec4.new(0,0,1,1))
	mm:SetFloat("MeshShininess", 128)
	mm:SetVec4("MeshSpecular", vec4.new(1,1,1,1))
	mm:SetVec4("MeshDiffuseOpacity", vec4.new(colour[1],colour[2],colour[3],1))

	e = World.AddEntity()
	local newTags = World.AddComponent_Tags(e)
	newTags:AddTag(Tag.new("Marble"))
	
	local transform = World.AddComponent_Transform(e)
	transform:SetPosition(pos[1],pos[2],pos[3])
	transform:SetScale(size,size,size)
	
	local newModel = World.AddComponent_Model(e)
	newModel:SetModel(SphereModel)
	newModel:SetShader(ModelDiffuseShader)
	-- newModel:SetMaterialEntity(em)
	
	local physics = World.AddComponent_Physics(e)
	physics:SetStaticFriction(0.1)
	physics:SetDynamicFriction(0.5)
	physics:SetRestitution(0.15)
	physics:SetStatic(false)
	physics:AddSphereCollider(vec3.new(0.0,0.0,0.0),size)
	physics:Rebuild()
	
	local light = World.AddComponent_Light(e)
	light:SetPointLight();
	light:SetColour(colour[1],colour[2],colour[3])
	light:SetAmbient(0.1)
	light:SetAttenuation(3.5)
	light:SetBrightness(1)
	light:SetDistance(16)
	light:SetCastsShadows(true)
	light:SetShadowmapSize(512,512)
	light:SetShadowBias(0.2)
	
	table.insert(AllMarbles, e)
end

function MakeOrientedBox(pos, rotation, dims, mat, tag)	
	e = World.AddEntity()
	local newTags = World.AddComponent_Tags(e)
	newTags:AddTag(Tag.new("Box"))
	newTags:AddTag(Tag.new(tag))
	
	local transform = World.AddComponent_Transform(e)
	transform:SetPosition(pos[1],pos[2],pos[3])
	transform:SetRotation(rotation[1],rotation[2],rotation[3])
	transform:SetScale(dims[1],dims[2],dims[3])
	
	local newModel = World.AddComponent_Model(e)
	newModel:SetModel(CubeModel)
	newModel:SetShader(ModelDiffuseShader)
	if(mat ~= nil) then 
		-- newModel:SetMaterialEntity(mat)
	end
	
	local physics = World.AddComponent_Physics(e)
	physics:SetStatic(true)
	physics:AddBoxCollider(vec3.new(0.0,0.0,0.0),vec3.new(dims[1],dims[2],dims[3]))
	physics:Rebuild()
	
	return e
end

function MakeBoxScalePhys(pos, dims, mat, tag, physscale)	
	e = World.AddEntity()
	local newTags = World.AddComponent_Tags(e)
	newTags:AddTag(Tag.new("Box"))
	newTags:AddTag(Tag.new(tag))
	
	local transform = World.AddComponent_Transform(e)
	transform:SetPosition(pos[1],pos[2],pos[3])
	transform:SetScale(dims[1],dims[2],dims[3])
	
	local newModel = World.AddComponent_Model(e)
	newModel:SetModel(CubeModel)
	newModel:SetShader(ModelDiffuseShader)
	if(mat ~= nil) then 
		-- newModel:SetMaterialEntity(mat)
	end
	
	local physics = World.AddComponent_Physics(e)
	physics:SetStatic(true)
	physics:AddBoxCollider(vec3.new(0.0,0.0,0.0),vec3.new(physscale[1] * dims[1],physscale[2] * dims[2],physscale[3] * dims[3]))
	physics:Rebuild()
	
	return e
end

function MakeBox(pos, dims, mat, tag)	
	e = World.AddEntity()
	local newTags = World.AddComponent_Tags(e)
	newTags:AddTag(Tag.new("Box"))
	newTags:AddTag(Tag.new(tag))
	
	local transform = World.AddComponent_Transform(e)
	transform:SetPosition(pos[1],pos[2],pos[3])
	transform:SetScale(dims[1],dims[2],dims[3])
	
	local newModel = World.AddComponent_Model(e)
	newModel:SetModel(CubeModel)
	newModel:SetShader(ModelDiffuseShader)
	if(mat ~= nil) then 
		-- newModel:SetMaterialEntity(mat)
	end
	
	local physics = World.AddComponent_Physics(e)
	physics:SetStatic(true)
	physics:AddBoxCollider(vec3.new(0.0,0.0,0.0),vec3.new(dims[1],dims[2],dims[3]))
	physics:Rebuild()
	
	return e
end

function MakeSunEntity()
	local newEntity = World.AddEntity()
	local newTags = World.AddComponent_Tags(newEntity)
	newTags:AddTag(Tag.new("Sunlight"))
	local transform = World.AddComponent_Transform(newEntity)
	transform:SetPosition(1.75,139.5,0.125)
	transform:SetRotation(0,0,0)
	local light = World.AddComponent_Light(newEntity)
	light:SetSpotLight();
	light:SetSpotAngles(0.57,0.78);
	light:SetColour(1,1,1)
	light:SetAmbient(0.39)
	light:SetAttenuation(1.6)
	light:SetBrightness(0.607)
	light:SetDistance(344.5)
	light:SetCastsShadows(true)
	light:SetShadowmapSize(4096,4096)
	light:SetShadowBias(0.000005)
end

return Marbles