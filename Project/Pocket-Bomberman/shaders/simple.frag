#version 330

// Shaders for drawing plain colored shapes, from the base code of the labs.
// The game doesn't use them: everything is drawn with texture.vert/.frag

uniform vec4 color;

out vec4 outColor;

void main()
{
	outColor = color;
}
