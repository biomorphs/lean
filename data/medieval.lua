Medieval = {}

function Medieval.Init()
	Graphics.SetClearColour(0.3,0.3,0.3)
	local BasicShader = Graphics.LoadShader("basic", "basic.vs", "basic.fs")
	local DiffuseShader = Graphics.LoadShader("diffuse", "simplediffuse.vs", "simplediffuse.fs")
	local ShadowShader = Graphics.LoadShader("shadow", "simpleshadow.vs", "simpleshadow.fs");
	Graphics.SetShadowShader(DiffuseShader, ShadowShader)
	Graphics.SetShadowShader(BasicShader, ShadowShader)
end


function Medieval.Tick(deltaTime)
end

return Medieval