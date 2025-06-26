#include "imgui.h"

static void _imgui_tooltip(const char* tooltip);
static void _imgui_timeline_element_frames(Imgui* self, void* element, s32* id, s32* index, Anm2AnimationType type);
static void _imgui_timeline_element(Imgui* self, void* element, s32* id, s32* index, Anm2AnimationType type);
static void _imgui_timeline(Imgui* self);
static void _imgui_animations(Imgui* self);
static void _imgui_events(Imgui* self);
static void _imgui_spritesheets(Imgui* self);
static void _imgui_frame_properties(Imgui* self);
static void _imgui_spritesheet_editor(Imgui* self);
static void _imgui_animation_preview(Imgui* self);
static void _imgui_taskbar(Imgui* self);

/* Makes a tooltip! */
static void _imgui_tooltip(const char* tooltip)
{
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
        ImGui::SetTooltip("%s", tooltip);
}

/* Displays the element's frames */
static void
_imgui_timeline_element_frames(Imgui* self, void* element, s32* id, s32* index, Anm2AnimationType type)
{
	Anm2Animation* animation = &self->anm2->animations[self->animationID];
	ImVec2 framePos;
	ImVec2 frameFinishPos;
	Anm2LayerAnimation* layerAnimation = NULL;
	Anm2NullAnimation* nullAnimation = NULL;
	Anm2RootAnimation* rootAnimation = NULL;
	Anm2Triggers* triggers = NULL;
	ImVec2 cursorPos = ImGui::GetCursorPos();

	void* frames = NULL;

	switch (type)
	{
		case ANM2_ROOT_ANIMATION:
			rootAnimation = (Anm2RootAnimation*)element;
			frames = &rootAnimation->frames;
			break;
		case ANM2_LAYER_ANIMATION:
			layerAnimation = (Anm2LayerAnimation*)element;
			frames = &layerAnimation->frames;
			break;
		case ANM2_NULL_ANIMATION:
			nullAnimation = (Anm2NullAnimation*)element;
			frames = &nullAnimation->frames;
			break;
		case ANM2_TRIGGER:
			triggers = (Anm2Triggers*)element;
			frames = &triggers->items;
			break;
		default:
			break;
	}

	ImGui::PushID(*index);

	if (animation->frameNum > 0)
	{
		ImVec2 frameListSize = {IMGUI_TIMELINE_FRAME_SIZE.x * animation->frameNum, IMGUI_TIMELINE_ELEMENTS_TIMELINE_SIZE.y};

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

		ImGui::BeginChild(STRING_IMGUI_TIMELINE_FRAMES, frameListSize, true);

		framePos = ImGui::GetCursorPos();

		for (s32 i = 0; i < animation->frameNum; i++)
		{
			ImGui::PushID(i);

			ImVec2 frameTexturePos = ImGui::GetCursorScreenPos();

			if (i % IMGUI_TIMELINE_FRAME_INDICES_MULTIPLE == 0)
			{
				ImVec2 bgMin = frameTexturePos;
				ImVec2 bgMax = ImVec2(frameTexturePos.x + IMGUI_TIMELINE_FRAME_SIZE.x, frameTexturePos.y + IMGUI_TIMELINE_FRAME_SIZE.y);
				ImU32 bgColor = ImGui::GetColorU32(IMGUI_FRAME_OVERLAY_COLOR); 
				ImGui::GetWindowDrawList()->AddRectFilled(bgMin, bgMax, bgColor);
			}

			ImGui::Image(self->resources->textures[TEXTURE_FRAME_ALTERNATE].handle, IMGUI_TIMELINE_FRAME_SIZE);

			ImGui::SameLine();
			ImGui::PopID();
		}

		frameFinishPos = ImGui::GetCursorPos();

		if (type == ANM2_TRIGGER)
		{
			std::vector<Anm2Trigger>* elementTriggers = (std::vector<Anm2Trigger>*)frames;

			for (auto [i, trigger] : std::views::enumerate(*elementTriggers))
			{
				ImVec2 triggerPos = framePos;
				triggerPos.x = framePos.x + (IMGUI_TIMELINE_FRAME_SIZE.x * trigger.atFrame);
				
				ImGui::SetCursorPos(triggerPos);

				ImGui::PushStyleColor(ImGuiCol_Border, IMGUI_FRAME_BORDER_COLOR);			
				ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, IMGUI_FRAME_BORDER);

				ImVec4 buttonColor = self->frameIndex == i && self->frameVector == elementTriggers ?
				ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered] : ImGui::GetStyle().Colors[ImGuiCol_Button];
				ImGui::PushStyleColor(ImGuiCol_Button, buttonColor);
					
				ImGui::PushID(i);
				if (ImGui::Button(STRING_IMGUI_TIMELINE_TRIGGER_LABEL, IMGUI_TIMELINE_FRAME_SIZE))
				{
					self->frameIndex = i;
					self->frameVector = elementTriggers;
					self->animationType = type;
				}

				ImGui::PopStyleVar();
				ImGui::PopStyleColor();
				ImGui::PopStyleColor();

				ImGui::SetCursorPos(ImVec2(triggerPos.x + 1.0f, (triggerPos.y + (IMGUI_TIMELINE_FRAME_SIZE.y / 2)) - IMGUI_ICON_SMALL_SIZE.y / 2));

				ImGui::Image(self->resources->textures[TEXTURE_TRIGGER_FRAME_ICON].handle, IMGUI_ICON_SMALL_SIZE);

				ImGui::PopID();
			}
		}
		else
		{
			std::vector<Anm2Frame>* elementFrames = (std::vector<Anm2Frame>*)frames;
			
			for (auto [i, frame] : std::views::enumerate(*elementFrames))
			{
				Texture* texture = frame.isInterpolated ? 
					&self->resources->textures[TEXTURE_INTERPOLATED_FRAME_ICON] : 
					&self->resources->textures[TEXTURE_UNINTERPOLATED_FRAME_ICON];
				
				f32 frameWidth = IMGUI_TIMELINE_FRAME_SIZE.x * frame.delay;
				ImVec2 frameSize = ImVec2(frameWidth, IMGUI_TIMELINE_FRAME_SIZE.y);
				
				ImGui::SetCursorPos(framePos);
				
				ImGui::PushStyleColor(ImGuiCol_Border, IMGUI_FRAME_BORDER_COLOR);			
				ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, IMGUI_FRAME_BORDER);

				ImVec4 buttonColor = self->frameIndex == i && self->frameVector == elementFrames ? 
				ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered] : ImGui::GetStyle().Colors[ImGuiCol_Button];
				ImGui::PushStyleColor(ImGuiCol_Button, buttonColor);
									
				ImGui::PushID(i);

				if (ImGui::Button(STRING_IMGUI_TIMELINE_FRAME_LABEL, frameSize))
				{
					self->frameIndex = i;
					self->frameVector = elementFrames;
					self->animationType = type;
				}

				ImGui::PopStyleVar();
				ImGui::PopStyleColor();
				ImGui::PopStyleColor();

				ImGui::SetCursorPos(ImVec2(framePos.x + 1.0f, (framePos.y + (frameSize.y / 2)) - IMGUI_ICON_SMALL_SIZE.y / 2));
				
				ImGui::Image(texture->handle, IMGUI_ICON_SMALL_SIZE);

				ImGui::PopID();
				
				framePos.x += frameWidth;
			}
		}

		ImGui::EndChild();
		ImGui::PopStyleVar();
		ImGui::PopStyleVar();

		ImGui::SetCursorPosX(cursorPos.x);
		ImGui::SetCursorPosY(cursorPos.y + IMGUI_TIMELINE_FRAME_SIZE.y);
	}
	else
		ImGui::Dummy(IMGUI_DUMMY_SIZE);

	*index = *index + 1;

	ImGui::PopID();
}

