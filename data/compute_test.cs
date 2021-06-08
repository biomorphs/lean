#version 430

layout(local_size_x = 1, local_size_y = 1) in;
layout(rgba32f, binding = 0) uniform image2D theTexture;

uniform vec4 Colour;
uniform float Time;

void main() {
  
  // get index in global work group i.e x,y position
  ivec2 pixel_coords = ivec2(gl_GlobalInvocationID.xy);
  
  vec4 pixel = Colour * vec4(sin(pixel_coords.x * 0.05 + Time * 1.5),0,sin(pixel_coords.y * 0.05 + Time * 3),1);
  
  //
  // interesting stuff happens here later
  //
  
  // output to a specific pixel in the image
  imageStore(theTexture, pixel_coords, pixel);
}