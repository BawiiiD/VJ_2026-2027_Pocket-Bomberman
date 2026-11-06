#version 330

// Not used by the game (see simple.frag)

uniform mat4 projection, modelview;

in vec2 position;

void main()
{
	// Transform position from pixel coordinates to clipping coordinates
	gl_Position = projection * modelview * vec4(position, 0.0, 1.0);
}