/* Displays each element of the timeline of a selected animation */
static void
_imgui_timeline_element(Imgui* self, void* element, s32* id, s32* index, Anm2AnimationType type)
{
		static s32 selectedSpritesheetIndex = -1;
		Anm2Animation* animation = &self->anm2->animations[self->animationID];
		Anm2Layer* layer = NULL;
		Anm2LayerAnimation* layerAnimation = NULL;
		Anm2Null* null = NULL;
		Anm2NullAnimation* nullAnimation = NULL;
		Anm2RootAnimation* rootAnimation = NULL;
		Anm2Triggers* triggers = NULL;
		void* frames = NULL;
		ImVec2 framePos;
		ImVec2 frameFinishPos;
		ImTextureID iconTexture = -1;
		bool isSelected = *index == self->timelineElementIndex;
		bool isArrows = false;
		bool* isShowRect = NULL;
		bool* isVisible = NULL;
		char nameBuffer[ANM2_STRING_MAX] = STRING_EMPTY;
		char nameVisible[ANM2_STRING_FORMATTED_MAX] = STRING_EMPTY;
		char* namePointer = NULL;
		s32* spritesheetID = NULL;
		bool isChangeable = type != ANM2_ROOT_ANIMATION && type != ANM2_TRIGGER;
		f32 cursorPosY = ImGui::GetCursorPosY();

		switch (type)
		{
			case ANM2_ROOT_ANIMATION:
				rootAnimation = (Anm2RootAnimation*)element;
				iconTexture = self->resources->textures[TEXTURE_ROOT].handle;
				strncpy(nameVisible, STRING_IMGUI_TIMELINE_ROOT, ANM2_STRING_FORMATTED_MAX);
				isVisible = &rootAnimation->isVisible;
				frames = &rootAnimation->frames;
				break;
			case ANM2_LAYER_ANIMATION:
				layerAnimation = (Anm2LayerAnimation*)element;
				layer = &self->anm2->layers[*id];
				iconTexture = self->resources->textures[TEXTURE_LAYER].handle;
				isVisible = &layerAnimation->isVisible;
				spritesheetID = &layer->spritesheetID;
				namePointer = layer->name;
				snprintf(nameBuffer, ANM2_STRING_MAX, "%s", namePointer);
				snprintf(nameVisible, ANM2_STRING_FORMATTED_MAX, STRING_IMGUI_TIMELINE_ELEMENT_FORMAT, *id, namePointer); 
				frames = &layerAnimation->frames;
				break;
			case ANM2_NULL_ANIMATION:
				nullAnimation = (Anm2NullAnimation*)element;
				null = &self->anm2->nulls[*id];
				iconTexture = self->resources->textures[TEXTURE_NULL].handle;
				isVisible = &nullAnimation->isVisible;
				isShowRect = &null->isShowRect;
				namePointer = null->name;
				snprintf(nameBuffer, ANM2_STRING_MAX, "%s", namePointer);
				snprintf(nameVisible, ANM2_STRING_FORMATTED_MAX, STRING_IMGUI_TIMELINE_ELEMENT_FORMAT, *id, namePointer); 
				frames = &nullAnimation->frames;
				break;
			case ANM2_TRIGGER:
				triggers = (Anm2Triggers*)element;
				iconTexture = self->resources->textures[TEXTURE_TRIGGER].handle;
				strncpy(nameVisible, STRING_IMGUI_TIMELINE_TRIGGERS, ANM2_STRING_FORMATTED_MAX);
				isVisible = &triggers->isVisible;
				frames = &triggers->items;
				break;
			default:
				break;
		}

		ImGui::PushID(*index);

		ImGui::BeginChild(nameVisible, IMGUI_TIMELINE_ELEMENT_SIZE, true, ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar);

		/* Shift arrows */
		if (isChangeable)
		{
			bool isSwap = false;
			bool isReversed = (type == ANM2_LAYER_ANIMATION);

			auto arrows_draw = [&](auto it, auto begin, auto end,
								   auto& map,
								   auto& allAnimationMaps, // vector of maps across all animations
								   bool* didSwap, bool isReversed)
			{
				bool canMoveUp   = isReversed ? (std::next(it) != end) : (it != begin);
				bool canMoveDown = isReversed ? (it != begin)          : (std::next(it) != end);

				isArrows = canMoveUp || canMoveDown;

				if (isArrows)
					ImGui::BeginChild(STRING_IMGUI_TIMELINE_ELEMENT_SHIFT_ARROWS_LABEL, IMGUI_TIMELINE_SHIFT_ARROWS_SIZE);

				if (canMoveUp)
				{
					auto target = isReversed ? std::next(it) : std::prev(it);
					if (target != map.end() && ImGui::ImageButton(STRING_IMGUI_TIMELINE_ELEMENT_SHIFT_ABOVE, self->resources->textures[TEXTURE_ARROW_UP].handle, IMGUI_ICON_SIZE))
					{
						map_swap(map, it->first, target->first);

						for (auto& animation : self->anm2->animations)
						{
							if (type == ANM2_LAYER_ANIMATION)
								map_swap(animation.second.layerAnimations, it->first, target->first);
							else if (type == ANM2_NULL_ANIMATION)
								map_swap(animation.second.nullAnimations, it->first, target->first);
						}

						*didSwap = true;
					}
					_imgui_tooltip(STRING_IMGUI_TOOLTIP_TIMELINE_ELEMENT_SHIFT_UP);
				}

				if (canMoveDown)
				{
					if (!canMoveUp)
					{
						ImGui::Dummy(IMGUI_ICON_BUTTON_SIZE);
						ImGui::SameLine(0.0f, ImGui::GetStyle().ItemSpacing.x); 
					}
					else
						ImGui::SameLine();

					auto target = isReversed ? std::prev(it) : std::next(it);
					if (target != map.end() && ImGui::ImageButton(STRING_IMGUI_TIMELINE_ELEMENT_SHIFT_BELOW, self->resources->textures[TEXTURE_ARROW_DOWN].handle, IMGUI_ICON_SIZE))
					{
						map_swap(map, it->first, target->first);

						for (auto& animation : self->anm2->animations)
						{
							if (type == ANM2_LAYER_ANIMATION)
									map_swap(animation.second.layerAnimations, it->first, target->first);
								else if (type == ANM2_NULL_ANIMATION)
									map_swap(animation.second.nullAnimations, it->first, target->first);
						}

						*didSwap = true;
					}
					_imgui_tooltip(STRING_IMGUI_TOOLTIP_TIMELINE_ELEMENT_SHIFT_DOWN);
				}

				if (isArrows)
				{
					ImGui::EndChild();
					ImGui::SameLine();
				}
			};

			if (type == ANM2_LAYER_ANIMATION)
			{
				auto it = std::find_if(self->anm2->layers.begin(), self->anm2->layers.end(),
					[&](const auto& pair) { return &pair.second == layer; });

				if (it != self->anm2->layers.end())
				{
					arrows_draw(it, self->anm2->layers.begin(), self->anm2->layers.end(),
								self->anm2->layers, self->anm2->animations, &isSwap, isReversed);
				}
			}

			if (type == ANM2_NULL_ANIMATION)
			{
				auto it = std::find_if(self->anm2->nulls.begin(), self->anm2->nulls.end(),
					[&](const auto& pair) { return &pair.second == null; });

				if (it != self->anm2->nulls.end())
				{
					arrows_draw(it, self->anm2->nulls.begin(), self->anm2->nulls.end(),
								self->anm2->nulls, self->anm2->animations, &isSwap, isReversed);
				}
			}
		}

		ImGui::Image(iconTexture, IMGUI_ICON_SIZE);
		
		ImGui::SameLine();

		ImGui::BeginChild(STRING_IMGUI_TIMELINE_ELEMENT_NAME_LABEL, IMGUI_TIMELINE_ELEMENT_NAME_SIZE);
		
		if (isSelected && isChangeable)
		{
			if (ImGui::InputText(STRING_IMGUI_TIMELINE_ANIMATION_LABEL, nameBuffer, ANM2_STRING_MAX, ImGuiInputTextFlags_EnterReturnsTrue))
				strncpy(namePointer, nameBuffer, ANM2_STRING_MAX);
		}
		else
		{
			if (ImGui::Selectable(nameVisible, isSelected))
			{
				self->frameVector = frames;
				self->animationType = type;
				self->timelineElementIndex = *index;
			}
		}
	
		switch (type)
		{
			case ANM2_ROOT_ANIMATION:
				_imgui_tooltip(STRING_IMGUI_TOOLTIP_TIMELINE_ELEMENT_ROOT);
				break;
			case ANM2_LAYER_ANIMATION:
				_imgui_tooltip(STRING_IMGUI_TOOLTIP_TIMELINE_ELEMENT_LAYER);
				break;
			case ANM2_NULL_ANIMATION:
				_imgui_tooltip(STRING_IMGUI_TOOLTIP_TIMELINE_ELEMENT_NULL);
				break;
			case ANM2_TRIGGER:
				_imgui_tooltip(STRING_IMGUI_TOOLTIP_TIMELINE_ELEMENT_TRIGGERS);
				break;
			default:
				break;
		}

		ImGui::EndChild();

		/* Visiblity */
		if (isVisible)
		{
			ImVec2 cursorPos;
			ImTextureID visibilityIcon = *isVisible
				? self->resources->textures[TEXTURE_EYE_OPEN].handle
				: self->resources->textures[TEXTURE_EYE_CLOSED].handle;

			ImGui::SameLine();
				
			cursorPos = ImGui::GetCursorPos();
			ImGui::SetCursorPosX(cursorPos.x + ImGui::GetContentRegionAvail().x - IMGUI_ICON_BUTTON_SIZE.x - ImGui::GetStyle().FramePadding.x * 2);

			if (ImGui::ImageButton(STRING_IMGUI_TIMELINE_VISIBLE, visibilityIcon, IMGUI_ICON_SIZE))
				*isVisible = !*isVisible;
			_imgui_tooltip(STRING_IMGUI_TOOLTIP_TIMELINE_ELEMENT_VISIBLE);

			ImGui::SetCursorPos(cursorPos);
		}
		
		/* Spritesheet IDs */
		if (spritesheetID)
		{
			char spritesheetIDName[ANM2_STRING_FORMATTED_MAX];

			if (*spritesheetID == -1)
				snprintf(spritesheetIDName, ANM2_STRING_FORMATTED_MAX, STRING_IMGUI_TIMELINE_SPRITESHEET_UNKNOWN);
			else
				snprintf(spritesheetIDName, ANM2_STRING_FORMATTED_MAX, STRING_IMGUI_TIMELINE_SPRITESHEET_FORMAT, *spritesheetID);

			ImGui::BeginChild(STRING_IMGUI_TIMELINE_ELEMENT_SPRITESHEET_ID_LABEL, IMGUI_TIMELINE_ELEMENT_SPRITESHEET_ID_SIZE);
				
			ImGui::Image(self->resources->textures[TEXTURE_SPRITESHEET].handle, IMGUI_ICON_SIZE);
			ImGui::SameLine();

			if (selectedSpritesheetIndex == *index)
			{
				if (ImGui::InputInt(STRING_IMGUI_TIMELINE_ELEMENT_SPRITESHEET_ID_LABEL, spritesheetID, 0, 0, ImGuiInputTextFlags_None))
					selectedSpritesheetIndex = -1; 
			}
			else
			{
				if (ImGui::Selectable(spritesheetIDName))
					selectedSpritesheetIndex = *index;
			}
			_imgui_tooltip(STRING_IMGUI_TOOLTIP_TIMELINE_ELEMENT_SPRITESHEET);

			ImGui::EndChild();
		}
		
		/* ShowRect */
		if (isShowRect)
		{
			ImTextureID rectIcon = *isShowRect
				? self->resources->textures[TEXTURE_RECT_SHOW].handle
				: self->resources->textures[TEXTURE_RECT_HIDE].handle;

			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - ((IMGUI_ICON_BUTTON_SIZE.x - ImGui::GetStyle().FramePadding.x * 2) * 4));
			
			if (ImGui::ImageButton(STRING_IMGUI_TIMELINE_RECT, rectIcon, IMGUI_ICON_SIZE))
				*isShowRect = !*isShowRect;

			_imgui_tooltip(STRING_IMGUI_TOOLTIP_TIMELINE_ELEMENT_RECT);
		}

		if (ImGui::IsMouseClicked(0) && !ImGui::IsAnyItemHovered() && ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
    	{
			self->timelineElementIndex = -1;
			selectedSpritesheetIndex = -1;
		}
		
		ImGui::EndChild();
		
		*index = *index + 1;

		ImGui::PopID();

		ImGui::SetCursorPosY(cursorPosY + IMGUI_TIMELINE_ELEMENT_SIZE.y);
}

