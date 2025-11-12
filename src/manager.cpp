#include "manager.h"

#include <algorithm>

#include "filesystem_.h"
#include "log.h"
#include "toast.h"
#include "vector_.h"

using namespace anm2ed::types;
using namespace anm2ed::util;

namespace anm2ed
{
  constexpr std::size_t RECENT_LIMIT = 10;

  std::filesystem::path Manager::recent_files_path_get() { return filesystem::path_preferences_get() + "recent.txt"; }
  std::filesystem::path Manager::autosave_path_get() { return filesystem::path_preferences_get() + "autosave.txt"; }
  std::filesystem::path Manager::autosave_directory_get() { return filesystem::path_preferences_get() + "autosave"; }

  Manager::Manager()
  {
    recent_files_load();
    autosave_files_load();
  }

  Document* Manager::get(int index) { return vector::find(documents, index > -1 ? index : selected); }

  void Manager::open(const std::filesystem::path& path, bool isNew, bool isRecent)
  {
    const auto pathString = path.string();
    std::string errorString{};
    documents.emplace_back(pathString, isNew, &errorString);

    auto& document = documents.back();
    if (!document.is_valid())
    {
      documents.pop_back();
      toasts.error(std::format("Failed to open document: {} ({})", pathString, errorString));
      return;
    }

    if (isRecent) recent_file_add(path);

    selected = (int)documents.size() - 1;
    pendingSelected = selected;
    toasts.info(std::format("Opened document: {}", pathString));
  }

  void Manager::new_(const std::filesystem::path& path) { open(path, true); }

  void Manager::save(int index, const std::filesystem::path& path)
  {
    if (auto document = get(index); document)
    {
      std::string errorString{};
      document->path = !path.empty() ? path : document->path;
      document->save(document->path.string(), &errorString);
      recent_file_add(document->path);
    }
  }

  void Manager::save(const std::filesystem::path& path) { save(selected, path); }

  void Manager::autosave(Document& document)
  {
    std::string errorString{};
    auto autosavePath = document.autosave_path_get();
    document.autosave(&errorString);

    autosaveFiles.erase(std::remove(autosaveFiles.begin(), autosaveFiles.end(), autosavePath), autosaveFiles.end());
    autosaveFiles.insert(autosaveFiles.begin(), autosavePath);

    autosave_files_write();
  }

  void Manager::close(int index)
  {
    if (!vector::in_bounds(documents, index)) return;

    autosaveFiles.erase(std::remove(autosaveFiles.begin(), autosaveFiles.end(), get()->autosave_path_get()),
                        autosaveFiles.end());
    autosave_files_write();

    documents.erase(documents.begin() + index);

    if (documents.empty())
    {
      selected = -1;
      pendingSelected = -1;
      return;
    }

    if (selected >= index) selected = std::max(0, selected - 1);

    selected = std::clamp(selected, 0, (int)documents.size() - 1);
    pendingSelected = selected;

    if (selected >= 0 && selected < (int)documents.size()) documents[selected].change(Document::ALL);
  }

  void Manager::set(int index)
  {
    if (documents.empty())
    {
      selected = -1;
      pendingSelected = -1;
      return;
    }

    index = std::clamp(index, 0, (int)documents.size() - 1);
    selected = index;

    if (auto document = get()) document->change(Document::ALL);
  }

  void Manager::layer_properties_open(int id)
  {
    if (auto document = get(); document)
    {
      if (id == -1)
        editLayer = anm2::Layer();
      else
        editLayer = document->anm2.content.layers.at(id);

      document->layer.reference = id;

      layerPropertiesPopup.open();
    }
  }

  void Manager::layer_properties_trigger() { layerPropertiesPopup.trigger(); }

  void Manager::layer_properties_end() { layerPropertiesPopup.end(); }

  void Manager::layer_properties_close()
  {
    editLayer = anm2::Layer();
    layerPropertiesPopup.close();
  }

  void Manager::null_properties_open(int id)
  {
    if (auto document = get(); document)
    {
      if (id == -1)
        editNull = anm2::Null();
      else
        editNull = document->anm2.content.nulls.at(id);

      document->null.reference = id;

      nullPropertiesPopup.open();
    }
  }

