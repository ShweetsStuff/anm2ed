#include "audio_data.hpp"

#include <fstream>
#include <iterator>

#include "uid.hpp"

namespace anm2ed::resource
{
  AudioData::AudioData(const unsigned char* memory, size_t size)
  {
    if (!memory || size == 0) return;
    bytes.assign(memory, memory + size);
    uid = util::uid_next();
  }

  AudioData::AudioData(const std::filesystem::path& path)
  {
    std::ifstream file(path, std::ios::binary);
    std::vector<unsigned char> data{std::istreambuf_iterator<char>(file), {}};
    *this = AudioData(data.data(), data.size());
  }

  bool AudioData::is_valid() const { return !bytes.empty(); }
}
