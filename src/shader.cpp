#include "shader.h"

#include "log.h"

using namespace anm2ed::log;

namespace anm2ed::shader
{
  Shader::Shader() = default;

  Shader::Shader(const char* vertex, const char* fragment)
  {
    id = glCreateProgram();

    auto compile = [&](const GLuint& id, const char* text)
    {
      int isCompile{};
      glShaderSource(id, 1, &text, nullptr);
      glCompileShader(id);
      glGetShaderiv(id, GL_COMPILE_STATUS, &isCompile);

      if (!isCompile)
      {
        std::string compileLog(255, '\0');
        glGetShaderInfoLog(id, 255, nullptr, compileLog.data());
        logger.error(std::format("Unable to compile shader {}: {}", id, compileLog.c_str()));
        return false;
      }
      return true;
    };

    auto vertexHandle = glCreateShader(GL_VERTEX_SHADER);
    auto fragmentHandle = glCreateShader(GL_FRAGMENT_SHADER);

    if (!(compile(vertexHandle, vertex) && compile(fragmentHandle, fragment))) return;

    glAttachShader(id, vertexHandle);
    glAttachShader(id, fragmentHandle);

    glLinkProgram(id);

    auto isLinked = GL_FALSE;
    glGetProgramiv(id, GL_LINK_STATUS, &isLinked);
    if (!isLinked)
    {
      glDeleteProgram(id);
      id = 0;
    }

    glDeleteShader(vertexHandle);
    glDeleteShader(fragmentHandle);
  }

  Shader& Shader::operator=(Shader&& other) noexcept
  {
    if (this != &other)
    {
      if (is_valid()) glDeleteProgram(id);
      id = other.id;
      other.id = 0;
    }
    return *this;
  }

  Shader::~Shader()
  {
    if (is_valid()) glDeleteProgram(id);
  }

  bool Shader::is_valid() const
  {
    return id != 0;
  }
}
