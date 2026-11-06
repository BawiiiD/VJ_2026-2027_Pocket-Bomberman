#version 330

uniform vec4 color;
uniform vec3 recolor;
uniform sampler2D tex;

in vec2 texCoordFrag;
out vec4 outColor;

void main()
{
	// Discard fragment if texture sample has alpha < 0.5
	// otherwise compose the texture sample with the fragment's interpolated color
	vec4 texColor = texture(tex, texCoordFrag);
	if(texColor.a < 0.5f)
		discard;
	if(any(notEqual(recolor, vec3(0.0))))
	{
		float brightness = max(texColor.r, max(texColor.g, texColor.b));
		texColor.rgb = brightness * recolor;
	}
	outColor = color * texColor;
}

