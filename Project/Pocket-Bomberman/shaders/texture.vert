#version 330

// Vertex shader: places the corners of every quad on the screen.

// projection turns game pixels (and the camera) into the screen; modelview
// moves the quad to the position of the sprite
uniform mat4 projection, modelview;
// Where the current frame is in the texture (see Sprite)
uniform vec2 texCoordDispl;

in vec2 position;
in vec2 texCoord;
out vec2 texCoordFrag;

void main()
{
	// The quad's texture coordinates only cover one frame starting at (0,0);
	// adding the displacement moves them to the frame we want to show
	texCoordFrag = texCoord + texCoordDispl;
	// Transform position from pixel coordinates to clipping coordinates
	gl_Position = projection * modelview * vec4(position, 0.0, 1.0);
}

