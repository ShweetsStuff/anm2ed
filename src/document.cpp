#include "document.h"

#include "anm2.h"
#include "filesystem.h"

using namespace anm2ed::anm2;
using namespace anm2ed::filesystem;
using namespace anm2ed::types;

namespace anm2ed::document
{
  Document::Document()
  {
    for (auto& value : isJustChanged)
      value = true;
  }

  Document::Document(const std::string& path, bool isNew, std::string* errorString)
  {
    for (auto& value : isJustChanged)
      value = true;

    if (!path_is_exist(path)) return;

    if (isNew)
      anm2 = anm2::Anm2();
    else
    {
      anm2 = Anm2(path, errorString);
      if (errorString && !errorString->empty()) return;
    }

    this->path = path;
    on_change();
    clean();
  }

  bool Document::save(const std::string& path, std::string* errorString)
  {
    this->path = !path.empty() ? path : this->path.string();

    if (anm2.serialize(this->path, errorString))
    {
      clean();
      return true;
    }

    return false;
  }

  void Document::hash_set()
  {
    hash = anm2.hash();
  }

  void Document::clean()
  {
    saveHash = anm2.hash();
    hash = saveHash;
  }

  void Document::change(change::Type type)
  {
    hash_set();
    isJustChanged[type] = true;
  }

  bool Document::is_dirty()
  {
    return hash != saveHash;
  }

  bool Document::is_just_changed(types::change::Type type)
  {
    return isJustChanged[type];
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

  anm2::Item* Document::item_get()
  {
    return anm2.item_get(reference);
  }

  anm2::Spritesheet* Document::spritesheet_get()
  {
    return anm2.spritesheet_get(referenceSpritesheet);
  }

  bool Document::is_valid()
  {
    return !path.empty();
  }

  void Document::on_change()
  {
    if (is_just_changed(change::SPRITESHEETS))
    {
      spritesheetNames = anm2.spritesheet_names_get();
      spritesheetNamesCstr.clear();
      for (auto& name : spritesheetNames)
        spritesheetNamesCstr.push_back(name.c_str());
    }
  }

  void Document::update()
  {
    on_change();

    for (auto& value : isJustChanged)
      value = false;
  }
};