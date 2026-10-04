#include "manager.hpp"

#include <algorithm>
#include <unordered_set>

#include <format>

#include "file.hpp"
#include "log.hpp"
#include "path.hpp"
#include "sdl.hpp"
#include "strings.hpp"
#include "toast.hpp"
#include "util/imgui/shortcut.hpp"
#include "vector.hpp"

using namespace anm2ed::types;
using namespace anm2ed::util;

namespace anm2ed
{
  constexpr std::size_t RECENT_LIMIT = 10;

  void ensure_parent_directory_exists(const std::filesystem::path& path)
  {
    auto parent = path.parent_path();
    if (parent.empty()) return;
    std::error_code ec{};
    std::filesystem::create_directories(parent, ec);
    if (ec) logger.warning(std::format("Could not create directory for {}: {}", path::to_utf8(path), ec.message()));
  }

  void autosave_file_remove(const std::filesystem::path& path)
  {
    if (path.empty()) return;

    std::error_code ec{};
    std::filesystem::remove(path, ec);
    if (ec) logger.warning(std::format("Could not remove autosave file {}: {}", path::to_utf8(path), ec.message()));
  }

  std::vector<std::filesystem::path> path_list_load(const std::filesystem::path& path, std::string_view name)
  {
    std::string fileData{};
    if (!file::read_to_string(path, &fileData, "rb"))
    {
      logger.warning(std::format("Could not load {} from: {}. Skipping...", name, path::to_utf8(path)));
      return {};
    }
    logger.info(std::format("Loading {} from: {}", name, path::to_utf8(path)));

    std::vector<std::filesystem::path> result{};
    std::istringstream file(fileData);
    for (std::string line{}; std::getline(file, line);)
    {
      if (!line.empty() && line.back() == '\r') line.pop_back();
      auto entry = path::from_utf8(line);
      if (!line.empty() && std::ranges::find(result, entry) == result.end()) result.push_back(entry);
    }
    return result;
  }

  void path_list_save(const std::filesystem::path& path, const std::vector<std::filesystem::path>& entries,
                      std::string_view name)
  {
    ensure_parent_directory_exists(path);
    std::ostringstream file{};
    for (auto& entry : entries)
      file << path::to_utf8(entry) << '\n';
    if (!file::write_string(path, file.str(), "wb"))
      logger.warning(std::format("Could not write {} to: {}. Skipping...", name, path::to_utf8(path)));
  }

  void Manager::selection_history_push(int index)
  {
    if (index < 0 || index >= (int)documents.size()) return;
    selectionHistory.erase(std::remove(selectionHistory.begin(), selectionHistory.end(), index),
                           selectionHistory.end());
    selectionHistory.push_back(index);
  }

  void Manager::selection_history_cleanup(int removedIndex)
  {
    if (removedIndex >= 0)
    {
      for (auto& entry : selectionHistory)
      {
        if (entry == removedIndex)
          entry = -1;
        else if (entry > removedIndex)
          --entry;
      }
    }

    selectionHistory.erase(std::remove_if(selectionHistory.begin(), selectionHistory.end(),
                                          [&](int idx) { return idx < 0 || idx >= (int)documents.size(); }),
                           selectionHistory.end());
    if (documents.empty()) selectionHistory.clear();
  }

  std::filesystem::path Manager::recent_files_path_get() { return sdl::preferences_directory_get() / "recent.txt"; }
  std::filesystem::path Manager::autosave_path_get() { return sdl::preferences_directory_get() / "autosave.txt"; }

  Manager::Manager()
  {
    recent_files_load();
    autosave_files_load();
  }

  Document* Manager::get(int index) { return vector::find(documents, index > -1 ? index : selected); }

  void Manager::command_push(Command command)
  {
    if (command.documentIndex == -1) command.documentIndex = selected;
    commands.push_back(std::move(command));
  }