/* Timeline */
static void
_imgui_timeline(Imgui* self)
{
	/* -- Timeline -- */
	ImGui::Begin(STRING_IMGUI_TIMELINE);

	if (self->animationID != -1)
	{
		ImVec2 cursorPos;
		ImVec2 mousePos;
		ImVec2 mousePosRelative;
		s32 index = 0;
		Anm2Animation* animation = &self->anm2->animations[self->animationID];
		ImVec2 frameSize = IMGUI_TIMELINE_FRAME_SIZE;
		ImVec2 pickerPos;
		ImVec2 lineStart;
		ImVec2 lineEnd;
		ImDrawList* drawList;
		static f32 elementScrollX = 0;
		static f32 elementScrollY = 0;
		ImVec2 frameIndicesSize = {frameSize.x * animation->frameNum, IMGUI_TIMELINE_FRAME_INDICES_SIZE.y};
		s32 idDefault = 0;
		const char* buttonText = self->preview->isPlaying ? STRING_IMGUI_TIMELINE_PAUSE : STRING_IMGUI_TIMELINE_PLAY;
		const char* buttonTooltipText = self->preview->isPlaying ? STRING_IMGUI_TOOLTIP_TIMELINE_PAUSE : STRING_IMGUI_TOOLTIP_TIMELINE_PLAY;
	
		ImVec2 timelineSize = {0, ImGui::GetContentRegionAvail().y - IMGUI_TIMELINE_OFFSET_Y};
		
		/* Generally, things need to be dranw out of apparent order for correct scrolling to work. */

		/* Main timeline child */
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::BeginChild(STRING_IMGUI_TIMELINE_CHILD, timelineSize, true);

		cursorPos = ImGui::GetCursorPos();

		drawList = ImGui::GetWindowDrawList();
	
		/* Element frames */
		ImGui::SetCursorPos(ImVec2(cursorPos.x + IMGUI_TIMELINE_ELEMENT_SIZE.x, cursorPos.y + IMGUI_TIMELINE_VIEWER_SIZE.y));
		ImGui::BeginChild(STRING_IMGUI_TIMELINE_ELEMENT_FRAMES, IMGUI_TIMELINE_ELEMENT_FRAMES_SIZE, true);
		ImGui::PopStyleVar();
		ImGui::PopStyleVar();
		elementScrollX = ImGui::GetScrollX();
		elementScrollY = ImGui::GetScrollY();
		
		_imgui_timeline_element_frames(self, &animation->rootAnimation, &idDefault, &index, ANM2_ROOT_ANIMATION);

		for (auto it = animation->layerAnimations.rbegin(); it != animation->layerAnimations.rend(); it++)
		{
			s32 id = it->first;
			Anm2LayerAnimation& layer = it->second;

			_imgui_timeline_element_frames(self, &layer, &id, &index, ANM2_LAYER_ANIMATION); 
		}

		for (auto & [id, null] : animation->nullAnimations)
			_imgui_timeline_element_frames(self, &null, (s32*)&id, &index, ANM2_NULL_ANIMATION);

		_imgui_timeline_element_frames(self, &animation->triggers, &idDefault, &index, ANM2_TRIGGER);

		ImGui::EndChild();

		ImGui::SetCursorPos(cursorPos);

		/* Element bar */
		ImGui::BeginChild(STRING_IMGUI_TIMELINE_ELEMENTS, IMGUI_TIMELINE_ELEMENT_SIZE, true);
		ImGui::Text(STRING_IMGUI_TIMELINE_ELEMENTS);
		
		ImGui::EndChild();

		/* Viewer */
		if (animation->frameNum > 0)
		{
			bool isMouseInElementsRegion = false;
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
			
			ImGui::SameLine();

			ImGui::BeginChild(STRING_IMGUI_TIMELINE_FRAME_INDICES, frameIndicesSize, true);
			ImGui::SetScrollX(elementScrollX);
			
			ImVec2 elementsRectMin = ImGui::GetWindowPos();
			ImVec2 elementsRectMax = ImVec2(elementsRectMin.x + frameIndicesSize.x, elementsRectMin.y + frameIndicesSize.y);
			
			cursorPos = ImGui::GetCursorScreenPos();
			mousePos = ImGui::GetMousePos();
			mousePosRelative = ImVec2(ImGui::GetMousePos().x - cursorPos.x, ImGui::GetMousePos().y - cursorPos.y);

			isMouseInElementsRegion =
				mousePos.x >= elementsRectMin.x && mousePos.x < elementsRectMax.x &&
				mousePos.y >= elementsRectMin.y && mousePos.y < elementsRectMax.y;
			
			if (isMouseInElementsRegion && ImGui::IsMouseDown(0) && !self->preview->isPlaying)
			{
				if (mousePosRelative.x >= 0 && mousePosRelative.x < frameIndicesSize.x)
				{
					s32 index = (s32)(mousePosRelative.x / frameSize.x);
					if (index >= 0 && index < animation->frameNum)
						self->preview->time = (f32)index;
				}
				else if (mousePosRelative.x < 0)
					self->preview->time = 0;
				else
					self->preview->time = (f32)(animation->frameNum - 1);
			}

			for (s32 i = 0; i < animation->frameNum; i++)
			{
				ImVec2 imagePos = ImGui::GetCursorScreenPos();

				if (i % IMGUI_TIMELINE_FRAME_INDICES_MULTIPLE == 0)
				{
					char frameIndexString[IMGUI_TIMELINE_FRAME_INDICES_STRING_MAX];
					snprintf(frameIndexString, IMGUI_TIMELINE_FRAME_INDICES_STRING_MAX, "%i", i);

					ImVec2 bgMin = imagePos;
					ImVec2 bgMax = ImVec2(imagePos.x + IMGUI_TIMELINE_FRAME_SIZE.x,
		                      imagePos.y + IMGUI_TIMELINE_FRAME_SIZE.y);

					ImU32 bgColor = ImGui::GetColorU32(IMGUI_FRAME_INDICES_OVERLAY_COLOR); 
					drawList->AddRectFilled(bgMin, bgMax, bgColor);

					ImVec2 textSize = ImGui::CalcTextSize(frameIndexString);

					ImVec2 textPos;
					textPos.x = imagePos.x + (IMGUI_TIMELINE_FRAME_SIZE.x - textSize.x) / 2.0f;
					textPos.y = imagePos.y + (IMGUI_TIMELINE_FRAME_SIZE.y - textSize.y) / 2.0f;

					drawList->AddText(textPos, ImGui::GetColorU32(ImGuiCol_Text), frameIndexString);
				}
				else
				{
					ImVec2 bgMin = imagePos;
					ImVec2 bgMax = ImVec2(imagePos.x + IMGUI_TIMELINE_FRAME_SIZE.x,
		                      imagePos.y + IMGUI_TIMELINE_FRAME_SIZE.y);

					ImU32 bgColor = ImGui::GetColorU32(IMGUI_FRAME_INDICES_COLOR); 
					drawList->AddRectFilled(bgMin, bgMax, bgColor);
				}

				ImGui::Image(self->resources->textures[TEXTURE_FRAME].handle, IMGUI_TIMELINE_FRAME_SIZE);

				ImGui::SameLine();
			}

			ImGui::PopStyleVar();
			ImGui::PopStyleVar();

			pickerPos = ImVec2(cursorPos.x + self->preview->time * frameSize.x, cursorPos.y);
			lineStart = ImVec2(pickerPos.x + frameSize.x / 2.0f, pickerPos.y + frameSize.y);
			lineEnd = ImVec2(lineStart.x, lineStart.y + timelineSize.y - IMGUI_TIMELINE_FRAME_SIZE.y);

			ImGui::GetWindowDrawList()->AddImage(
				self->resources->textures[TEXTURE_PICKER].handle,
				pickerPos,
				ImVec2(pickerPos.x + frameSize.x, pickerPos.y + frameSize.y)
			);
			
			ImGui::GetForegroundDrawList()->AddRectFilled(
				ImVec2(lineStart.x - IMGUI_PICKER_LINE_SIZE, lineStart.y),
				ImVec2(lineStart.x + IMGUI_PICKER_LINE_SIZE, lineEnd.y),
				IMGUI_PICKER_LINE_COLOR
			);
	
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
			ImGui::EndChild();
			ImGui::PopStyleVar();
			ImGui::PopStyleVar();
		}
		else
		{
			ImGui::SameLine();
			ImGui::Dummy(frameIndicesSize);		
		}

		/* Element list */
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::BeginChild(STRING_IMGUI_TIMELINE_ELEMENT_LIST, IMGUI_TIMELINE_ELEMENT_LIST_SIZE, true,  ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
		ImGui::PopStyleVar();
		ImGui::PopStyleVar();
		ImGui::SetScrollY(elementScrollY);

		_imgui_timeline_element(self, &animation->rootAnimation, &idDefault, &index, ANM2_ROOT_ANIMATION);

		for (auto it = animation->layerAnimations.rbegin(); it != animation->layerAnimations.rend(); it++)
		{
			s32 id = it->first;
			Anm2LayerAnimation& layer = it->second;

			_imgui_timeline_element(self, &layer, &id, &index, ANM2_LAYER_ANIMATION); 
		}

		for (auto & [id, null] : animation->nullAnimations)
			_imgui_timeline_element(self, &null, (s32*)&id, &index, ANM2_NULL_ANIMATION);

		_imgui_timeline_element(self, &animation->triggers, &idDefault, &index, ANM2_TRIGGER);

		ImGui::EndChild();

		ImGui::EndChild();

		/* Buttons */
		if (ImGui::Button(buttonText))
			self->preview->isPlaying = !self->preview->isPlaying;
		_imgui_tooltip(buttonTooltipText);

		ImGui::SameLine();
		
		if (ImGui::Button(STRING_IMGUI_TIMELINE_ELEMENT_ADD))
			ImGui::OpenPopup(STRING_IMGUI_TIMELINE_ELEMENT_ADD_MENU);
		_imgui_tooltip(STRING_IMGUI_TOOLTIP_TIMELINE_ELEMENT_ADD);
		
		if (ImGui::BeginPopup(STRING_IMGUI_TIMELINE_ELEMENT_ADD_MENU))
		{
			if (ImGui::Selectable(STRING_IMGUI_TIMELINE_ELEMENT_ADD_MENU_LAYER))
				anm2_layer_add(self->anm2);
			
			if (ImGui::Selectable(STRING_IMGUI_TIMELINE_ELEMENT_ADD_MENU_NULL))
				anm2_null_add(self->anm2);

			ImGui::EndPopup();
		}

		ImGui::SameLine();

		if 
		(
			ImGui::Button(STRING_IMGUI_TIMELINE_ELEMENT_REMOVE) &&
			self->animationID != -1
		)
		{
			switch (self->animationType)
			{
				case ANM2_LAYER_ANIMATION:
					anm2_layer_remove(self->anm2, self->animationID);
					break;
				case ANM2_NULL_ANIMATION:
					anm2_null_remove(self->anm2, self->animationID);
					break;
				default:
					break;
			}

			self->animationID = -1;
			self->timelineElementIndex = -1;
			self->animationType = ANM2_NONE;
		}
		_imgui_tooltip(STRING_IMGUI_TOOLTIP_TIMELINE_ELEMENT_REMOVE);

		ImGui::SameLine();

		if (ImGui::Button(STRING_IMGUI_TIMELINE_FRAME_ADD))
		{
			if (self->frameVector)
			{
				Anm2Animation* animation = &self->anm2->animations[self->animationID];
				std::vector<Anm2Frame>* addFrameVector;
				std::vector<Anm2Trigger>* addTriggerVector;
				Anm2Frame frame = Anm2Frame{};
				Anm2Trigger trigger = Anm2Trigger{};
				s32 delay = 0;
				s32 frameDelayCount = 0;
				s32 index = -1;

				switch (self->animationType)
				{
					case ANM2_ROOT_ANIMATION:
					case ANM2_LAYER_ANIMATION:
					case ANM2_NULL_ANIMATION:
						addFrameVector = (std::vector<Anm2Frame>*)self->frameVector;
						for (auto& frameCheck : *addFrameVector)
							frameDelayCount += frameCheck.delay;
						
						if (frameDelayCount + frame.delay > animation->frameNum)
							break;

						if (self->frameIndex > -1)
						{
							frame = (*addFrameVector)[self->frameIndex];

							if (frameDelayCount + frame.delay > animation->frameNum)
								frame.delay = animation->frameNum - frameDelayCount;

							if (frame.delay <= 0)
								break;

							index = self->frameIndex + 1;

							addFrameVector->insert(addFrameVector->begin() + index, frame);
						}
						else
						{
							index = (s32)addFrameVector->size();
							addFrameVector->push_back(frame);
						}

						self->frameIndex = index;
						break;
					case ANM2_TRIGGER:
						addTriggerVector = (std::vector<Anm2Trigger>*)self->frameVector;

						if (self->preview->time > -1)
						{
							index = (s32)self->preview->time;
							bool isTrigger = false;

        					for (const auto & existingTrigger : *addTriggerVector)
								if (existingTrigger.atFrame == index) 
								{
									isTrigger = true;
									break;
								}

							if (isTrigger)
								break;

							trigger.atFrame = index;
							addTriggerVector->push_back(trigger);

							self->frameIndex = index;
						}
						else
							break;

						break;
					default:
						break;
				}
			}
		}
		_imgui_tooltip(STRING_IMGUI_TOOLTIP_TIMELINE_FRAME_ADD);

		ImGui::SameLine();

		if (ImGui::Button(STRING_IMGUI_TIMELINE_FRAME_REMOVE))
		{
			if (self->frameVector && self->frameIndex > -1)
			{
					std::vector<Anm2Frame>* removeFrameVector;
					std::vector<Anm2Trigger>* removeTriggerVector;

					switch (self->animationType)
					{
						case ANM2_ROOT_ANIMATION:
						case ANM2_LAYER_ANIMATION:
						case ANM2_NULL_ANIMATION:
							removeFrameVector = (std::vector<Anm2Frame>*)self->frameVector;
							removeFrameVector->erase(removeFrameVector->begin() + self->frameIndex);
							break;
						case ANM2_TRIGGER:
							removeTriggerVector = (std::vector<Anm2Trigger>*)self->frameVector;
							removeTriggerVector->erase(removeTriggerVector->begin() + self->frameIndex);
							break;
						break;
						default:
							break;
					}

					self->frameIndex = -1;
					self->frameVector = NULL;
					self->animationType = ANM2_NONE;
			}
		}
		_imgui_tooltip(STRING_IMGUI_TOOLTIP_TIMELINE_FRAME_REMOVE);

		ImGui::SameLine();

		ImGui::SetNextItemWidth(IMGUI_TIMELINE_ANIMATION_LENGTH_WIDTH);
		ImGui::InputInt(STRING_IMGUI_TIMELINE_ANIMATION_LENGTH, &animation->frameNum);
		_imgui_tooltip(STRING_IMGUI_TOOLTIP_TIMELINE_ANIMATION_LENGTH);

		animation->frameNum = CLAMP(animation->frameNum, ANM2_FRAME_NUM_MIN, ANM2_FRAME_NUM_MAX);

		ImGui::SameLine();

		/* FPS */
		ImGui::SetNextItemWidth(IMGUI_TIMELINE_FPS_WIDTH);
		ImGui::SameLine();
		ImGui::InputInt(STRING_IMGUI_TIMELINE_FPS, &self->anm2->fps);
		_imgui_tooltip(STRING_IMGUI_TOOLTIP_TIMELINE_FPS);
		
		self->anm2->fps = CLAMP(self->anm2->fps, ANM2_FPS_MIN, ANM2_FPS_MAX);

		ImGui::SameLine();

		/* Loop */
		ImGui::Checkbox(STRING_IMGUI_TIMELINE_LOOP, &animation->isLoop);
		_imgui_tooltip(STRING_IMGUI_TOOLTIP_TIMELINE_LOOP);

		ImGui::SameLine();

		/* CreatedBy */
		ImGui::SetNextItemWidth(IMGUI_TIMELINE_CREATED_BY_WIDTH);
		ImGui::SameLine();
		ImGui::InputText(STRING_IMGUI_TIMELINE_CREATED_BY, self->anm2->createdBy, ANM2_STRING_MAX);
		_imgui_tooltip(STRING_IMGUI_TOOLTIP_TIMELINE_CREATED_BY);
		
		ImGui::SameLine();

		/* CreatedOn */
		ImGui::Text(STRING_IMGUI_TIMELINE_CREATED_ON, self->anm2->createdOn);

		ImGui::SameLine();

		/* Version */
		ImGui::Text(STRING_IMGUI_TIMELINE_VERSION, self->anm2->version);
	}

	ImGui::End();
}

/* Taskbar */
static void
_imgui_taskbar(Imgui* self)
{
	ImGuiWindowFlags taskbarWindowFlags = 0 |
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoScrollWithMouse |
		ImGuiWindowFlags_NoSavedSettings;

	ImGuiViewport* viewport = ImGui::GetMainViewport();
	
	ImGui::SetNextWindowPos(viewport->Pos);
	ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, IMGUI_TASKBAR_HEIGHT)); 

	ImGui::Begin(STRING_IMGUI_TASKBAR, NULL, taskbarWindowFlags);

	if (ImGui::Selectable(STRING_IMGUI_TASKBAR_FILE, false, 0, ImGui::CalcTextSize(STRING_IMGUI_TASKBAR_FILE)))
		ImGui::OpenPopup(STRING_IMGUI_FILE_MENU);

	if (ImGui::IsItemHovered() || ImGui::IsItemActive())
		ImGui::SetNextWindowPos(ImVec2(ImGui::GetItemRectMin().x, ImGui::GetItemRectMin().y + ImGui::GetItemRectSize().y));
		
	if (ImGui::BeginPopup(STRING_IMGUI_FILE_MENU))
	{
		if (ImGui::Selectable(STRING_IMGUI_FILE_NEW))
		{
			self->animationID = -1;
			self->spritesheetID = -1;
			self->eventID = -1;
			anm2_new(self->anm2);
		}

		if (ImGui::Selectable(STRING_IMGUI_FILE_OPEN))
		{
			self->animationID = -1;
			self->eventID = -1;
			self->spritesheetID = -1;
			dialog_anm2_open(self->dialog);
		}

		if (ImGui::Selectable(STRING_IMGUI_FILE_SAVE))
		{
			if (!strcmp(self->anm2->path, STRING_EMPTY) == 0)
				anm2_serialize(self->anm2, self->anm2->path);
			else 
				dialog_anm2_save(self->dialog);
		}

		if (ImGui::Selectable(STRING_IMGUI_FILE_SAVE_AS))
		{
			dialog_anm2_save(self->dialog);
		}
		
		ImGui::EndPopup();
	}

	ImGui::End();
}

