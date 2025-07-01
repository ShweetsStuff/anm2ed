#include "shader.h"

static bool _shader_compile(GLuint* self, const std::string& text);

// Compiles the shader; returns true/false based on success
static bool
_shader_compile(GLuint* self, const std::string& text)
{
	std::string compileLog;
	s32 isCompile;

	const GLchar* source = text.c_str();

	glShaderSource(*self, 1, &source, NULL);

	glCompileShader(*self);
	glGetShaderiv(*self, GL_COMPILE_STATUS, &isCompile);

	if (!isCompile)
	{
		glGetShaderInfoLog(*self, SHADER_INFO_LOG_MAX, NULL, &compileLog[0]);
		std::cout << STRING_ERROR_SHADER_INIT << *self << std::endl << compileLog << std::endl;
		return false;
	}

	return true;
}

// Initializes a given shader with vertex/fragment
bool
shader_init(GLuint* self, const std::string& vertex, const std::string& fragment)
{
	GLuint vertexHandle;
	GLuint fragmentHandle;

	vertexHandle = glCreateShader(GL_VERTEX_SHADER);
	fragmentHandle = glCreateShader(GL_FRAGMENT_SHADER);

	if 
	(
		!_shader_compile(&vertexHandle, vertex) 	||
		!_shader_compile(&fragmentHandle, fragment)
	)
		return false;

	*self = glCreateProgram();

	glAttachShader(*self, vertexHandle);
	glAttachShader(*self, fragmentHandle);

	glLinkProgram(*self);

	glDeleteShader(vertexHandle);
	glDeleteShader(fragmentHandle);

	return true;
}

// Frees a given shader 
void
shader_free(GLuint* self)
{
	glDeleteProgram(*self);
}