#include "shader.hpp"

#include <cctype>
#include <format>
#include <sstream>
#include <utility>

#include "log.hpp"

namespace anm2ed::resource::shader
{
#define SHADER_GLES_WORD_REPLACEMENTS                                                                                  \
  X("texture2D", "texture")                                                                                            \
  X("gl_FragColor", "fragColor")

  struct UniformAlias
  {
    UniformBinding binding{};
    const char* name{};
  };

#define RESOURCE_SHADER_UNIFORM_ALIASES                                                                                \
  X(UNIFORM_BINDING_MAIN_TEXTURE, UNIFORM_TEXTURE)                                                                     \
  X(UNIFORM_BINDING_MAIN_TEXTURE, "Texture0")                                                                          \
  X(UNIFORM_BINDING_MAIN_TEXTURE, "texture0")                                                                          \
  X(UNIFORM_BINDING_MAIN_TEXTURE, "Texture")                                                                           \
  X(UNIFORM_BINDING_MAIN_TEXTURE, "Sampler0")                                                                          \
  X(UNIFORM_BINDING_MAIN_TEXTURE, "ColorMap")                                                                          \
  X(UNIFORM_BINDING_TRANSFORM, UNIFORM_TRANSFORM)                                                                       \
  X(UNIFORM_BINDING_TRANSFORM, "Transform")                                                                            \
  X(UNIFORM_BINDING_TRANSFORM, "MVP")                                                                                  \
  X(UNIFORM_BINDING_TRANSFORM, "Matrix")                                                                               \
  X(UNIFORM_BINDING_FRAME_TINT, UNIFORM_TINT)                                                                           \
  X(UNIFORM_BINDING_FRAME_TINT, "Tint")                                                                                \
  X(UNIFORM_BINDING_FRAME_TINT, "Color")                                                                               \
  X(UNIFORM_BINDING_FRAME_TINT, "Color0")                                                                              \
  X(UNIFORM_BINDING_COLOR_OFFSET, UNIFORM_COLOR_OFFSET)                                                                 \
  X(UNIFORM_BINDING_COLOR_OFFSET, "ColorOffset")                                                                       \
  X(UNIFORM_BINDING_COLOR_OFFSET, "ColorOffset0")                                                                      \
  X(UNIFORM_BINDING_TEXTURE_SIZE, "u_texture_size")                                                                    \
  X(UNIFORM_BINDING_TEXTURE_SIZE, "TextureSize")                                                                       \
  X(UNIFORM_BINDING_TEXTURE_SIZE, "TexSize")

  constexpr UniformAlias UNIFORM_ALIASES[] = {
#define X(binding, name) {binding, name},
      RESOURCE_SHADER_UNIFORM_ALIASES
#undef X
  };

  bool is_shader_identifier(char character)
  {
    return std::isalnum((unsigned char)character) || character == '_';
  }

  std::string shader_text_replace(std::string string, std::string_view from, std::string_view to)
  {
    if (from.empty()) return string;
    std::size_t position{};
    while ((position = string.find(from, position)) != std::string::npos)
    {
      string.replace(position, from.size(), to.data(), to.size());
      position += to.size();
    }
    return string;
  }

  std::string shader_word_replace(std::string string, std::string_view from, std::string_view to)
  {
    if (from.empty()) return string;

    std::size_t position{};
    while ((position = string.find(from, position)) != std::string::npos)
    {
      auto after = position + from.size();
      auto isBeforeIdentifier = position > 0 && is_shader_identifier(string[position - 1]);
      auto isAfterIdentifier = after < string.size() && is_shader_identifier(string[after]);
      if (!isBeforeIdentifier && !isAfterIdentifier)
      {
        string.replace(position, from.size(), to.data(), to.size());
        position += to.size();
        continue;
      }
      position = after;
    }
    return string;
  }

  std::string shader_space_collapse(std::string_view string)
  {
    std::string out{};
    bool isSpace{};
    for (auto character : string)
    {
      if (std::isspace((unsigned char)character))
      {
        if (!out.empty() && !isSpace) out.push_back(' ');
        isSpace = true;
        continue;
      }
      out.push_back(character);
      isSpace = false;
    }
    if (!out.empty() && out.back() == ' ') out.pop_back();
    return out;
  }

  std::string shader_trim_get(std::string_view line)
  {
    std::size_t begin{};
    while (begin < line.size() && std::isspace((unsigned char)line[begin]))
      ++begin;

    auto end = line.size();
    while (end > begin && std::isspace((unsigned char)line[end - 1]))
      --end;

    return std::string(line.substr(begin, end - begin));
  }

