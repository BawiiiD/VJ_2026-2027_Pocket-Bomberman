#ifndef _TEXTURE_INCLUDE
#define _TEXTURE_INCLUDE


#include <string>
#include <GL/glew.h>


using namespace std;


enum PixelFormat {TEXTURE_PIXEL_FORMAT_RGB, TEXTURE_PIXEL_FORMAT_RGBA};


// A Texture is an image that lives in the GPU. We load it once (from a PNG,
// or from pixels we built in memory, like the font) and OpenGL gives us an
// id. Every time we draw something with it, use() binds that id.


class Texture
{

public:
	Texture();
	~Texture();

	bool loadFromFile(const string &filename, PixelFormat format);
	void loadFromGlyphBuffer(unsigned char *buffer, int width, int height);
	void loadFromRGBABuffer(const unsigned char *buffer, int width, int height);

	void createEmptyTexture(int width, int height);
	void loadSubtextureFromGlyphBuffer(unsigned char *buffer, int x, int y, int width, int height);
	void generateMipmap();

	// These only remember the values; they're applied in use()
	void setWrapS(GLint value);
	void setWrapT(GLint value);
	void setMinFilter(GLint value);
	void setMagFilter(GLint value);

	void use() const;

	// Size in pixels, needed to turn pixel positions into texture coordinates (0..1)
	int width() const { return widthTex; }
	int height() const { return heightTex; }

private:
	int widthTex, heightTex;
	GLuint texId;
	GLint wrapS, wrapT, minFilter, magFilter;

};


#endif // _TEXTURE_INCLUDE

