#include "clipboard.h"

void clipboard_copy(Clipboard* self) {
  std::string clipboardText{};

  auto clipboard_text_set = [&]() {
    if (!SDL_SetClipboardText(clipboardText.c_str()))
      log_warning(std::format(CLIPBOARD_TEXT_SET_WARNING, SDL_GetError()));
  };

  switch (self->type) {
  case CLIPBOARD_FRAME: {
    if (Anm2Reference* reference = std::get_if<Anm2Reference>(&self->destination)) {
      if (Anm2Frame* frame = anm2_frame_from_reference(self->anm2, *reference)) {
        anm2_frame_serialize_to_string(frame, reference->itemType, &clipboardText);
        clipboard_text_set();
      }
    }
    break;
  }
  case CLIPBOARD_ANIMATION: {
    if (std::set<int>* set = std::get_if<std::set<int>>(&self->source)) {
      for (auto& i : *set) {
        if (Anm2Animation* animation = anm2_animation_from_reference(self->anm2, {i})) {
          std::string animationText{};
          anm2_animation_serialize_to_string(animation, &animationText);
          clipboardText += animationText;
        }
      }
      clipboard_text_set();
    }
    break;
  }
  default:
    break;
  }
}

void clipboard_cut(Clipboard* self) {
  clipboard_copy(self);

  switch (self->type) {
  case CLIPBOARD_FRAME: {
    if (Anm2Reference* reference = std::get_if<Anm2Reference>(&self->destination))
      anm2_frame_remove(self->anm2, *reference);
    break;
  }
  case CLIPBOARD_ANIMATION: {
    if (std::set<int>* set = std::get_if<std::set<int>>(&self->source))
      anm2_animations_remove(self->anm2, *set);
    break;
  }
  default:
    break;
  }
}

bool clipboard_paste(Clipboard* self) {
  auto clipboard_string = [&]() {
    char* clipboardText = SDL_GetClipboardText();
    std::string clipboardString = std::string(clipboardText);
    SDL_free(clipboardText);
    return clipboardString;
  };

  switch (self->type) {
  case CLIPBOARD_FRAME: {
    if (Anm2Reference* reference = std::get_if<Anm2Reference>(&self->destination)) {
      Anm2Frame frame;
      if (!anm2_frame_deserialize_from_xml(&frame, clipboard_string()))
        return false;
      anm2_frame_add(self->anm2, &frame, *reference);
    }
    break;
  }
  case CLIPBOARD_ANIMATION: {
    if (int* index = std::get_if<int>(&self->destination)) {
      std::vector<Anm2Animation> clipboardAnimations;
      if (!anm2_animations_deserialize_from_xml(clipboardAnimations, clipboard_string()))
        return false;

      int useIndex = std::clamp(*index + 1, 0, (int)self->anm2->animations.size());

      for (auto& animation : clipboardAnimations)
        anm2_animation_add(self->anm2, &animation, useIndex++);
    }
    break;
  }
  default:
    break;
  }

  return true;
}

void clipboard_init(Clipboard* self, Anm2* anm2) { self->anm2 = anm2; }
bool clipboard_is_value(void) { return SDL_HasClipboardText(); }