#version 330

// Fragment shader: decides the color of every pixel of a sprite or a tile.

// Tint that multiplies the texture: white = normal, grey = darker (fades,
// the closed door), reddish = the boss flash, and its alpha for the bush blink
uniform vec4 color;
// If it's not (0,0,0), the sprite is repainted with this color (bombs, fire)
uniform vec3 recolor;
uniform sampler2D tex;

in vec2 texCoordFrag;
out vec4 outColor;

void main()
{
	// Transparent pixels are thrown away, so sprites don't show a box around them.
	// That's why we don't need blending to draw sprites over the map
	vec4 texColor = texture(tex, texCoordFrag);
	if(texColor.a < 0.5f)
		discard;
	// Recolor: keep how light each pixel is, but change its color. The shapes
	// and the shading of the sprite stay, only the color changes
	if(any(notEqual(recolor, vec3(0.0))))
	{
		float brightness = max(texColor.r, max(texColor.g, texColor.b));
		texColor.rgb = brightness * recolor;
	}
	outColor = color * texColor;
}

