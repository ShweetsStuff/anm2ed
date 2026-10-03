#include "internal.hpp"

using namespace tinyxml2;

namespace anm2ed
{
  constexpr std::uint64_t ANM2_HASH_OFFSET = 14695981039346656037ull;
  constexpr std::uint64_t ANM2_HASH_PRIME = 1099511628211ull;
  constexpr std::size_t HASH_BUFFER_SIZE = 64;

  struct HashSink : ElementSink
  {
    std::uint64_t hash{ANM2_HASH_OFFSET};

    void byte_append(unsigned char value)
    {
      hash ^= value;
      hash *= ANM2_HASH_PRIME;
    }

    void bytes_append(const char* data, std::size_t size)
    {
      for (std::size_t i = 0; i < size; ++i)
        byte_append((unsigned char)data[i]);
    }

    void string_append(std::string_view value)
    {
      auto size = (std::uint64_t)value.size();
      bytes_append(reinterpret_cast<const char*>(&size), sizeof(size));
      bytes_append(value.data(), value.size());
    }

    template <class T> void formatted_append(const char* name, T value)
    {
      char buffer[HASH_BUFFER_SIZE]{};
      XMLUtil::ToStr(value, buffer, sizeof(buffer));
      attribute(name, (const char*)buffer);
    }

    void open(std::string_view tag) override
    {
      byte_append('E');
      string_append(tag);
    }

    void attribute(const char* name, const char* value) override
    {
      byte_append('A');
      string_append(name);
      string_append(value);
    }

    void attribute(const char* name, int value) override { formatted_append(name, value); }
    void attribute(const char* name, bool value) override { formatted_append(name, value); }
    void attribute(const char* name, float value) override { formatted_append(name, value); }
    void body() override { byte_append('['); }
    void close() override { byte_append(']'); }
  };

  std::uint64_t element_hash(const Element& element, Flags flags)
  {
    HashSink sink{};
    element_emit(sink, element, ElementType::UNKNOWN, flags);
    return sink.hash;
  }

  std::uint64_t anm2_hash_get(const Element& root, Options options)
  {
    HashSink sink{};
    sink.byte_append(options.isExtendedFormat ? 'E' : 'A');
    element_emit(sink, root, ElementType::UNKNOWN, SERIALIZE_ANM2ED_DEFAULT);
    return sink.hash;
  }
}
