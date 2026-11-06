#ifndef _SHADER_INCLUDE
#define _SHADER_INCLUDE


#include <string>
#include <GL/glew.h>
#include <GL/gl.h>


using namespace std;


enum ShaderType { VERTEX_SHADER, FRAGMENT_SHADER };


// A Shader is one small GPU program (a vertex or a fragment shader) that we
// send to OpenGL and compile. It can be loaded from a file (what we do, with
// the files in shaders/) or straight from a string.
// On its own it does nothing: ShaderProgram joins two of them into the
// program we actually draw with.


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
	// Compiler messages, very useful when the shader has a typo
	const string &log() const;

private:
	bool loadShaderSource(const string &filename, string &shaderSource);

private:
	GLuint shaderId;
	bool compiled;
	string errorLog;

};


#endif // _SHADER_INCLUDE