  void Manager::commands_run()
  {
    auto queued = std::move(commands);
    commands.clear();

    for (auto& command : queued)
    {
      if (command.runManager) continue;

      if (auto document = get(command.documentIndex); document) document->command_run(*this, command);
    }

    for (auto& command : queued)
      if (command.runManager) command.runManager(*this);
  }

  Document* Manager::open(const std::filesystem::path& path, bool isNew, bool isRecent)
  {
    std::string errorString{};
    documents.emplace_back(path, isNew, &errorString);
    auto pathString = path::to_utf8(path);

    auto& document = documents.back();
    if (!document.is_valid())
    {
      documents.pop_back();
      toast_log(Level::ERROR, TOAST_OPEN_DOCUMENT_FAILED, pathString, errorString);
      return nullptr;
    }

    if (isRecent) recent_file_add(path);

    selected = (int)documents.size() - 1;
    pendingSelected = selected;
    selection_history_push(selected);
    toast_log(Level::INFO, TOAST_OPEN_DOCUMENT, pathString);

    return &document;
  }

  void Manager::new_(const std::filesystem::path& path) { open(path, true); }

  bool Manager::save(int index, const std::filesystem::path& path, Options options)
  {
    if (auto document = get(index); document)
    {
      std::string errorString{};
      const auto previousAutosavePath = document->autosave_path_get();
      auto savePath = !path.empty() ? path : document->path;
      savePath.replace_extension(".anm2");
      ensure_parent_directory_exists(savePath);

      if (!document->save(savePath, &errorString, options)) return false;

      const auto autosavePath = document->autosave_path_get();
      autosaveFiles.erase(std::remove(autosaveFiles.begin(), autosaveFiles.end(), previousAutosavePath),
                          autosaveFiles.end());
      if (autosavePath != previousAutosavePath)
        autosaveFiles.erase(std::remove(autosaveFiles.begin(), autosaveFiles.end(), autosavePath), autosaveFiles.end());
      autosave_file_remove(previousAutosavePath);
      autosave_file_remove(autosavePath);
      autosave_files_write();
      recent_file_add(document->path);
      return true;
    }

    return false;
  }

  bool Manager::save(const std::filesystem::path& path, Options options) { return save(selected, path, options); }

  void Manager::autosave(Document& document, Options options)
  {
    std::string errorString{};
    auto autosavePath = document.autosave_path_get();
    if (!document.autosave(&errorString, options)) return;

    autosaveFiles.erase(std::remove(autosaveFiles.begin(), autosaveFiles.end(), autosavePath), autosaveFiles.end());
    autosaveFiles.insert(autosaveFiles.begin(), autosavePath);

    autosave_files_write();
  }

  void Manager::autosave_file_clear(int index)
  {
    auto document = get(index);
    if (!document) return;

    auto autosavePath = document->autosave_path_get();
    autosaveFiles.erase(std::remove(autosaveFiles.begin(), autosaveFiles.end(), autosavePath), autosaveFiles.end());
    autosave_file_remove(autosavePath);
    document->spritesheets_autosave_clear();
    autosave_files_write();
  }

  void Manager::close(int index)
  {
    if (!vector::in_bounds(documents, index)) return;

    documents.erase(documents.begin() + index);
    selection_history_cleanup(index);

    if (documents.empty())
    {
      selected = -1;
      pendingSelected = -1;
      return;
    }

    if (!selectionHistory.empty())
    {
      selected = selectionHistory.back();
      selectionHistory.pop_back();
    }
    else if (selected >= index)
      selected = std::max(0, selected - 1);

    selected = std::clamp(selected, 0, (int)documents.size() - 1);
    pendingSelected = selected;

    if (selected >= 0 && selected < (int)documents.size()) documents[selected].change();
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
    selection_history_push(selected);

    if (auto document = get()) document->change();
  }