  bool is_shader_word_present(std::string_view string, std::string_view word)
  {
    std::size_t position{};
    while ((position = string.find(word, position)) != std::string_view::npos)
    {
      auto after = position + word.size();
      auto isBeforeIdentifier = position > 0 && is_shader_identifier(string[position - 1]);
      auto isAfterIdentifier = after < string.size() && is_shader_identifier(string[after]);
      if (!isBeforeIdentifier && !isAfterIdentifier) return true;
      position = after;
    }
    return false;
  }

  bool is_shader_line_skipped(std::string_view line)
  {
    auto normalized = shader_space_collapse(shader_trim_get(line));
    if (normalized.starts_with("#version")) return true;
    if (normalized.starts_with("precision ")) return true;
    if (normalized == "#define lowp" || normalized == "# define lowp") return true;
    if (normalized == "#define mediump" || normalized == "# define mediump") return true;
    if (normalized == "#define highp" || normalized == "# define highp") return true;
    if (normalized == "#define texture texture2D" || normalized == "# define texture texture2D") return true;
    if (normalized == "#define fragColor gl_FragColor" || normalized == "# define fragColor gl_FragColor") return true;
    return false;
  }

  std::string shader_precision_erase(std::string line)
  {
    line = shader_word_replace(std::move(line), "lowp", "");
    line = shader_word_replace(std::move(line), "mediump", "");
    line = shader_word_replace(std::move(line), "highp", "");
    return line;
  }

  std::string shader_line_convert(std::string line, ShaderStage stage)
  {
    line = shader_precision_erase(std::move(line));
    line = shader_word_replace(std::move(line), "attribute", "in");
    line = shader_word_replace(std::move(line), "varying", stage == SHADER_STAGE_VERTEX ? "out" : "in");
#define X(from, to) line = shader_word_replace(std::move(line), from, to);
    SHADER_GLES_WORD_REPLACEMENTS
#undef X
    return line;
  }

  std::string shader_fragment_declarations_get(std::string_view body)
  {
    if (!is_shader_word_present(body, "fragColor")) return {};
    if (body.find("out vec4 fragColor") != std::string_view::npos) return {};
    return "out vec4 fragColor;\n";
  }

  std::string uniform_name_clean_get(std::string name)
  {
    if (name.ends_with("[0]")) name.erase(name.size() - 3);
    return name;
  }

  std::string uniform_float_string_get(float value) { return std::format("{:.6g}", value); }

  UniformBinding uniform_binding_auto_get(std::string_view name, UniformValueType type)
  {
    for (const auto& alias : UNIFORM_ALIASES)
      if (name == alias.name && is_uniform_binding_valid(alias.binding, type)) return alias.binding;
    if (type == UNIFORM_VALUE_SAMPLER2D) return UNIFORM_BINDING_MAIN_TEXTURE;
    if (is_uniform_value_editable(type)) return UNIFORM_BINDING_MANUAL;
    return UNIFORM_BINDING_IGNORE;
  }

  UniformBinding uniform_binding_get(std::string_view value)
  {
    for (const auto& info : UNIFORM_BINDING_INFOS)
      if (value == info.value) return info.binding;
    return UNIFORM_BINDING_IGNORE;
  }

  std::string_view uniform_binding_value_get(UniformBinding binding)
  {
    for (const auto& info : UNIFORM_BINDING_INFOS)
      if (binding == info.binding) return info.value;
    return UNIFORM_BINDING_INFOS[0].value;
  }

  std::string_view uniform_binding_label_get(UniformBinding binding)
  {
    for (const auto& info : UNIFORM_BINDING_INFOS)
      if (binding == info.binding) return info.label;
    return UNIFORM_BINDING_INFOS[0].label;
  }

  UniformValueType uniform_value_type_get(GLenum type)
  {
    switch (type)
    {
      case GL_FLOAT:
        return UNIFORM_VALUE_FLOAT;
      case GL_INT:
      case GL_BOOL:
        return UNIFORM_VALUE_INT;
      case GL_FLOAT_VEC2:
        return UNIFORM_VALUE_VEC2;
      case GL_FLOAT_VEC3:
        return UNIFORM_VALUE_VEC3;
      case GL_FLOAT_VEC4:
        return UNIFORM_VALUE_VEC4;
      case GL_FLOAT_MAT4:
        return UNIFORM_VALUE_MAT4;
      case GL_SAMPLER_2D:
        return UNIFORM_VALUE_SAMPLER2D;
      default:
        return UNIFORM_VALUE_UNKNOWN;
    }
  }

