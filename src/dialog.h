#pragma once

#include <string>

#include <SDL3/SDL.h>

namespace anm2ed::dialog
{
#if defined(_WIN32)
  #define EXECUTABLE_FILTER {"Executable", "exe"}
#else
  #define EXECUTABLE_FILTER {"Executable", "*"}
#endif

#define FILTER_LIST                                                                                                    \
  X(NO_FILTER, {})                                                                                                     \
  X(ANM2, {"Anm2 file", "anm2;xml"})                                                                                   \
  X(PNG, {"PNG image", "png"})                                                                                         \
  X(SOUND, {"WAV file;OGG file", "wav;ogg"})                                                                           \
  X(GIF, {"GIF image", "gif"})                                                                                         \
  X(WEBM, {"WebM video", "webm"})                                                                                      \
  X(MP4, {"MP4 video", "MP4"})                                                                                         \
  X(EXECUTABLE, EXECUTABLE_FILTER)

  enum Filter
  {
#define X(symbol, ...) symbol,
    FILTER_LIST
#undef X
  };

  constexpr SDL_DialogFileFilter FILTERS[][1] = {
#define X(symbol, ...) {__VA_ARGS__},
      FILTER_LIST
#undef X
  };

#undef FILTER_LIST

#define DIALOG_LIST                                                                                                    \
  X(NONE, NO_FILTER)                                                                                                   \
  X(ANM2_NEW, ANM2)                                                                                                    \
  X(ANM2_OPEN, ANM2)                                                                                                   \
  X(ANM2_SAVE, ANM2)                                                                                                   \
  X(SOUND_OPEN, SOUND)                                                                                                 \
  X(SPRITESHEET_OPEN, PNG)                                                                                             \
  X(SPRITESHEET_REPLACE, PNG)                                                                                          \
  X(FFMPEG_PATH_SET, EXECUTABLE)                                                                                       \
  X(PNG_DIRECTORY_SET, NO_FILTER)                                                                                      \
  X(PNG_PATH_SET, PNG)                                                                                                 \
  X(GIF_PATH_SET, GIF)                                                                                                 \
  X(WEBM_PATH_SET, WEBM)                                                                                               \
  X(MP4_PATH_SET, MP4)

  enum Type
  {
#define X(symbol, filter) symbol,
    DIALOG_LIST
#undef X
  };

  constexpr Filter TYPE_FILTERS[] = {
#define X(symbol, filter) filter,
      DIALOG_LIST
#undef X
  };

#undef DIALOG_LIST
}

namespace anm2ed
{

  class Dialog
  {
  public:
    SDL_Window* window{};
    std::string path{};
    dialog::Type type{dialog::NONE};
    int selectedFilter{-1};

    Dialog() = default;
    Dialog(SDL_Window*);
    void file_open(dialog::Type type);
    void file_save(dialog::Type type);
    void folder_open(dialog::Type type);
    bool is_selected(dialog::Type type) const;
    void reset();
    void file_explorer_open(const std::string&);
    void set_string_to_selected_path(std::string& set, dialog::Type type);
  };
}