/* Animations */
static void
_imgui_animations(Imgui* self)
{
	ImGui::Begin(STRING_IMGUI_ANIMATIONS);

	/* Iterate through all animations, can be selected and names can be edited */
	for (auto & [id, animation] : self->anm2->animations)
	{
		char name[ANM2_STRING_FORMATTED_MAX];
		char oldName[ANM2_STRING_MAX];
		bool isSelected = self->animationID == id;

		/* Distinguish default animation */
		if (strcmp(animation.name, self->anm2->defaultAnimation) == 0)
			snprintf(name, ANM2_STRING_FORMATTED_MAX, STRING_IMGUI_ANIMATIONS_DEFAULT_ANIMATION_FORMAT, animation.name);
		else
			strncpy(name, animation.name, ANM2_STRING_FORMATTED_MAX - 1);

		ImGui::PushID(id);

		ImGui::Image(self->resources->textures[TEXTURE_ANIMATION].handle, IMGUI_ICON_SIZE);
		ImGui::SameLine();

		if (isSelected)
		{
			if (ImGui::InputText(STRING_IMGUI_ANIMATIONS_ANIMATION_LABEL, animation.name, ANM2_STRING_MAX, ImGuiInputTextFlags_EnterReturnsTrue))
			{
					strncpy(self->anm2->defaultAnimation, animation.name, ANM2_STRING_MAX);

				self->animationID = -1;
			}
		}
		else
		{
			if (ImGui::Selectable(name, isSelected))
			{
				self->animationID = id;
				self->frameIndex = -1;
				self->frameVector = NULL;
				self->animationType = ANM2_NONE;
				self->preview->isPlaying = false;
				self->preview->time = 0.0f;
			}
		}

		_imgui_tooltip(STRING_IMGUI_TOOLTIP_ANIMATIONS_SELECT);

		ImGui::PopID();
	}

	if (ImGui::Button(STRING_IMGUI_ANIMATIONS_ADD))
	{
		bool isDefault = self->anm2->animations.size() == 0; /* first animation is default automatically */
		s32 id = anm2_animation_add(self->anm2);
		
		self->animationID = id;

		if (isDefault)
			strncpy(self->anm2->defaultAnimation, self->anm2->animations[id].name, ANM2_STRING_MAX);
	}
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_ANIMATIONS_ADD);
		
	ImGui::SameLine();

	/* Remove */
	if (ImGui::Button(STRING_IMGUI_ANIMATIONS_REMOVE))
	{
		if (self->animationID != -1)
		{
			anm2_animation_remove(self->anm2, self->animationID);
			self->animationID = -1;
		}
	}
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_ANIMATIONS_REMOVE);
	
	ImGui::SameLine();

	/* Duplicate */
	if (ImGui::Button(STRING_IMGUI_ANIMATIONS_DUPLICATE))
	{
		if (self->animationID > -1)
		{
			s32 id = map_next_id_get(self->anm2->animations);
			self->anm2->animations.insert({id, self->anm2->animations[self->animationID]});
			self->animationID = id;
		}
	}
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_ANIMATIONS_DUPLICATE);

	ImGui::SameLine();
	
	/* Set as default */
	if (ImGui::Button(STRING_IMGUI_ANIMATIONS_SET_AS_DEFAULT))
	{
		if (self->animationID > -1)
			strncpy(self->anm2->defaultAnimation, self->anm2->animations[self->animationID].name, ANM2_STRING_MAX);
	}
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_ANIMATIONS_SET_AS_DEFAULT);

	if (ImGui::IsMouseClicked(0) && !ImGui::IsAnyItemHovered() && ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
    	self->animationID = -1;

	ImGui::End();
}

