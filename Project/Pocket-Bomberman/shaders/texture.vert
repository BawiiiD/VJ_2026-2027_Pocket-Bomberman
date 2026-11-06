#version 330

uniform mat4 projection, modelview;
uniform vec2 texCoordDispl;

in vec2 position;
in vec2 texCoord;
out vec2 texCoordFrag;

void main()
{
	// The quad's texture coordinates only cover one frame starting at (0,0);
	// adding the displacement moves them to the frame we want to show
	texCoordFrag = texCoord + texCoordDispl;
	gl_Position = projection * modelview * vec4(position, 0.0, 1.0);
}

