Fishtank = {}

local offsetY = 38
local nukeEntity = nil
local nukeBrightness = 0.0
local TankWalls = {}

function Fishtank.Init()
	Graphics.SetClearColour(0.3,0.55,0.8)
	MakeSunEntity()
	MakeFloorEntity()
	
	-- table 
	MakeTable({20,-7,0},0.6)
	
	-- lamp
	MakeLampEntity({60.0,38.5,-2.75},0.9)
	
	-- walls
	table.insert(TankWalls, MakeTankWall({0,offsetY+20,20},{51,40,1}))
	table.insert(TankWalls, MakeTankWall({0,offsetY+20,-20},{51,40,1}))
	table.insert(TankWalls, MakeTankWall({25,offsetY+20,0},{1,40,39}))
	table.insert(TankWalls, MakeTankWall({-25,offsetY+20,0},{1,40,39}))
	
	-- floor
	MakeTankWall({0,offsetY+1,0},{49,1,39})
	
	-- sand base
	MakeSandBox({0,offsetY+4,0},{49,5,39})
	
	MakeRandomMaterials()
end

function Fishtank.Tick(deltaTime)
	if(Input.IsKeyPressed("KEY_SPACE")) then
		for b=0,10 do 
			local pos = {RandomFloat(-4,4), offsetY+RandomFloat(10,35), RandomFloat(-4,4)}
			local scale = RandomFloat(0.4,1.4)
			MakeBall(pos,scale)
		end
	end

	if(Input.IsKeyPressed("KEY_b")) then
		for b=0,5 do 
			local pos = {RandomFloat(-4,4), offsetY+RandomFloat(10,35), RandomFloat(-4,4)}
			local scale = RandomFloat(1.5,2.5)
			MakeBunny(pos,scale)
		end
	end
	
	if(Input.IsKeyPressed("KEY_c")) then
		for b=0,5 do 
			local pos = {RandomFloat(-4,4), offsetY+RandomFloat(10,35), RandomFloat(-4,4)}
			local scale = RandomFloat(1.5,2.5)
			MakeBox(pos,{RandomFloat(1.5,2.5),RandomFloat(1.5,2.5),RandomFloat(1.5,2.5)})
		end
	end
	
	if(Input.IsKeyPressed("KEY_r")) then
		Playground.ReloadScripts()
	end
	
	if(nukeEntity == nil and Input.IsKeyPressed("KEY_n")) then 
		nukeEntity = World.AddEntity()
		local t = World.AddComponent_Tags(nukeEntity)
		t:AddTag(Tag.new("Nuke!"))
		local transform = World.AddComponent_Transform(nukeEntity)
		transform:SetPosition(RandomFloat(-15,15),46,RandomFloat(-15,15))
		local physics = World.AddComponent_Physics(nukeEntity)
		physics:SetStatic(false)
		physics:SetKinematic(true)
		physics:AddSphereCollider(vec3.new(0,-4,0), RandomFloat(15,22))
		physics:Rebuild()
		local light = World.AddComponent_Light(nukeEntity)
		light:SetPointLight();
		light:SetColour(1,0.2,0.0)
		light:SetAmbient(0.09)
		light:SetBrightness(32.0)
		nukeBrightness = 20.0
		light:SetDistance(100)
		light:SetAttenuation(2.0)
		light:SetCastsShadows(false)
		light:SetShadowmapSize(1024,1024)
		light:SetShadowBias(0.5)
		
		-- blow the bloody walls off
		if(math.random(0,100) < 10) then 
			for i=1,#TankWalls do
				local physics = World.GetComponent_Physics(TankWalls[i])
				if(physics ~= nil) then
					physics:SetStatic(false)
					physics:Rebuild()
				end
			end
		end
		
		hasNuked = true
	end
	
	if(nukeEntity ~= nil) then 
		local t = World.GetComponent_Transform(nukeEntity)
		local l = World.GetComponent_Light(nukeEntity)
		if(l ~= nil and t ~= nil) then 
			local p = t:GetPosition()
			p.y = p.y + deltaTime * 48.0
			t:SetPosition(p.x,p.y,p.z)
			nukeBrightness = nukeBrightness - deltaTime * 32.0
			if(nukeBrightness > 0) then 
				l:SetBrightness(nukeBrightness)
			else
				World.RemoveEntity(nukeEntity)
				nukeEntity = nil
			end
		end
	end
	
	DebugGui.BeginWindow(true, "Fish Tank!")
	DebugGui.Text("Controls:\n\tHold left click to rotate\n\tWSAD To Move\n\tSHIFT to move faster")
	DebugGui.Text("\t'C' - Spawn cubes\n\t'B' - Spawn bunnies\n\t'SPACE' - Spawn spheres")
	DebugGui.Text("\t'N' - Set off the bomb!")
	DebugGui.Text("\t'R' - Reload Scripts")
	DebugGui.EndWindow()