/* Events */
static void
_imgui_events(Imgui* self)
{
	ImGui::Begin(STRING_IMGUI_EVENTS);

	/* Iterate through all events, can be selected and names can be edited */
	for (auto & [id, event] : self->anm2->events)
	{
		char eventString[ANM2_STRING_FORMATTED_MAX];
		bool isSelected;
		
		snprintf(eventString, ANM2_STRING_FORMATTED_MAX, STRING_IMGUI_EVENT_FORMAT, (s32)id, event.name); 

		ImGui::PushID(id);
		
		ImGui::Image(self->resources->textures[TEXTURE_EVENT].handle, IMGUI_ICON_SIZE);
		ImGui::SameLine();

		isSelected = self->eventID == id;
		
		if (isSelected)
		{
			if (ImGui::InputText(STRING_IMGUI_EVENTS_EVENT_LABEL, event.name, ANM2_STRING_MAX, ImGuiInputTextFlags_EnterReturnsTrue))
				self->eventID = -1;
		}
		else
		{
			if (ImGui::Selectable(eventString, isSelected))
				self->eventID = id;
		}

		_imgui_tooltip(STRING_IMGUI_TOOLTIP_EVENTS_SELECT);

		ImGui::PopID();
	}

	if (ImGui::Button(STRING_IMGUI_EVENTS_ADD))
	{
		s32 id = map_next_id_get(self->anm2->events);
		self->anm2->events[id] = Anm2Event{}; 
		self->eventID = id;
	}
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_EVENTS_ADD);
	
	ImGui::SameLine();
	
	if (ImGui::Button(STRING_IMGUI_EVENTS_REMOVE))
	{
		if (self->eventID != -1)
		{
			self->anm2->events.erase(self->eventID);
			self->eventID = -1;
		}
	}
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_EVENTS_REMOVE);

	if (ImGui::IsMouseClicked(0) && !ImGui::IsAnyItemHovered() && ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
    	self->eventID = -1;

	ImGui::End();
}