  void Manager::item_properties_open(ElementType type, int id)
  {
    auto document = get();
    if (!document || (type != ElementType::LAYER_ELEMENT && type != ElementType::NULL_ELEMENT)) return;
    auto isLayer = type == ElementType::LAYER_ELEMENT;
    auto& content = document->model.content;
    auto layer = isLayer ? model::item_get(content.layers, id) : nullptr;
    auto null = isLayer ? nullptr : model::item_get(content.nulls, id);
    if (id != -1 && !layer && !null) return;
    itemEdit = layer  ? ItemEdit{id, layer->name, layer->spritesheetId, false}
               : null ? ItemEdit{id, null->name, 0, null->isShowRect}
                      : ItemEdit{};
    itemPropertiesPopups[isLayer ? 0 : 1].open();
  }

  void Manager::recent_files_trim()
  {
    while (recentFiles.size() > RECENT_LIMIT)
    {
      auto oldest = std::min_element(recentFiles.begin(), recentFiles.end(),
                                     [](const auto& lhs, const auto& rhs) { return lhs.second < rhs.second; });
      if (oldest == recentFiles.end()) break;
      recentFiles.erase(oldest);
    }
  }

  std::vector<std::filesystem::path> Manager::recent_files_ordered() const
  {
    std::vector<std::pair<std::string, std::size_t>> orderedEntries(recentFiles.begin(), recentFiles.end());
    std::sort(orderedEntries.begin(), orderedEntries.end(),
              [](const auto& lhs, const auto& rhs) { return lhs.second > rhs.second; });

    std::vector<std::filesystem::path> ordered;
    ordered.reserve(orderedEntries.size());
    for (const auto& [pathString, _] : orderedEntries)
      ordered.emplace_back(path::from_utf8(pathString));
    return ordered;
  }

  void Manager::recent_file_add(const std::filesystem::path& path)
  {
    if (path.empty()) return;
    std::error_code ec{};
    if (!std::filesystem::exists(path, ec))
    {
      logger.warning(std::format("Skipping missing recent file: {}", path::to_utf8(path)));
      return;
    }

    recentFiles[path::to_utf8(path)] = ++recentFilesCounter;
    recent_files_trim();
    recent_files_write();
  }

  void Manager::recent_files_load()
  {
    recentFiles.clear();
    recentFilesCounter = 0;
    auto loaded = path_list_load(recent_files_path_get(), "recent files");
    for (auto it = loaded.rbegin(); it != loaded.rend(); ++it)
    {
      std::error_code ec{};
      if (std::filesystem::exists(*it, ec))
        recentFiles[path::to_utf8(*it)] = ++recentFilesCounter;
      else
        logger.warning(std::format("Skipping missing recent file: {}", path::to_utf8(*it)));
    }
    recent_files_trim();
  }

  void Manager::recent_files_write()
  {
    path_list_save(recent_files_path_get(), recent_files_ordered(), "recent files");
  }

  void Manager::recent_files_clear()
  {
    recentFiles.clear();
    recentFilesCounter = 0;
    recent_files_write();
  }

  void Manager::autosave_files_open()
  {
    for (auto& path : autosaveFiles)
    {
      if (auto document = open(path, false, false))
      {
        document->isForceDirty = true;
        document->path = document->path_from_autosave_get(path);
        document->spritesheets_autosave_restore();
        document->change();
      }
    }
  }

  void Manager::autosave_files_load()
  {
    for (auto& entry : path_list_load(autosave_path_get(), "autosave files"))
      if (std::ranges::find(autosaveFiles, entry) == autosaveFiles.end()) autosaveFiles.push_back(entry);
  }

  void Manager::autosave_files_write() { path_list_save(autosave_path_get(), autosaveFiles, "autosave files"); }

  void Manager::autosave_files_clear(bool removeFiles)
  {
    if (removeFiles)
      for (auto& path : autosaveFiles)
        autosave_file_remove(path);

    autosaveFiles.clear();
    autosave_files_write();
  }

  void Manager::chords_set(Settings& settings)
  {
    for (int i = 0; i < SHORTCUT_COUNT; i++)
      chords[i] = imgui::string_to_chord(settings.*SHORTCUT_MEMBERS[i]);
  }

  Manager::~Manager() = default;
}