end

local ModelDiffuseShader = Graphics.LoadShader("model_diffuse", "simplediffuse.vs", "simplediffuse.fs")
local ModelShadowShader = Graphics.LoadShader("model_shadow", "simpleshadow.vs", "simpleshadow.fs");
local ModelNoLighting = Graphics.LoadShader("model_nolight",  "basic.vs", "basic.fs")
Graphics.SetShadowShader(ModelDiffuseShader, ModelShadowShader)

local TableModel = Graphics.LoadModel("Dining_Table_FBX.fbx")
local CubeModel = Graphics.LoadModel("cube2.fbx")
local SphereModel = Graphics.LoadModel("sphere_low.fbx")
local BunnyModel = Graphics.LoadModel("bunny/bunny_superlow.obj")
local LampModel = Graphics.LoadModel("desklamp.FBX")

local TankMaterial = nil
local SandMaterial = nil

local MaterialEntities = {}
function MakeRandomMaterials()
	-- make a bunch of random materials
	for m=0,100 do 
		local e = World.AddEntity()
		local t = World.AddComponent_Tags(e)
		t:AddTag(Tag.new("Random Material"))
		local m = World.AddComponent_Material(e)
		m:SetVec4("MeshUVOffsetScale", vec4.new(0,0,1,1))
		m:SetFloat("MeshShininess", math.random(0,200))
		m:SetVec4("MeshSpecular", vec4.new(math.random(0,255)/255.0,math.random(0,255)/255.0,math.random(0,255)/255.0,math.random(1,1000)/800.0))
		m:SetVec4("MeshDiffuseOpacity", vec4.new(math.random(0,255)/125.0,math.random(0,255)/125.0,math.random(0,255)/125.0,1.0))
		table.insert(MaterialEntities, e)
	end
end


function RandomFloat(minv,maxv) 
	return minv + (math.random() * (maxv-minv))
end