/* Spritesheets */
static void
_imgui_spritesheets(Imgui* self)
{
	ImGui::Begin(STRING_IMGUI_SPRITESHEETS);
	
	for (auto [id, spritesheet] : self->anm2->spritesheets)
	{
		ImVec2 spritesheetPreviewSize = IMGUI_SPRITESHEET_PREVIEW_SIZE;
		char spritesheetString[ANM2_STRING_FORMATTED_MAX];
		bool isSelected = false;

		f32 spritesheetAspect = (f32)self->resources->loadedTextures[id].size.x / self->resources->loadedTextures[id].size.y;

		if ((IMGUI_SPRITESHEET_PREVIEW_SIZE.x / IMGUI_SPRITESHEET_PREVIEW_SIZE.y) > spritesheetAspect)
			spritesheetPreviewSize.x = IMGUI_SPRITESHEET_PREVIEW_SIZE.y * spritesheetAspect;
		else
			spritesheetPreviewSize.y = IMGUI_SPRITESHEET_PREVIEW_SIZE.x / spritesheetAspect;

		snprintf(spritesheetString, ANM2_PATH_FORMATTED_MAX, STRING_IMGUI_SPRITESHEET_FORMAT, (s32)id, spritesheet.path);
	
		ImGui::BeginChild(spritesheetString, IMGUI_SPRITESHEET_SIZE, true, ImGuiWindowFlags_None);
		
		ImGui::PushID(id);

		ImGui::Image(self->resources->textures[TEXTURE_SPRITESHEET].handle, IMGUI_ICON_SIZE);
		ImGui::SameLine();

		isSelected = self->spritesheetID == id;
		if (ImGui::Selectable(spritesheetString, isSelected))
			self->spritesheetID = id;

		_imgui_tooltip(STRING_IMGUI_TOOLTIP_SPRITESHEETS_SELECT);
			
		ImGui::Image(self->resources->loadedTextures[id].handle, spritesheetPreviewSize);
			
		ImGui::PopID();
			
		ImGui::EndChild();
	}

	if (ImGui::Button(STRING_IMGUI_SPRITESHEETS_ADD))
		dialog_png_open(self->dialog);
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_SPRITESHEETS_ADD);
	
	ImGui::SameLine();

	/* Remove */
	if (ImGui::Button(STRING_IMGUI_SPRITESHEETS_REMOVE))
	{
		if (self->spritesheetID > -1)
		{
			self->resources->loadedTextures.erase(self->spritesheetID);
			self->anm2->spritesheets.erase(self->spritesheetID);
			self->spritesheetID = -1;
		}
	}
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_SPRITESHEETS_REMOVE);

	ImGui::SameLine();
	
	/* Reload */
	if (ImGui::Button(STRING_IMGUI_SPRITESHEETS_RELOAD))
	{
		if (self->spritesheetID > -1)
			anm2_spritesheet_texture_load(self->anm2, self->resources, self->anm2->spritesheets[self->spritesheetID].path, self->spritesheetID);
	}
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_SPRITESHEETS_RELOAD);
	ImGui::SameLine();
	
	/* Replace */
	if (ImGui::Button(STRING_IMGUI_SPRITESHEETS_REPLACE))
	{
		if (self->spritesheetID > -1)
		{
			self->dialog->replaceID = self->spritesheetID;
			dialog_png_replace(self->dialog);
		}
	}
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_SPRITESHEETS_REPLACE);

	if (ImGui::IsMouseClicked(0) && !ImGui::IsAnyItemHovered() && ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
    	self->spritesheetID = -1;
		
	ImGui::End();
}

