#include "clipboard.h"

void clipboard_copy(Clipboard* self)
{
    std::string clipboardText{};
    
    auto clipboard_text_set = [&]()
    {
        if (!SDL_SetClipboardText(clipboardText.c_str()))
            log_warning(std::format(CLIPBOARD_TEXT_SET_WARNING, SDL_GetError()));
    };
    
    switch (self->type)
    {
        case CLIPBOARD_FRAME:
        {
            Anm2Reference* reference = std::get_if<Anm2Reference>(&self->location);
            if (!reference) break;
            Anm2Frame* frame = anm2_frame_from_reference(self->anm2, reference);
            if (!frame) break;
            anm2_frame_serialize_to_string(frame, reference->itemType, &clipboardText);
            clipboard_text_set();
            break;
        }
        case CLIPBOARD_ANIMATION:
        {
            s32* id = std::get_if<s32>(&self->location);
            if (!id) break;
            Anm2Animation* animation = map_find(self->anm2->animations, *id);
            if (!animation) break;
            anm2_animation_serialize_to_string(animation, &clipboardText);
            clipboard_text_set();
            break;
        }
            break;
        default:
            break;
    }
}

void clipboard_cut(Clipboard* self)
{
    clipboard_copy(self);

    switch (self->type)
    {
        case CLIPBOARD_FRAME:
        {
            Anm2Reference* reference = std::get_if<Anm2Reference>(&self->location);
            if (!reference) break;
            anm2_frame_remove(self->anm2, reference);
            break;
        }
        case CLIPBOARD_ANIMATION:
        {
            s32* id = std::get_if<s32>(&self->location);
            if (!id) break;
            anm2_animation_remove(self->anm2, *id);
            break;
        }
        default:
            break;
    }
}

bool clipboard_paste(Clipboard* self)
{
    auto clipboard_string = [&]()
    {
        char* clipboardText = SDL_GetClipboardText();
        std::string clipboardString = std::string(clipboardText);
        SDL_free(clipboardText);
        return clipboardString;
    };

    switch (self->type)
    {
        case CLIPBOARD_FRAME:
        {
            Anm2Reference* reference = std::get_if<Anm2Reference>(&self->location);
            if (!reference) break;
            Anm2Frame frame;
            if (anm2_frame_deserialize_from_xml(&frame, clipboard_string()))
                anm2_frame_add(self->anm2, &frame, reference);
            else return false;
            break;
        }
        case CLIPBOARD_ANIMATION:
        {
            s32* id = std::get_if<s32>(&self->location);
            if (!id) break;
            Anm2Animation animation;
             if (anm2_animation_deserialize_from_xml(&animation, clipboard_string()))
                anm2_animation_add(self->anm2, &animation, *id);
            else return false;
            break;
        }
        default:
            break;
    }

    return true;
}

void clipboard_init(Clipboard* self, Anm2* anm2)
{
    self->anm2 = anm2;
}

bool clipboard_is_value(void) 
{
    return SDL_HasClipboardText(); 
}