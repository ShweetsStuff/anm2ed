#include "clipboard.h"

static void _clipboard_item_remove(ClipboardItem* self, Anm2* anm2)
{
    switch (self->type)
    {
        case CLIPBOARD_FRAME:
        {
            Anm2FrameWithReference* frameWithReference = std::get_if<Anm2FrameWithReference>(&self->data);
            if (!frameWithReference) break;

            anm2_frame_erase(anm2, &frameWithReference->reference);
            break;
        }
        case CLIPBOARD_ANIMATION:
        {
            Anm2AnimationWithID* animationWithID = std::get_if<Anm2AnimationWithID>(&self->data);
            if (!animationWithID) break;

            for (auto & [id, animation] : anm2->animations)
            {
                if (id == animationWithID->id)
                {
                    anm2->animations.erase(animationWithID->id);
                    break;
                }
            }
            break;
        }
        default:
            break;
    }
}

static void _clipboard_item_paste(ClipboardItem* self, ClipboardLocation* location, Anm2* anm2)
{
    switch (self->type)
    {
        case CLIPBOARD_FRAME:
        {
            Anm2FrameWithReference* frameWithReference = std::get_if<Anm2FrameWithReference>(&self->data);
            Anm2Reference* reference = std::get_if<Anm2Reference>(location);
            
            if (!frameWithReference || !reference) break;
            if (frameWithReference->reference.itemType != reference->itemType) break;

            Anm2Animation* animation = anm2_animation_from_reference(anm2, reference);
            Anm2Item* anm2Item = anm2_item_from_reference(anm2, reference);
 
            if (!animation || !anm2Item) break;

            anm2_frame_add(anm2, &frameWithReference->frame, reference, reference->frameIndex); 

            break;
        }
        case CLIPBOARD_ANIMATION:
        {
            Anm2AnimationWithID* animationWithID = std::get_if<Anm2AnimationWithID>(&self->data);
            if (!animationWithID) break;

            s32 index = 0;
            
            if (std::holds_alternative<s32>(*location))
                index = std::get<s32>(*location);
            else
                break;

            index = std::clamp(index, 0, (s32)anm2->animations.size());

            map_insert_shift(anm2->animations, index, animationWithID->animation);
            break;
        }
        default:
            break;
    }
}

void clipboard_copy(Clipboard* self)
{
    self->item = self->hoveredItem;
}

void clipboard_cut(Clipboard* self)
{
    self->item = self->hoveredItem;
    _clipboard_item_remove(&self->item, self->anm2);
}

void clipboard_paste(Clipboard* self)
{
    _clipboard_item_paste(&self->item, &self->location, self->anm2);
}

void clipboard_init(Clipboard* self, Anm2* anm2)
{
    self->anm2 = anm2;
}