/* Animation Preview */
static void
_imgui_animation_preview(Imgui* self)
{
	static bool isHoverPreview = false;
	static bool isPreviewCenter = false;
	
	ImGui::Begin(STRING_IMGUI_ANIMATION_PREVIEW, NULL, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
	
	/* Grid settings */
	ImGui::BeginChild(STRING_IMGUI_ANIMATION_PREVIEW_GRID_SETTINGS, IMGUI_ANIMATION_PREVIEW_SETTINGS_CHILD_SIZE, true);

	/* Grid toggle */
	ImGui::Checkbox(STRING_IMGUI_ANIMATION_PREVIEW_GRID, &self->settings->isGrid);
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_ANIMATION_PREVIEW_GRID);

	ImGui::SameLine();
	
	/* Grid Color */
	ImGui::ColorEdit4(STRING_IMGUI_ANIMATION_PREVIEW_GRID_COLOR, (f32*)&self->settings->gridColorR, ImGuiColorEditFlags_NoInputs);
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_ANIMATION_PREVIEW_GRID_COLOR);
	
	/* Grid Size */
	ImGui::InputInt2(STRING_IMGUI_ANIMATION_PREVIEW_GRID_SIZE, (s32*)&self->settings->gridSizeX);
	self->settings->gridSizeX = CLAMP(self->settings->gridSizeX, PREVIEW_GRID_MIN, PREVIEW_GRID_MAX);
	self->settings->gridSizeY = CLAMP(self->settings->gridSizeY, PREVIEW_GRID_MIN, PREVIEW_GRID_MAX);
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_ANIMATION_PREVIEW_GRID_SIZE);
	
	/* Grid Offset */
	ImGui::InputInt2(STRING_IMGUI_ANIMATION_PREVIEW_GRID_OFFSET, (s32*)&self->settings->gridOffsetX);
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_ANIMATION_PREVIEW_GRID_OFFSET);

	ImGui::EndChild();
	
	ImGui::SameLine();
	
	/* Helper settings */
	ImGui::BeginChild(STRING_IMGUI_ANIMATION_PREVIEW_HELPER_SETTINGS, IMGUI_ANIMATION_PREVIEW_SETTINGS_CHILD_SIZE, true);
	
	/* Axis toggle */
	ImGui::Checkbox(STRING_IMGUI_ANIMATION_PREVIEW_AXIS, &self->settings->isAxis);
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_ANIMATION_PREVIEW_AXIS);

	ImGui::SameLine();

	/* Axis colors */
	ImGui::ColorEdit4(STRING_IMGUI_ANIMATION_PREVIEW_AXIS_COLOR, (f32*)&self->settings->axisColorR, ImGuiColorEditFlags_NoInputs);
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_ANIMATION_PREVIEW_AXIS_COLOR);
	
	/* Root transform */
	ImGui::Checkbox(STRING_IMGUI_ANIMATION_PREVIEW_ROOT_TRANSFORM, &self->settings->isRootTransform);
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_ANIMATION_PREVIEW_ROOT_TRANSFORM);

	/* Show pivot */
	ImGui::Checkbox(STRING_IMGUI_ANIMATION_PREVIEW_SHOW_PIVOT, &self->settings->isShowPivot);
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_ANIMATION_PREVIEW_SHOW_PIVOT);

	ImGui::EndChild();
	
	ImGui::SameLine();
	
	/* View settings */
	ImGui::BeginChild(STRING_IMGUI_ANIMATION_PREVIEW_VIEW_SETTINGS, IMGUI_ANIMATION_PREVIEW_SETTINGS_CHILD_SIZE, true);

	/* Zoom */
	ImGui::DragFloat(STRING_IMGUI_ANIMATION_PREVIEW_ZOOM, &self->settings->zoom, 1, PREVIEW_ZOOM_MIN, PREVIEW_ZOOM_MAX, "%.0f");
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_ANIMATION_PREVIEW_ZOOM);
	
	/* Center view */
	if (ImGui::Button(STRING_IMGUI_ANIMATION_PREVIEW_CENTER_VIEW))
		isPreviewCenter = true;
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_ANIMATION_PREVIEW_CENTER_VIEW);
	
	ImGui::EndChild();

	ImGui::SameLine();
	
	/* Background settings */
	ImGui::BeginChild(STRING_IMGUI_ANIMATION_PREVIEW_BACKGROUND_SETTINGS, IMGUI_ANIMATION_PREVIEW_SETTINGS_CHILD_SIZE, true);

	/* Background color */
	ImGui::ColorEdit4(STRING_IMGUI_ANIMATION_PREVIEW_BACKGROUND_COLOR, (f32*)&self->settings->backgroundColorR, ImGuiColorEditFlags_NoInputs);
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_ANIMATION_PREVIEW_BACKGROUND_COLOR);
	
	ImGui::EndChild();

	/* Elements drawn out of order in order to get the size of the preview before how it visually appears */
	ImGui::Image(self->preview->texture, ImVec2(PREVIEW_SIZE.x, PREVIEW_SIZE.y));
	
	/* Panning */
	if (ImGui::IsItemHovered())
	{
		SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_MOVE));
		isHoverPreview = true;
		
		if (mouse_held(&self->input->mouse, MOUSE_LEFT))
		{
			self->settings->panX += self->input->mouse.delta.x;
			self->settings->panY -= self->input->mouse.delta.y;
		}

		self->settings->zoom = self->settings->zoom == PREVIEW_ZOOM_MIN ? 0 : self->settings->zoom;

		if (self->input->mouse.wheelDeltaY > 0)
			self->settings->zoom += PREVIEW_ZOOM_STEP;

		if (self->input->mouse.wheelDeltaY < 0)
			self->settings->zoom -= PREVIEW_ZOOM_STEP;

    	self->settings->zoom = CLAMP(self->settings->zoom, PREVIEW_ZOOM_MIN, PREVIEW_ZOOM_MAX);
	}
	else
	{
		if (isHoverPreview)
		{
			SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_DEFAULT));
			isHoverPreview = false;
		}
	}

	if (isPreviewCenter)
	{
		ImVec2 previewWindowRectSize = ImGui::GetCurrentWindow()->ClipRect.GetSize();

		/* Based on the preview's crop in its window, adjust the pan */
		self->settings->panX = PREVIEW_CENTER.x + ((previewWindowRectSize.x - PREVIEW_SIZE.x) / 2.0f);
		self->settings->panY = PREVIEW_CENTER.y - ((previewWindowRectSize.y - PREVIEW_SIZE.y) / 2.0f);

		self->settings->panY += (IMGUI_ANIMATION_PREVIEW_SETTINGS_SIZE.y / 2.0f);

		isPreviewCenter = false;
	}

	ImGui::End();
}

/* Spritesheet Editor */
static void
_imgui_spritesheet_editor(Imgui* self)
{
	ImGui::Begin(STRING_IMGUI_SPRITESHEET_EDITOR);
	
	ImGui::End();
}