  std::string_view uniform_value_type_label_get(UniformValueType type)
  {
    for (const auto& info : UNIFORM_VALUE_TYPE_INFOS)
      if (type == info.type) return info.label;
    return UNIFORM_VALUE_TYPE_INFOS[0].label;
  }

  bool is_uniform_value_editable(UniformValueType type)
  {
    return type == UNIFORM_VALUE_FLOAT || type == UNIFORM_VALUE_INT || type == UNIFORM_VALUE_VEC2 ||
           type == UNIFORM_VALUE_VEC3 || type == UNIFORM_VALUE_VEC4 || type == UNIFORM_VALUE_SAMPLER2D;
  }

  bool is_uniform_value_vector(UniformValueType type)
  {
    return type == UNIFORM_VALUE_VEC2 || type == UNIFORM_VALUE_VEC3 || type == UNIFORM_VALUE_VEC4;
  }

  bool is_uniform_binding_valid(UniformBinding binding, UniformValueType type)
  {
    switch (binding)
    {
      case UNIFORM_BINDING_IGNORE:
        return true;
      case UNIFORM_BINDING_MANUAL:
        return is_uniform_value_editable(type);
      case UNIFORM_BINDING_MAIN_TEXTURE:
        return type == UNIFORM_VALUE_SAMPLER2D;
      case UNIFORM_BINDING_TRANSFORM:
        return type == UNIFORM_VALUE_MAT4;
      case UNIFORM_BINDING_FRAME_TINT:
      case UNIFORM_BINDING_COLOR_OFFSET:
      case UNIFORM_BINDING_TEXTURE_SIZE:
      case UNIFORM_BINDING_PLAYBACK_TIME:
        return type == UNIFORM_VALUE_FLOAT || is_uniform_value_vector(type);
      case UNIFORM_BINDING_COMPONENTS:
        return is_uniform_value_vector(type);
      default:
        return false;
    }
  }

  void uniform_value_parse(Uniform& uniform, std::string_view value)
  {
    if (value.empty()) return;

    std::stringstream stream{std::string(value)};
    if (uniform.valueType == UNIFORM_VALUE_INT || uniform.valueType == UNIFORM_VALUE_SAMPLER2D)
    {
      stream >> uniform.intValue;
      return;
    }

    stream >> uniform.value.x >> uniform.value.y >> uniform.value.z >> uniform.value.w;
    uniform.components[0].value = uniform.value.x;
    uniform.components[1].value = uniform.value.y;
    uniform.components[2].value = uniform.value.z;
    uniform.components[3].value = uniform.value.w;
  }

  std::string uniform_value_string_get(const Uniform& uniform)
  {
    switch (uniform.valueType)
    {
      case UNIFORM_VALUE_FLOAT:
        return uniform_float_string_get(uniform.value.x);
      case UNIFORM_VALUE_INT:
      case UNIFORM_VALUE_SAMPLER2D:
        return std::format("{}", uniform.intValue);
      case UNIFORM_VALUE_VEC2:
        return std::format("{} {}", uniform_float_string_get(uniform.value.x), uniform_float_string_get(uniform.value.y));
      case UNIFORM_VALUE_VEC3:
        return std::format("{} {} {}", uniform_float_string_get(uniform.value.x),
                           uniform_float_string_get(uniform.value.y), uniform_float_string_get(uniform.value.z));
      case UNIFORM_VALUE_VEC4:
        return std::format("{} {} {} {}", uniform_float_string_get(uniform.value.x),
                           uniform_float_string_get(uniform.value.y), uniform_float_string_get(uniform.value.z),
                           uniform_float_string_get(uniform.value.w));
      default:
        return {};
    }
  }

  std::vector<Uniform> uniforms_get(GLuint id)
  {
    GLint count{};
    GLint maxLength{};
    glGetProgramiv(id, GL_ACTIVE_UNIFORMS, &count);
    glGetProgramiv(id, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxLength);

    std::vector<Uniform> uniforms{};
    if (count <= 0 || maxLength <= 0) return uniforms;

    std::string name((std::size_t)maxLength, '\0');
    for (GLint index = 0; index < count; ++index)
    {
      GLsizei length{};
      GLint size{};
      GLenum type{};
      glGetActiveUniform(id, (GLuint)index, maxLength, &length, &size, &type, name.data());

      auto uniformName = uniform_name_clean_get(std::string(name.data(), (std::size_t)length));
      auto valueType = uniform_value_type_get(type);

      Uniform uniform{};
      uniform.name = uniformName;
      uniform.location = glGetUniformLocation(id, uniformName.c_str());
      uniform.glType = type;
      uniform.size = size;
      uniform.valueType = valueType;
      uniform.binding = uniform_binding_auto_get(uniformName, valueType);
      uniform.components[0].value = uniform.value.x;
      uniform.components[1].value = uniform.value.y;
      uniform.components[2].value = uniform.value.z;
      uniform.components[3].value = uniform.value.w;
      uniforms.push_back(std::move(uniform));
    }

    return uniforms;
  }

