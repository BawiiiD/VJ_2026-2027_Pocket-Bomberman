#include <SOIL.h>
#include "Texture.h"


using namespace std;


Texture::Texture()
{
	// texId = 0 means "no texture yet", and deleting texture 0 is harmless
	texId = 0;
	widthTex = 0;
	heightTex = 0;
	wrapS = GL_REPEAT;
	wrapT = GL_REPEAT;
	minFilter = GL_LINEAR_MIPMAP_LINEAR;
	magFilter = GL_LINEAR_MIPMAP_LINEAR;
}

Texture::~Texture()
{
	glDeleteTextures(1, &texId);
}


bool Texture::loadFromFile(const string &filename, PixelFormat format)
{
	unsigned char *image = NULL;
	// SOIL decodes the PNG into a plain array of pixels in CPU memory (RGB or RGBA)
	switch(format)
	{
	case TEXTURE_PIXEL_FORMAT_RGB:
		image = SOIL_load_image(filename.c_str(), &widthTex, &heightTex, 0, SOIL_LOAD_RGB);
		break;
	case TEXTURE_PIXEL_FORMAT_RGBA:
		image = SOIL_load_image(filename.c_str(), &widthTex, &heightTex, 0, SOIL_LOAD_RGBA);
		break;
	}
	if(image == NULL)
		return false;
	// Create an empty texture in the GPU and make it the current one
	glGenTextures(1, &texId);
	glBindTexture(GL_TEXTURE_2D, texId);
	// Copy the pixels to the GPU
	switch(format)
	{
	case TEXTURE_PIXEL_FORMAT_RGB:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, widthTex, heightTex, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		break;
	case TEXTURE_PIXEL_FORMAT_RGBA:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, widthTex, heightTex, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		break;
	}
	glGenerateMipmap(GL_TEXTURE_2D);
	// The GPU has its own copy now, so the CPU one can go. It has to be freed
	// with SOIL's own function because SOIL allocated it
	SOIL_free_image_data(image);

	return true;
}

// These glyph functions come from the base code of the labs (one-channel
// textures for fonts). Our Text class builds an RGBA image instead.
void Texture::loadFromGlyphBuffer(unsigned char *buffer, int width, int height)
{
	glGenTextures(1, &texId);
	glBindTexture(GL_TEXTURE_2D, texId);
	// Rows of a one-byte-per-pixel image aren't aligned to 4 bytes, so tell OpenGL
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, buffer);
	glGenerateMipmap(GL_TEXTURE_2D);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
}

// Uploads pixels that we built in memory (the font, or intro.png after making
// its green background transparent)
void Texture::loadFromRGBABuffer(const unsigned char *buffer, int width, int height)
{
	widthTex = width;
	heightTex = height;
	glGenTextures(1, &texId);
	glBindTexture(GL_TEXTURE_2D, texId);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
	glGenerateMipmap(GL_TEXTURE_2D);
}

void Texture::createEmptyTexture(int width, int height)
{
	glGenTextures(1, &texId);
	glBindTexture(GL_TEXTURE_2D, texId);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, 0);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
}

void Texture::loadSubtextureFromGlyphBuffer(unsigned char *buffer, int x, int y, int width, int height)
{
	glBindTexture(GL_TEXTURE_2D, texId);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, width, height, GL_RED, GL_UNSIGNED_BYTE, buffer);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
}

void Texture::generateMipmap()
{
	glBindTexture(GL_TEXTURE_2D, texId);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glGenerateMipmap(GL_TEXTURE_2D);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
}

// What happens when a texture coordinate goes outside 0..1: GL_REPEAT tiles
// the image, GL_CLAMP_TO_EDGE repeats the border pixel. Sprites use clamp so
// they never pick pixels from the opposite side of the atlas
void Texture::setWrapS(GLint value)
{
	wrapS = value;
}

void Texture::setWrapT(GLint value)
{
	wrapT = value;
}

// Filter used when the texture is drawn smaller than it is
void Texture::setMinFilter(GLint value)
{
	minFilter = value;
}

// Filter used when the texture is drawn bigger (always, in our 2x game).
// GL_NEAREST keeps the pixels sharp
void Texture::setMagFilter(GLint value)
{
	magFilter = value;
}

void Texture::use() const
{
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texId);
	// The wrap and filter settings belong to the texture, so we set them here
	// every time it's bound
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapS);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
}