/* Frame Properties */
static void
_imgui_frame_properties(Imgui* self)
{
	ImGui::Begin(STRING_IMGUI_FRAME_PROPERTIES);
		
	if (self->frameIndex > -1)
	{
		Anm2Animation* animation = &self->anm2->animations[self->animationID];

		std::vector<Anm2Frame>* frameVector;
		std::vector<Anm2Trigger>* triggerVector;
		Anm2Frame* frame = NULL;
		Anm2Trigger* trigger = NULL;
		std::vector<const char*> eventNames;
		std::vector<s32> eventIDs;
		static s32 selectedEventIndex = -1;

		switch (self->animationType)
		{
			case ANM2_ROOT_ANIMATION:
				ImGui::Text(STRING_IMGUI_FRAME_PROPERTIES_ROOT);
				break;
			case ANM2_LAYER_ANIMATION:
				ImGui::Text(STRING_IMGUI_FRAME_PROPERTIES_LAYER);
				break;
			case ANM2_NULL_ANIMATION:
				ImGui::Text(STRING_IMGUI_FRAME_PROPERTIES_NULL);
				break;
			case ANM2_TRIGGER:
				ImGui::Text(STRING_IMGUI_FRAME_PROPERTIES_TRIGGER);
				break;
			default:
				break;
		}

		switch (self->animationType)
		{
			case ANM2_ROOT_ANIMATION:
			case ANM2_NULL_ANIMATION:
				frameVector = (std::vector<Anm2Frame>*)self->frameVector;
				frame = (Anm2Frame*)&(*frameVector)[self->frameIndex];

				/* Position */	
				ImGui::DragFloat2(STRING_IMGUI_FRAME_PROPERTIES_POSITION, value_ptr(frame->position), 1, 0, 0, "%.0f");
				_imgui_tooltip(STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_POSITION);

				/* Scale */
				ImGui::DragFloat2(STRING_IMGUI_FRAME_PROPERTIES_SCALE, value_ptr(frame->scale), 1.0, 0, 0, "%.1f");
				_imgui_tooltip(STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_SCALE);
				
				/* Rotation */
				ImGui::DragFloat(STRING_IMGUI_FRAME_PROPERTIES_ROTATION, &frame->rotation, 1, 0, 0, "%.1f");
				_imgui_tooltip(STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_ROTATION);
				
				/* Duration */
				ImGui::InputInt(STRING_IMGUI_FRAME_PROPERTIES_DURATION, &frame->delay);
				_imgui_tooltip(STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_DURATION);
				frame->delay = CLAMP(frame->delay, 0, animation->frameNum + 1);
				
				/* Tint */
				ImGui::ColorEdit4(STRING_IMGUI_FRAME_PROPERTIES_TINT, value_ptr(frame->tintRGBA));
				_imgui_tooltip(STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_TINT);
				
				/* Color Offset */
				ImGui::ColorEdit3(STRING_IMGUI_FRAME_PROPERTIES_COLOR_OFFSET, value_ptr(frame->offsetRGB));
				_imgui_tooltip(STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_COLOR_OFFSET);
				
				/* Visible */
				ImGui::Checkbox(STRING_IMGUI_FRAME_PROPERTIES_VISIBLE, &frame->isVisible);
				_imgui_tooltip(STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_VISIBLE);
				
				ImGui::SameLine();
				
				ImGui::Checkbox(STRING_IMGUI_FRAME_PROPERTIES_INTERPOLATED, &frame->isInterpolated);
				_imgui_tooltip(STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_INTERPOLATED);
				break;
			case ANM2_LAYER_ANIMATION:
				frameVector = (std::vector<Anm2Frame>*)self->frameVector;
				frame = (Anm2Frame*)&(*frameVector)[self->frameIndex];
			
				/* Position */	
				ImGui::DragFloat2(STRING_IMGUI_FRAME_PROPERTIES_POSITION, value_ptr(frame->position), 1, 0, 0, "%.0f");
				_imgui_tooltip(STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_POSITION);

				/* Crop Position */
				ImGui::DragFloat2(STRING_IMGUI_FRAME_PROPERTIES_CROP_POSITION, value_ptr(frame->crop), 1, 0, 0, "%.0f");
				_imgui_tooltip(STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_CROP_POSITION);
				
				/* Crop */
				ImGui::DragFloat2(STRING_IMGUI_FRAME_PROPERTIES_CROP_SIZE, value_ptr(frame->size), 1, 0, 0, "%.0f");
				_imgui_tooltip(STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_CROP_SIZE);
				
				/* Pivot */
				ImGui::DragFloat2(STRING_IMGUI_FRAME_PROPERTIES_PIVOT, value_ptr(frame->pivot), 1, 0, 0, "%.0f");
				_imgui_tooltip(STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_PIVOT);

				/* Scale */
				ImGui::DragFloat2(STRING_IMGUI_FRAME_PROPERTIES_SCALE, value_ptr(frame->scale), 1.0, 0, 0, "%.1f");
				_imgui_tooltip(STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_SCALE);
				
				/* Rotation */
				ImGui::DragFloat(STRING_IMGUI_FRAME_PROPERTIES_ROTATION, &frame->rotation, 1, 0, 0, "%.1f");
				_imgui_tooltip(STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_ROTATION);
				
				/* Duration */
				ImGui::InputInt(STRING_IMGUI_FRAME_PROPERTIES_DURATION, &frame->delay);
				_imgui_tooltip(STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_DURATION);
				/* clamp delay */
				frame->delay = CLAMP(frame->delay, 0, animation->frameNum + 1);
				
				/* Tint */
				ImGui::ColorEdit4(STRING_IMGUI_FRAME_PROPERTIES_TINT, value_ptr(frame->tintRGBA));
				_imgui_tooltip(STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_TINT);
				
				/* Color Offset */
				ImGui::ColorEdit3(STRING_IMGUI_FRAME_PROPERTIES_COLOR_OFFSET, value_ptr(frame->offsetRGB));
				_imgui_tooltip(STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_COLOR_OFFSET);
				
				/* Visible */
				ImGui::Checkbox(STRING_IMGUI_FRAME_PROPERTIES_VISIBLE, &frame->isVisible);
				_imgui_tooltip(STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_VISIBLE);
				
				ImGui::SameLine();
				
				ImGui::Checkbox(STRING_IMGUI_FRAME_PROPERTIES_INTERPOLATED, &frame->isInterpolated);
				_imgui_tooltip(STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_INTERPOLATED);
				break;
			case ANM2_TRIGGER:
				triggerVector = (std::vector<Anm2Trigger>*)self->frameVector;
				trigger = (Anm2Trigger*)&(*triggerVector)[self->frameIndex];

				/* Events drop down; pick one! */
				for (auto & [id, event] : self->anm2->events) 
				{
					eventIDs.push_back(id);
					eventNames.push_back(event.name);
					if (id == trigger->eventID)
						selectedEventIndex = eventIDs.size() - 1;
				}
				
				if (ImGui::Combo(STRING_IMGUI_FRAME_PROPERTIES_EVENT, &selectedEventIndex, eventNames.data(), eventNames.size())) 
				{
    				trigger->eventID = eventIDs[selectedEventIndex];
					selectedEventIndex = -1;
				}
				_imgui_tooltip(STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_EVENT);

				/* At Frame */
				ImGui::InputInt(STRING_IMGUI_FRAME_PROPERTIES_AT_FRAME, &trigger->atFrame);
				_imgui_tooltip(STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_AT_FRAME);
				/* clamp at frame */
				trigger->atFrame = CLAMP(trigger->atFrame, 0, animation->frameNum- 1);
				
				break;
			default:
				break;
		}
	
	}

	ImGui::End();
}


void
imgui_init
(
	Imgui* self, 
	Dialog* dialog,
	Resources* resources,
	Input* input,
	Anm2* anm2,
	Preview* preview,
	Settings* settings,
	SDL_Window* window, 
	SDL_GLContext* glContext
)
{
	IMGUI_CHECKVERSION();

	self->dialog = dialog;
	self->resources = resources;
	self->input = input;
	self->anm2 = anm2;
	self->preview = preview;
	self->settings = settings;
	self->window = window;
	self->glContext = glContext;

	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	ImGui_ImplSDL3_InitForOpenGL(self->window, *self->glContext);
	ImGui_ImplOpenGL3_Init(STRING_OPENGL_VERSION);

	ImGuiIO& io = ImGui::GetIO();
	io.IniFilename = NULL;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	/* load ini manually */
	ImGui::LoadIniSettingsFromDisk(PATH_SETTINGS);
	
	printf(STRING_INFO_IMGUI_INIT);
}

/* Main dockspace */
static void
_imgui_dock(Imgui* self)
{
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	
	ImGuiWindowFlags dockspaceWindowFlags = 0 |
		ImGuiWindowFlags_NoTitleBar | 
		ImGuiWindowFlags_NoCollapse | 
		ImGuiWindowFlags_NoResize | 
		ImGuiWindowFlags_NoMove | 
		ImGuiWindowFlags_NoBringToFrontOnFocus | 
		ImGuiWindowFlags_NoNavFocus;

    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + IMGUI_TASKBAR_HEIGHT));
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, viewport->Size.y - IMGUI_TASKBAR_HEIGHT));
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGui::Begin(STRING_IMGUI_WINDOW, NULL, dockspaceWindowFlags);

    ImGui::DockSpace(ImGui::GetID(STRING_IMGUI_DOCKSPACE), ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

	_imgui_animations(self);
	_imgui_events(self);
	_imgui_spritesheets(self);
	_imgui_animation_preview(self);
	_imgui_spritesheet_editor(self);
	_imgui_timeline(self);
	_imgui_frame_properties(self);
	
	ImGui::End();
}

void
imgui_tick(Imgui* self)
{
	ImGui_ImplSDL3_NewFrame();
	ImGui_ImplOpenGL3_NewFrame();
	ImGui::NewFrame();

	_imgui_taskbar(self);
	_imgui_dock(self);

	self->preview->animationID = self->animationID;
}

void
imgui_draw(Imgui* self)
{
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void
imgui_free(Imgui* self)
{
	ImGui_ImplSDL3_Shutdown();
	ImGui_ImplOpenGL3_Shutdown();

	/* Save Ini manually */
	ImGui::SaveIniSettingsToDisk(PATH_SETTINGS);
	ImGui::DestroyContext();

	printf(STRING_INFO_IMGUI_FREE);
}
