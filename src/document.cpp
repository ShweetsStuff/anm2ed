#include "document.h"

#include "anm2.h"
#include "filesystem.h"

using namespace anm2ed::anm2;
using namespace anm2ed::filesystem;

namespace anm2ed::document
{
  Document::Document() = default;

  Document::Document(const std::string& path, bool isNew, std::string* errorString)
  {
    if (!path_is_exist(path)) return;

    if (isNew)
      anm2 = anm2::Anm2();
    else
    {
      anm2 = Anm2(path, errorString);
      if (errorString && !errorString->empty()) return;
    }

    this->path = path;
    hash_set();
  }

  bool Document::save(const std::string& path, std::string* errorString)
  {
    this->path = !path.empty() ? path : this->path.string();

    if (anm2.serialize(this->path, errorString))
    {
      hash_set();
      return true;
    }

    return false;
  }

  void Document::hash_set()
  {
    saveHash = anm2.hash();
    hash = saveHash;
  }

  void Document::hash_time(double time, double interval)
  {
    if (time - lastHashTime > interval)
    {
      hash = anm2.hash();
      lastHashTime = time;
    }
  }

  bool Document::is_dirty()
  {
    return hash != saveHash;
  }

  std::string Document::directory_get()
  {
    return path.parent_path();
  }

  std::string Document::filename_get()
  {
    return path.filename().string();
  }

  anm2::Animation* Document::animation_get()
  {
    return anm2.animation_get(reference);
  }

  anm2::Frame* Document::frame_get()
  {
    return anm2.frame_get(reference);
  }

  anm2::Spritesheet* Document::spritesheet_get()
  {
    return anm2.spritesheet_get(referenceSpritesheet);
  }

  bool Document::is_valid()
  {
    return !path.empty();
  }
};