#include "shader.h"

static bool _shader_compile(GLuint handle, const char* path);

static bool
_shader_compile(GLuint handle, const char* path)
{
	char buffer[SHADER_BUFFER_MAX];
	char compileLog[SHADER_BUFFER_MAX];
	s32 isCompile;

	file_read(path, buffer, SHADER_BUFFER_MAX);

	const GLchar* source = buffer;

	glShaderSource(handle, 1, &source, NULL);

	glCompileShader(handle);
	glGetShaderiv(handle, GL_COMPILE_STATUS, &isCompile);

	if (!isCompile)
	{
		glGetShaderInfoLog(handle, SHADER_BUFFER_MAX, NULL, compileLog);
		printf(STRING_ERROR_SHADER_INIT, path, compileLog);
		return false;
	}

	return true;
}

bool
shader_init(Shader* self, const char* vertexPath, const char* fragmentPath)
{
	GLuint vertexHandle;
	GLuint fragmentHandle;
	bool isSuccess;

 	memset(self, '\0', sizeof(Shader));

	vertexHandle = glCreateShader(GL_VERTEX_SHADER);
	fragmentHandle = glCreateShader(GL_FRAGMENT_SHADER);

	if 
	(
		!_shader_compile(vertexHandle, vertexPath) 	||
		!_shader_compile(fragmentHandle, fragmentPath)
	)
		return false;

	self->handle = glCreateProgram();

	glAttachShader(self->handle, vertexHandle);
	glAttachShader(self->handle, fragmentHandle);

	glLinkProgram(self->handle);

	glDeleteShader(vertexHandle);
	glDeleteShader(fragmentHandle);

	printf(STRING_INFO_SHADER_INIT, vertexPath, fragmentPath);

	return true;
}

void
shader_use(Shader* self)
{
	glUseProgram(self->handle);
}

void
shader_free(Shader* self)
{
	glDeleteProgram(self->handle);
}
