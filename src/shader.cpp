#include "shader.h"

static bool _shader_compile(GLuint* self, const char* text);

static bool
_shader_compile(GLuint* self, const char* text)
{
	char compileLog[SHADER_BUFFER_MAX];
	s32 isCompile;

	const GLchar* source = text;

	glShaderSource(*self, 1, &source, NULL);

	glCompileShader(*self);
	glGetShaderiv(*self, GL_COMPILE_STATUS, &isCompile);

	if (!isCompile)
	{
		glGetShaderInfoLog(*self, SHADER_BUFFER_MAX, NULL, compileLog);
		printf(STRING_ERROR_SHADER_INIT, *self, compileLog);
		return false;
	}

	return true;
}

bool
shader_init(GLuint* self, const char* vertex, const char* fragment)
{
	GLuint vertexHandle;
	GLuint fragmentHandle;
	bool isSuccess;

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

	printf(STRING_INFO_SHADER_INIT, *self);

	return true;
}

void
shader_free(GLuint* self)
{
	glDeleteProgram(*self);
}