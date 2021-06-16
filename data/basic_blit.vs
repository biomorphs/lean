#version 460
#pragma sde include "global_uniforms.h"

layout(location = 0) in vec2 vs_in_position;
out vec2 vs_out_position;

void main()
{
    
    vs_out_position = vs_in_position;
    gl_Position = glm::vec4(vs_in_position,0.0,1.0);
}
