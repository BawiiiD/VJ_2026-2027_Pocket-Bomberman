#ifndef _SHADER_PROGRAM_INCLUDE
#define _SHADER_PROGRAM_INCLUDE


#include <GL/glew.h>
#include <GL/gl.h>
#include <glm/glm.hpp>
#include "Shader.h"


// A ShaderProgram is a vertex shader and a fragment shader linked together.
// This is what we "use" before drawing. It also lets us:
// - tell OpenGL where each vertex attribute (position, texture coordinate)
//   is inside our vertex arrays,
// - send uniforms: values that are the same for the whole draw call, like
//   the projection matrix, the sprite position or a color tint.


class ShaderProgram
{

public:
	ShaderProgram();

	void init();
	void addShader(const Shader &shader);
	void bindFragmentOutput(const string &outputName);
	// Says where to find an attribute in the vertex data and returns its location
	GLint bindVertexAttribute(const string &attribName, GLint size, GLsizei stride, GLvoid *firstPointer);
	void link();
	void free();

	// From now on, draw calls go through this program
	void use();

	// Send values to the uniforms of the shaders. If the shader doesn't have
	// a uniform with that name they just do nothing
	void setUniform2f(const string &uniformName, float v0, float v1);
	void setUniform3f(const string &uniformName, float v0, float v1, float v2);
	void setUniform4f(const string &uniformName, float v0, float v1, float v2, float v3);
	void setUniformMatrix4f(const string &uniformName, glm::mat4 &mat);

	bool isLinked();
	const string &log() const;

private:
	GLuint programId;
	bool linked;
	string errorLog;

};


#endif // _SHADER_PROGRAM_INCLUDE