  void Manager::null_properties_trigger() { nullPropertiesPopup.trigger(); }

  void Manager::null_properties_end() { nullPropertiesPopup.end(); }

  void Manager::null_properties_close()
  {
    editNull = anm2::Null();
    nullPropertiesPopup.close();
  }

  void Manager::recent_file_add(const std::filesystem::path& path)
  {
    if (path.empty()) return;
    const auto pathString = path.string();
    std::error_code ec{};
    if (!std::filesystem::exists(path, ec))
    {
      logger.warning(std::format("Skipping missing recent file: {}", pathString));
      return;
    }

    recentFiles.erase(std::remove(recentFiles.begin(), recentFiles.end(), path), recentFiles.end());
    recentFiles.insert(recentFiles.begin(), path);
    if (recentFiles.size() > RECENT_LIMIT) recentFiles.resize(RECENT_LIMIT);
    recent_files_write();
  }

  void Manager::recent_files_load()
  {
    auto path = recent_files_path_get();

    std::ifstream file(path);
    if (!file)
    {
      logger.warning(std::format("Could not load recent files from: {}. Skipping...", path.string()));
      return;
    }

    logger.info(std::format("Loading recent files from: {}", path.string()));

    std::string line{};

    while (std::getline(file, line))
    {
      if (line.empty()) continue;
      std::filesystem::path entry = line;
      if (std::find(recentFiles.begin(), recentFiles.end(), entry) != recentFiles.end()) continue;
      std::error_code ec{};
      if (!std::filesystem::exists(entry, ec))
      {
        logger.warning(std::format("Skipping missing recent file: {}", line));
        continue;
      }
      recentFiles.emplace_back(std::move(entry));
    }
  }

  void Manager::recent_files_write()
  {
    auto path = recent_files_path_get();

    std::ofstream file;
    file.open(path, std::ofstream::out | std::ofstream::trunc);

    if (!file.is_open())
    {
      logger.warning(std::format("Could not write recent files to: {}. Skipping...", path.string()));
      return;
    }

    for (auto& entry : recentFiles)
      file << entry.string() << '\n';
  }

  void Manager::recent_files_clear()
  {
    recentFiles.clear();
    recent_files_write();
  }

  void Manager::autosave_files_open()
  {
    for (auto& path : autosaveFiles)
    {
      auto fileName = path.filename().string();
      if (!fileName.empty() && fileName.front() == '.') fileName.erase(fileName.begin());

      auto restorePath = path.parent_path() / fileName;
      restorePath.replace_extension("");
      open(path, false, false);

      if (auto document = get())
      {
        document->isForceDirty = true;
        document->path = restorePath;
        document->change(Document::ALL);
      }
    }

    autosave_files_clear();
  }

  void Manager::autosave_files_load()
  {
    auto path = autosave_path_get();

    std::ifstream file(path);
    if (!file)
    {
      logger.warning(std::format("Could not load autosave files from: {}. Skipping...", path.string()));
      return;
    }

    logger.info(std::format("Loading autosave files from: {}", path.string()));

    std::string line{};

    while (std::getline(file, line))
    {
      if (line.empty()) continue;
      std::filesystem::path entry = line;
      if (std::find(autosaveFiles.begin(), autosaveFiles.end(), entry) != autosaveFiles.end()) continue;
      autosaveFiles.emplace_back(std::move(entry));
    }
  }

  void Manager::autosave_files_write()
  {
    std::ofstream autosaveWriteFile;
    autosaveWriteFile.open(autosave_path_get(), std::ofstream::out | std::ofstream::trunc);

    for (auto& path : autosaveFiles)
      autosaveWriteFile << path.string() << "\n";

    autosaveWriteFile.close();
  }

  void Manager::autosave_files_clear()
  {
    for (auto& path : autosaveFiles)
      std::filesystem::remove(path);

    autosaveFiles.clear();
    autosave_files_write();
  }

  void Manager::chords_set(Settings& settings)
  {
    for (int i = 0; i < SHORTCUT_COUNT; i++)
      chords[i] = imgui::string_to_chord(settings.*SHORTCUT_MEMBERS[i]);
  }

  Manager::~Manager() { autosave_files_clear(); }
}