  std::string gles_convert(std::string_view source, ShaderStage stage)
  {
    auto normalized = shader_text_replace(std::string(source), "\r\n", "\n");
    normalized = shader_text_replace(std::move(normalized), "\r", "\n");

    std::string body{};
    std::size_t position{};
    while (position <= normalized.size())
    {
      auto end = normalized.find('\n', position);
      auto isEnd = end == std::string::npos;
      auto line = normalized.substr(position, isEnd ? std::string::npos : end - position);
      if (!is_shader_line_skipped(line)) body += shader_line_convert(std::move(line), stage) + "\n";
      if (isEnd) break;
      position = end + 1;
    }

    auto declarations = stage == SHADER_STAGE_FRAGMENT ? shader_fragment_declarations_get(body) : std::string{};
    return "#version 330 core\n" + declarations + body;
  }

  std::string gles_fragment_convert(std::string_view source)
  {
    return gles_convert(source, SHADER_STAGE_FRAGMENT);
  }

  std::string gles_vertex_convert(std::string_view source)
  {
    return gles_convert(source, SHADER_STAGE_VERTEX);
  }
}

namespace anm2ed::resource
{
  ShaderCompileResult shader_compile(const char* vertex, const char* fragment)
  {
    ShaderCompileResult result{};
    result.id = glCreateProgram();

    auto compile = [&](const GLuint& id, const char* text, const char* label)
    {
      int isCompile{};
      glShaderSource(id, 1, &text, nullptr);
      glCompileShader(id);
      glGetShaderiv(id, GL_COMPILE_STATUS, &isCompile);

      if (!isCompile)
      {
        GLint length{};
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
        if (length <= 0) length = 1;
        std::string compileLog((std::size_t)length, '\0');
        glGetShaderInfoLog(id, length, nullptr, compileLog.data());
        result.output += std::format("{} shader compile log:\n{}\n", label, compileLog.c_str());
        return false;
      }
      return true;
    };

    auto vertexHandle = glCreateShader(GL_VERTEX_SHADER);
    auto fragmentHandle = glCreateShader(GL_FRAGMENT_SHADER);

    auto isVertexCompiled = compile(vertexHandle, vertex, "Vertex");
    auto isFragmentCompiled = compile(fragmentHandle, fragment, "Fragment");
    result.isCompiled = isVertexCompiled && isFragmentCompiled;
    if (result.isCompiled)
    {
      glAttachShader(result.id, vertexHandle);
      glAttachShader(result.id, fragmentHandle);

      for (const auto& alias : shader::ATTRIBUTE_ALIASES)
        glBindAttribLocation(result.id, alias.index, alias.name);

      glLinkProgram(result.id);

      auto isLinked = GL_FALSE;
      glGetProgramiv(result.id, GL_LINK_STATUS, &isLinked);
      result.isLinked = isLinked == GL_TRUE;
      if (!result.isLinked)
      {
        GLint length{};
        glGetProgramiv(result.id, GL_INFO_LOG_LENGTH, &length);
        if (length <= 0) length = 1;
        std::string linkLog((std::size_t)length, '\0');
        glGetProgramInfoLog(result.id, length, nullptr, linkLog.data());
        result.output += std::format("Shader link log:\n{}\n", linkLog.c_str());
      }
      else
        result.uniforms = shader::uniforms_get(result.id);
    }

    glDeleteShader(vertexHandle);
    glDeleteShader(fragmentHandle);

    if (!result.isLinked)
    {
      glDeleteProgram(result.id);
      result.id = 0;
    }

    return result;
  }

  Shader::Shader() = default;

  Shader::Shader(const char* vertex, const char* fragment)
  {
    auto result = shader_compile(vertex, fragment);
    id = result.id;
    uniforms = std::move(result.uniforms);
    if (!result.output.empty()) logger.error(result.output);
  }

  Shader::Shader(Shader&& other) noexcept { *this = std::move(other); }

  Shader& Shader::operator=(Shader&& other) noexcept
  {
    if (this != &other)
    {
      if (is_valid()) glDeleteProgram(id);
      id = other.id;
      uniforms = std::move(other.uniforms);
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
