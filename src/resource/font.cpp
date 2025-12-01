#include "font.h"

namespace anm2ed::resource
{
  Font::Font() = default;

  Font::Font(void* data, size_t length, int size)
  {
    ImFontConfig config;
    config.FontDataOwnedByAtlas = false;
    pointer = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(data, length, size, &config);
  }

  void Font::append(void* data, size_t length, int size)
  {
    ImFontConfig config;
    config.MergeMode = true;
    config.FontDataOwnedByAtlas = false;
    ImGui::GetIO().Fonts->AddFontFromMemoryTTF(data, length, size, &config);
  }

  Font::~Font()
  {
    if (get()) ImGui::GetIO().Fonts->RemoveFont(pointer);
  }

  ImFont* Font::get() { return pointer; }

  Font& Font::operator=(Font&& other) noexcept
  {
    if (this != &other)
    {
      pointer = other.pointer;
      other.pointer = nullptr;
    }
    return *this;
  }
}