function MakeBunny(p, scale)
	e = World.AddEntity()
	local newTags = World.AddComponent_Tags(e)
	newTags:AddTag(Tag.new("Bunny"))
	
	local transform = World.AddComponent_Transform(e)
	transform:SetPosition(p[1],p[2],p[3])
	transform:SetScale(scale,scale,scale)
	
	local newModel = World.AddComponent_Model(e)
	newModel:SetModel(BunnyModel)
	newModel:SetShader(ModelDiffuseShader)
	newModel:SetMaterialEntity(MaterialEntities[math.random(1,#MaterialEntities)])
	
	local physics = World.AddComponent_Physics(e)
	physics:SetStatic(false)
	physics:AddBoxCollider(vec3.new(-0.33*scale,1.46*scale,0.0*scale),vec3.new(0.56*scale,0.3*scale,0.42*scale))
	physics:AddBoxCollider(vec3.new(-0.73*scale,1.39*scale,-0.21*scale),vec3.new(0.18*scale,0.38*scale,0.86*scale))
	physics:AddSphereCollider(vec3.new(-0.7*scale,1.09*scale,0.36*scale),0.26*scale)
	physics:AddSphereCollider(vec3.new(-0.55*scale,0.67*scale,0.2*scale),0.36*scale)
	physics:AddSphereCollider(vec3.new(0.0*scale,0.55*scale,0.12*scale),0.46*scale)
	physics:AddSphereCollider(vec3.new(0.65*scale,0.29*scale,0.19*scale),0.16*scale)
	physics:Rebuild()
end

function MakeBox(pos, dims)	
	e = World.AddEntity()
	local newTags = World.AddComponent_Tags(e)
	newTags:AddTag(Tag.new("Box"))
	
	local transform = World.AddComponent_Transform(e)
	transform:SetPosition(pos[1],pos[2],pos[3])
	transform:SetScale(dims[1],dims[2],dims[3])
	
	local newModel = World.AddComponent_Model(e)
	newModel:SetModel(CubeModel)
	newModel:SetShader(ModelDiffuseShader)
	newModel:SetMaterialEntity(MaterialEntities[math.random(1,#MaterialEntities)])
	
	local physics = World.AddComponent_Physics(e)
	physics:SetStatic(false)
	physics:AddBoxCollider(vec3.new(0.0,0.0,0.0),vec3.new(dims[1],dims[2],dims[3]))
	physics:Rebuild()
end

function MakeBall(pos, radius)	
	e = World.AddEntity()
	local newTags = World.AddComponent_Tags(e)
	newTags:AddTag(Tag.new("Ball"))
	
	local transform = World.AddComponent_Transform(e)
	transform:SetPosition(pos[1],pos[2],pos[3])
	transform:SetScale(radius,radius,radius)
	
	local newModel = World.AddComponent_Model(e)
	newModel:SetModel(SphereModel)
	newModel:SetShader(ModelDiffuseShader)
	newModel:SetMaterialEntity(MaterialEntities[math.random(1,#MaterialEntities)])
	
	local physics = World.AddComponent_Physics(e)
	physics:SetStatic(false)
	physics:AddSphereCollider(vec3.new(0.0,0.0,0.0),radius)
	physics:Rebuild()
end

function MakeSandBox(pos, dims)
	if(SandMaterial == nil) then 
		SandMaterial = World.AddEntity()
		local t = World.AddComponent_Tags(SandMaterial)
		t:AddTag(Tag.new("Sand material"))
		local m = World.AddComponent_Material(SandMaterial)
		m:SetSampler("DiffuseTexture", Graphics.LoadTexture("sand.jpg"))
		m:SetFloat("MeshShininess", 46.55)
		m:SetVec4("MeshSpecular", vec4.new(0.1,0.1,0.1,0.1))
		m:SetVec4("MeshUVOffsetScale", vec4.new(0,0,3,3))
	end
	
	e = World.AddEntity()
	local newTags = World.AddComponent_Tags(e)
	newTags:AddTag(Tag.new("Sand"))
	
	local transform = World.AddComponent_Transform(e)
	transform:SetPosition(pos[1],pos[2],pos[3])
	transform:SetScale(dims[1],dims[2],dims[3])
	
	local newModel = World.AddComponent_Model(e)
	newModel:SetModel(CubeModel)
	newModel:SetShader(ModelDiffuseShader)
	newModel:SetMaterialEntity(SandMaterial)
	
	local physics = World.AddComponent_Physics(e)
	physics:SetStatic(true)
	physics:AddBoxCollider(vec3.new(0.0,0.0,0.0),vec3.new(dims[1],dims[2],dims[3]))
	physics:Rebuild()
end

function MakeTankWall(pos, dims)
	if(TankMaterial == nil) then 
		TankMaterial = World.AddEntity()
		local t = World.AddComponent_Tags(TankMaterial)
		t:AddTag(Tag.new("Tank wall material"))
		local m = World.AddComponent_Material(TankMaterial)
		m:SetFloat("MeshShininess", 50.55)
		m:SetVec4("MeshSpecular", vec4.new(1,1,1,8))
		m:SetVec4("MeshDiffuseOpacity", vec4.new(0.95,0.97,1,0.3))
		m:SetIsTransparent(true)
		m:SetCastShadows(false)
	end
	
	e = World.AddEntity()
	local newTags = World.AddComponent_Tags(e)
	newTags:AddTag(Tag.new("Tank wall"))
	
	local transform = World.AddComponent_Transform(e)
	transform:SetPosition(pos[1],pos[2],pos[3])
	transform:SetScale(dims[1],dims[2],dims[3])
	
	local newModel = World.AddComponent_Model(e)
	newModel:SetModel(CubeModel)
	newModel:SetShader(ModelDiffuseShader)
	newModel:SetMaterialEntity(TankMaterial)
	
	local physics = World.AddComponent_Physics(e)
	physics:SetStatic(true)
	physics:AddBoxCollider(vec3.new(0.0,0.0,0.0),vec3.new(dims[1],dims[2],dims[3]))
	physics:Rebuild()
	
	return e
end

function MakeTable(pos, scale)
	e = World.AddEntity()
	local newTags = World.AddComponent_Tags(e)
	newTags:AddTag(Tag.new("Table"))
	
	local transform = World.AddComponent_Transform(e)
	transform:SetPosition(pos[1],pos[2],pos[3])
	transform:SetRotation(0,90,0)
	transform:SetScale(scale,scale,scale)
	
	local newModel = World.AddComponent_Model(e)
	newModel:SetModel(TableModel)
	newModel:SetShader(ModelDiffuseShader)
	
	local physics = World.AddComponent_Physics(e)
	physics:SetStatic(true)
	physics:AddBoxCollider(vec3.new(0,scale*75.35,0),vec3.new(scale*107.44,scale*1.7,scale*180.6))
	physics:AddBoxCollider(vec3.new(0,scale*67.29,0),vec3.new(scale*109.66,scale*14.32,scale*182.33))
	physics:AddBoxCollider(vec3.new(scale*-47.5,scale*30.28,scale*85.11),vec3.new(scale*15.2,scale*59.69,scale*11.65))
	physics:AddBoxCollider(vec3.new(scale*47.5,scale*30.28,scale*85.11),vec3.new(scale*15.2,scale*59.69,scale*11.65))
	physics:AddBoxCollider(vec3.new(scale*47.5,scale*30.28,scale*-85.11),vec3.new(scale*15.2,scale*59.69,scale*11.65))
	physics:AddBoxCollider(vec3.new(scale*-47.5,scale*30.28,scale*-85.11),vec3.new(scale*15.2,scale*59.69,scale*11.65))
	physics:Rebuild()
end

function MakeLampEntity(pos,scale)
	local newEntity = World.AddEntity()
	local newTags = World.AddComponent_Tags(newEntity)
	newTags:AddTag(Tag.new("Lamp"))
	local transform = World.AddComponent_Transform(newEntity)
	transform:SetPosition(pos[1],pos[2],pos[3])
	transform:SetScale(scale,scale,scale)
	
	local newModel = World.AddComponent_Model(newEntity)
	newModel:SetModel(LampModel)
	newModel:SetShader(ModelDiffuseShader)
	
	local physics = World.AddComponent_Physics(newEntity)
	physics:SetStatic(true)
	physics:AddBoxCollider(vec3.new(scale*0,scale*1.33,scale*0),vec3.new(scale*23.17,scale*2.6,scale*23.41))
	physics:AddBoxCollider(vec3.new(scale*7.08,scale*16.45,scale*-0.128),vec3.new(scale*5.03,scale*26.93,scale*5.0))
	physics:AddBoxCollider(vec3.new(scale*-13.078,scale*42.24,scale*0),vec3.new(scale*19.37,scale*21.25,scale*19.7))
	physics:AddBoxCollider(vec3.new(scale*0,scale*38.25,scale*0),vec3.new(scale*15.29,scale*15.58,scale*5))
	physics:Rebuild()
	
	local lampLight = World.AddEntity()
	local newTags = World.AddComponent_Tags(lampLight)
	newTags:AddTag(Tag.new("Lamp Light"))
	local transform = World.AddComponent_Transform(lampLight)
	transform:SetPosition(pos[1] + -16.5 * scale,pos[2] + 38.9 * scale,pos[3] + 0.0 * scale)
	transform:SetScale(2,2,2)
	transform:SetRotation(0,0,-45)
	
	local newModel = World.AddComponent_Model(lampLight)
	newModel:SetModel(SphereModel)
	newModel:SetShader(ModelNoLighting)
	
	local light = World.AddComponent_Light(lampLight)
	light:SetSpotLight();
	light:SetSpotAngles(0.35,0.59);
	light:SetColour(0.97,0.68,0.34)
	light:SetAmbient(0.02)
	light:SetBrightness(1.5)
	light:SetDistance(82)
	light:SetCastsShadows(true)
	light:SetShadowmapSize(2048,2048)
	light:SetShadowBias(0.00001)
	light:SetShadowOrthoScale(142)
	
	local lampLightPoint = World.AddEntity()
	local newTags = World.AddComponent_Tags(lampLightPoint)
	newTags:AddTag(Tag.new("Lamp Point Light"))
	local transform = World.AddComponent_Transform(lampLightPoint)
	transform:SetPosition(pos[1] + -16.5 * scale,pos[2] + 38.9 * scale,pos[3] + 0.0 * scale)
	
	local light = World.AddComponent_Light(lampLightPoint)
	light:SetPointLight();
	light:SetColour(0.97,0.68,0.34)
	light:SetAmbient(0)
	light:SetBrightness(4.0)
	light:SetDistance(12)
	light:SetCastsShadows(false)
	
end

function MakeSunEntity()
	local newEntity = World.AddEntity()
	local newTags = World.AddComponent_Tags(newEntity)
	newTags:AddTag(Tag.new("Sunlight"))
	local transform = World.AddComponent_Transform(newEntity)
	transform:SetPosition(-131.75,206.75,92.875)
	transform:SetRotation(16.3,0.302,32.4)
	local light = World.AddComponent_Light(newEntity)
	light:SetSpotLight();
	light:SetSpotAngles(0.78,0.9);
	light:SetColour(1,1,1)
	light:SetAmbient(0.08)
	light:SetBrightness(0.3)
	light:SetDistance(500)
	light:SetCastsShadows(true)
	light:SetShadowmapSize(4096,4096)
	light:SetShadowBias(0.0000005)
	light:SetShadowOrthoScale(142)
end

function MakeFloorEntity()
	e = World.AddEntity()
	local newTags = World.AddComponent_Tags(e)
	newTags:AddTag(Tag.new("Floor"))
	
	local transform = World.AddComponent_Transform(e)
	transform:SetPosition(0,-8,0)
	transform:SetScale(400,8.0,400)
	
	local material = World.AddComponent_Material(e)
	material:SetVec4("MeshDiffuseOpacity", vec4.new(1.0,1.0,1.0,1.0))
	material:SetSampler("DiffuseTexture", Graphics.LoadTexture("mramor6x6.png"))
	material:SetVec4("MeshUVOffsetScale", vec4.new(0,0,16,16))
	
	local newModel = World.AddComponent_Model(e)
	newModel:SetModel(CubeModel)
	newModel:SetShader(ModelDiffuseShader)
	newModel:SetMaterialEntity(e)
	
	local physics = World.AddComponent_Physics(e)
	physics:SetStatic(true)
	physics:AddBoxCollider(vec3.new(0.0,0.0,0.0),vec3.new(400,8.0,400))
	physics:AddPlaneCollider(vec3.new(1.0,0.0,0.0),vec3.new(-200.0,0.0,0.0))
	physics:AddPlaneCollider(vec3.new(-1.0,0.0,0.0),vec3.new(200.0,0.0,0.0))
	physics:AddPlaneCollider(vec3.new(0.0,0.0,1.0),vec3.new(0.0,0.0,-200.0))
	physics:AddPlaneCollider(vec3.new(0.0,0.0,-1.0),vec3.new(0.0,0.0,200.0))
	physics:Rebuild()
end

return Fishtank