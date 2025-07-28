#include "clipboard.h"

static void _clipboard_item_remove(ClipboardItem* self, Anm2* anm2)
{
    switch (self->type)
    {
        case CLIPBOARD_FRAME:
        {
            Anm2FrameWithReference frameWithReference = std::get<Anm2FrameWithReference>(self->data);
            Anm2Animation* animation = anm2_animation_from_reference(anm2, &frameWithReference.reference);

            if (!animation) break;

            std::vector<Anm2Frame>* frames = nullptr;
            
            switch (frameWithReference.reference.itemType)
            {
                case ANM2_ROOT:
                    frames = &animation->rootAnimation.frames;
                    break;
                case ANM2_LAYER:
                    frames = &animation->layerAnimations[frameWithReference.reference.itemID].frames;
                    break;
                case ANM2_NULL:
                    frames = &animation->nullAnimations[frameWithReference.reference.itemID].frames;
                    break;
                case ANM2_TRIGGERS:
                    frames = &animation->triggers.frames;
                    break;
                default:
                    break;
            }

            if (frames)
                frames->erase(frames->begin() + frameWithReference.reference.frameIndex);
            
            break;
        }
        case CLIPBOARD_ANIMATION:
        {
            Anm2AnimationWithID animationWithID = std::get<Anm2AnimationWithID>(self->data);

            for (auto & [id, animation] : anm2->animations)
            {
                if (id == animationWithID.id)
                    anm2->animations.erase(animationWithID.id);
            }
            break;
        }
        case CLIPBOARD_EVENT:
        {
            Anm2EventWithID eventWithID = std::get<Anm2EventWithID>(self->data);

            for (auto & [id, event] : anm2->events)
            {
                if (id == eventWithID.id)
                    anm2->events.erase(eventWithID.id);
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

            s32 insertIndex = (reference->itemType == ANM2_TRIGGERS) ? 
            reference->frameIndex : MAX(reference->frameIndex, (s32)anm2Item->frames.size()); 
            
            anm2Item->frames.insert(anm2Item->frames.begin() + insertIndex, frameWithReference->frame);
            
            anm2_animation_length_set(animation);
            break;
        }
        case CLIPBOARD_ANIMATION:
        {
            s32 index = 0;
            
            if (std::holds_alternative<s32>(*location))
                index = std::get<s32>(*location);

            index = CLAMP(index, 0, (s32)anm2->animations.size());

            map_insert_shift(anm2->animations, index, std::get<Anm2AnimationWithID>(self->data).animation);
            break;
        }
        case CLIPBOARD_EVENT:
        {
            s32 index = 0;
            
            if (std::holds_alternative<s32>(*location))
                index = std::get<s32>(*location);

            index = CLAMP(index, 0, (s32)anm2->events.size());

            map_insert_shift(anm2->events, index, std::get<Anm2EventWithID>(self->data).event);
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

