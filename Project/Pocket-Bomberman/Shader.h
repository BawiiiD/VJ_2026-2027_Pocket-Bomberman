#ifndef _SHADER_INCLUDE
#define _SHADER_INCLUDE


#include <string>
#include <GL/glew.h>
#include <GL/gl.h>


using namespace std;


enum ShaderType { VERTEX_SHADER, FRAGMENT_SHADER };


class Shader
{

public:
	Shader();

	// These need an OpenGL context, so they can only be called after the window is created
	void initFromSource(const ShaderType type, const string &source);
	bool initFromFile(const ShaderType type, const string &filename);
	void free();

	GLuint getId() const;
	bool isCompiled() const;
	// Compiler messages
	const string &log() const;

private:
	bool loadShaderSource(const string &filename, string &shaderSource);

private:
	GLuint shaderId;
	bool compiled;
	string errorLog;

};


#endif // _SHADER_INCLUDE
