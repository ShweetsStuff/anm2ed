#include "shader.h"

static bool _shader_compile(GLuint* self, const std::string& text)
{
	s32 isCompile;
	const GLchar* source = text.c_str();

	glShaderSource(*self, 1, &source, nullptr);
	glCompileShader(*self);
	glGetShaderiv(*self, GL_COMPILE_STATUS, &isCompile);

	if (!isCompile)
	{
		std::string compileLog(SHADER_INFO_LOG_MAX, '\0');
		glGetShaderInfoLog(*self, SHADER_INFO_LOG_MAX, nullptr, compileLog.data());
		log_error(std::format(SHADER_INIT_ERROR, *self, compileLog.c_str()));
		return false;
	}

	return true;
}

// Initializes a given shader with vertex/fragment
bool shader_init(GLuint* self, const std::string& vertex, const std::string& fragment)
{
	GLuint vertexHandle;
	GLuint fragmentHandle;

	vertexHandle = glCreateShader(GL_VERTEX_SHADER);
	fragmentHandle = glCreateShader(GL_FRAGMENT_SHADER);

	if (!_shader_compile(&vertexHandle, vertex) || !_shader_compile(&fragmentHandle, fragment)) 
		return false;

	*self = glCreateProgram();

	glAttachShader(*self, vertexHandle);
	glAttachShader(*self, fragmentHandle);

	glLinkProgram(*self);

	glDeleteShader(vertexHandle);
	glDeleteShader(fragmentHandle);

	return true;
}

void shader_free(GLuint* self)
{
	glDeleteProgram(*self);
}