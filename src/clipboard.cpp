#include "clipboard.h"

#include <SDL3/SDL.h>

namespace anm2ed
{
  std::string Clipboard::get()
  {
    auto text = SDL_GetClipboardText();
    auto string = std::string(text);
    SDL_free(text);

    return string;
  }

  bool Clipboard::is_empty()
  {
    return get().empty();
  }

  void Clipboard::set(const std::string& string)
  {
    SDL_SetClipboardText(string.data());
  }
}