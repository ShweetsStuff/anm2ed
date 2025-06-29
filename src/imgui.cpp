#include "imgui.h"

static void _imgui_tooltip(const char* tooltip);
static void _imgui_timeline_item_frames(Imgui* self, Anm2Reference reference, s32* index);
static void _imgui_timeline_item(Imgui* self, Anm2Reference reference, s32* index);
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

/* Displays the item's frames */
static void
_imgui_timeline_item_frames(Imgui* self, Anm2Reference reference, s32* index) 
{
	static s32 draggedFrameIndex = -1;
	static s32 hoveredFrameIndex = -1;
	ImVec2 frameStartPos;
	ImVec2 framePos;
	ImVec2 frameFinishPos;
	ImVec2 cursorPos = ImGui::GetCursorPos();
	ImVec4 frameColor;
	ImVec4 hoveredColor;
	ImVec4 activeColor;
	
	Anm2Animation* animation = anm2_animation_from_id(self->anm2, *self->animationID);
	Anm2Item* item = anm2_item_from_reference(self->anm2, &reference, *self->animationID);

	switch (reference.type)
	{
		case ANM2_ROOT:
			frameColor = IMGUI_TIMELINE_ROOT_FRAME_COLOR;
			hoveredColor = IMGUI_TIMELINE_ROOT_HIGHLIGHT_COLOR;
			activeColor = IMGUI_TIMELINE_ROOT_ACTIVE_COLOR;
			break;
		case ANM2_LAYER:
			frameColor = IMGUI_TIMELINE_LAYER_FRAME_COLOR;
			hoveredColor = IMGUI_TIMELINE_LAYER_HIGHLIGHT_COLOR;
			activeColor = IMGUI_TIMELINE_LAYER_ACTIVE_COLOR;
			break;
		case ANM2_NULL:
			frameColor = IMGUI_TIMELINE_NULL_FRAME_COLOR;
			hoveredColor = IMGUI_TIMELINE_NULL_HIGHLIGHT_COLOR;
			activeColor = IMGUI_TIMELINE_NULL_ACTIVE_COLOR;
			break;
		case ANM2_TRIGGERS:
			frameColor = IMGUI_TIMELINE_TRIGGERS_FRAME_COLOR;
			hoveredColor = IMGUI_TIMELINE_TRIGGERS_HIGHLIGHT_COLOR;
			activeColor = IMGUI_TIMELINE_TRIGGERS_ACTIVE_COLOR;
			break;
		default:
			break;
	}

	ImGui::PushID(*index);

	if (animation->frameNum > 0)
	{
		ImVec2 frameListSize = {IMGUI_TIMELINE_FRAME_SIZE.x * animation->frameNum, IMGUI_TIMELINE_ELEMENTS_TIMELINE_SIZE.y};
		ImVec2 mousePosRelative;

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

		ImGui::BeginChild(STRING_IMGUI_TIMELINE_FRAMES, frameListSize, true);

		/* will deselect frame if hovering and click; but, if it's later clicked, this won't have any effect */
		if (ImGui::IsWindowHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
			*self->reference = Anm2Reference{};

		vec2 mousePos = VEC2_IMVEC2(ImGui::GetMousePos());
		vec2 windowPos = VEC2_IMVEC2(ImGui::GetWindowPos());

		f32 scrollX = ImGui::GetScrollX();
		f32 mousePosRelativeX = mousePos.x - windowPos.x - scrollX;

		frameStartPos = ImGui::GetCursorPos();

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

			ImGui::Image(self->resources->atlas.id, IMGUI_TIMELINE_FRAME_SIZE, IMVEC2_ATLAS_UV_GET(TEXTURE_FRAME_ALT));

			ImGui::SameLine();
			ImGui::PopID();
		}

		for (auto [i, frame] : std::views::enumerate(item->frames))
		{
			reference.index = i;

			TextureType textureType;
			f32 frameWidth = IMGUI_TIMELINE_FRAME_SIZE.x * frame.delay;
			ImVec2 frameSize = ImVec2(frameWidth, IMGUI_TIMELINE_FRAME_SIZE.y);

			if (reference.type == ANM2_TRIGGERS)
			{
				framePos.x = frameStartPos.x + (IMGUI_TIMELINE_FRAME_SIZE.x * frame.atFrame);
				textureType = TEXTURE_TRIGGER;
			}
			else
				textureType = frame.isInterpolated ? TEXTURE_CIRCLE : TEXTURE_SQUARE;

			ImGui::SetCursorPos(framePos);
			
			ImVec4 buttonColor = *self->reference == reference ? activeColor : frameColor;

			ImGui::PushStyleColor(ImGuiCol_Button, buttonColor);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoveredColor);			
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, activeColor);			
			ImGui::PushStyleColor(ImGuiCol_Border, IMGUI_FRAME_BORDER_COLOR);			
			ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, IMGUI_FRAME_BORDER);
			
			ImGui::PushID(i);

			if (ImGui::Button(STRING_IMGUI_TIMELINE_FRAME_LABEL, frameSize))
			{
				s32 frameTime = (s32)(mousePosRelativeX / IMGUI_TIMELINE_FRAME_SIZE.x);

				*self->reference = reference;

				self->preview->time = frameTime;

				if (reference.type == ANM2_LAYER)
					*self->spritesheetID = self->anm2->layers[reference.id].spritesheetID; 
			}

			if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
			{
				*self->reference = reference;

				ImGui::SetDragDropPayload(STRING_IMGUI_TIMELINE_FRAME_DRAG_DROP, &reference, sizeof(Anm2Reference));
				ImGui::Button(STRING_IMGUI_TIMELINE_FRAME_LABEL, frameSize);
				ImGui::SetCursorPos(ImVec2(1.0f, (IMGUI_TIMELINE_FRAME_SIZE.y / 2) - (TEXTURE_SIZE_SMALL.y / 2)));
				ImGui::Image(self->resources->atlas.id, IMVEC2_VEC2(ATLAS_SIZES[textureType]), IMVEC2_ATLAS_UV_GET(textureType));
				ImGui::EndDragDropSource();
			}

			if (self->reference->id == reference.id)
			{
				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(STRING_IMGUI_TIMELINE_FRAME_DRAG_DROP))
					{
						Anm2Reference checkReference = *(Anm2Reference*)payload->Data;
						if (checkReference != reference) 
						{
							self->isSwap = true;
							self->swapReference = reference;
						}
					}
					ImGui::EndDragDropTarget();
				}
			}

			ImGui::PopStyleVar();
			ImGui::PopStyleColor(4);

			ImGui::SetCursorPos(ImVec2(framePos.x + 1.0f, (framePos.y + (IMGUI_TIMELINE_FRAME_SIZE.y / 2)) - TEXTURE_SIZE_SMALL.y / 2));

			ImGui::Image(self->resources->atlas.id, IMVEC2_VEC2(ATLAS_SIZES[textureType]), IMVEC2_ATLAS_UV_GET(textureType));

			ImGui::PopID();

			if (reference.type != ANM2_TRIGGERS) 
				framePos.x += frameWidth;
		}

		ImGui::EndChild();
		ImGui::PopStyleVar(2);

		ImGui::SetCursorPosX(cursorPos.x);
		ImGui::SetCursorPosY(cursorPos.y + IMGUI_TIMELINE_FRAME_SIZE.y);
	}
	else
		ImGui::Dummy(IMGUI_DUMMY_SIZE);

	(*index)++;

	ImGui::PopID();
}

/* Displays each item of the timeline of a selected animation */
static void
_imgui_timeline_item(Imgui* self, Anm2Reference reference, s32* index) 
{
	static s32 textEntryItemIndex = -1;
	static s32 textEntrySpritesheetIndex = -1;

	TextureType textureType = TEXTURE_ERROR;
	bool isArrows = false;
	s32* spritesheetID = NULL;
	bool* isShowRect = NULL;
	char* namePointer = NULL;
	Anm2Null* null = NULL;
	Anm2Layer* layer = NULL;
	char nameBuffer[ANM2_STRING_MAX] = STRING_EMPTY;
	char nameVisible[ANM2_STRING_FORMATTED_MAX] = STRING_EMPTY;
	
	bool isChangeable = reference.type != ANM2_ROOT && reference.type != ANM2_TRIGGERS;
	bool isSelected = self->reference->id == reference.id && self->reference->type == reference.type;
	bool isTextEntry = textEntryItemIndex == *index && isChangeable;
	bool isSpritesheetTextEntry = textEntrySpritesheetIndex == *index; 

	f32 cursorPosY = ImGui::GetCursorPosY();
	ImVec4 color;

	Anm2Animation* animation = anm2_animation_from_id(self->anm2, *self->animationID);
	Anm2Item* item = anm2_item_from_reference(self->anm2, &reference, *self->animationID);

	switch (reference.type)
	{
		case ANM2_ROOT:
			textureType = TEXTURE_ROOT;
			color = IMGUI_TIMELINE_ROOT_COLOR;
			strncpy(nameVisible, STRING_IMGUI_TIMELINE_ROOT, ANM2_STRING_FORMATTED_MAX);
			break;
		case ANM2_LAYER:
			textureType = TEXTURE_LAYER;
			color = IMGUI_TIMELINE_LAYER_COLOR;
			layer = &self->anm2->layers[reference.id];
			spritesheetID = &layer->spritesheetID;
			namePointer = layer->name;
			snprintf(nameBuffer, ANM2_STRING_MAX, "%s", namePointer);
			snprintf(nameVisible, ANM2_STRING_FORMATTED_MAX, STRING_IMGUI_TIMELINE_ELEMENT_FORMAT, reference.id, namePointer); 
			break;
		case ANM2_NULL:
			textureType = TEXTURE_NULL;
			color = IMGUI_TIMELINE_NULL_COLOR;
			null = &self->anm2->nulls[reference.id];
			isShowRect = &null->isShowRect;
			namePointer = null->name;
			snprintf(nameBuffer, ANM2_STRING_MAX, "%s", namePointer);
			snprintf(nameVisible, ANM2_STRING_FORMATTED_MAX, STRING_IMGUI_TIMELINE_ELEMENT_FORMAT, reference.id, namePointer); 
			break;
		case ANM2_TRIGGERS:
			textureType = TEXTURE_TRIGGERS;
			color = IMGUI_TIMELINE_TRIGGERS_COLOR;
			strncpy(nameVisible, STRING_IMGUI_TIMELINE_TRIGGERS, ANM2_STRING_FORMATTED_MAX);
			break;
		default:
			break;
	}

	ImGui::PushID(*index);

	ImGui::PushStyleColor(ImGuiCol_ChildBg, color);
	ImGui::BeginChild(nameVisible, IMGUI_TIMELINE_ELEMENT_SIZE, true, ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar);
	ImGui::PopStyleColor();
	
	ImGui::Image(self->resources->atlas.id, IMVEC2_VEC2(ATLAS_SIZES[textureType]), IMVEC2_ATLAS_UV_GET(textureType));
	
	ImGui::SameLine();

	ImGui::BeginChild(STRING_IMGUI_TIMELINE_ELEMENT_NAME_LABEL, IMGUI_TIMELINE_ELEMENT_NAME_SIZE);

	if (isTextEntry)
	{
		if (ImGui::InputText(STRING_IMGUI_TIMELINE_ANIMATION_LABEL, nameBuffer, ANM2_STRING_MAX, ImGuiInputTextFlags_EnterReturnsTrue))
		{
			strncpy(namePointer, nameBuffer, ANM2_STRING_MAX);
			textEntryItemIndex = -1;
		}

		if (!ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
			textEntryItemIndex = -1;
	}
	else
	{
		if (ImGui::Selectable(nameVisible, isSelected))
		{
			*self->reference = reference;
			self->reference->index = -1;

			if (reference.type == ANM2_LAYER)
				*self->spritesheetID = self->anm2->layers[reference.id].spritesheetID; 
		}

		if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
			textEntryItemIndex = *index;
	}

	if (isChangeable && ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
	{
		*self->reference = reference;
		self->reference->index = -1;

		ImGui::PushStyleColor(ImGuiCol_ChildBg, color);
		ImGui::SetDragDropPayload(STRING_IMGUI_TIMELINE_ITEM_DRAG_DROP, &reference, sizeof(Anm2Frame));
		ImGui::PopStyleColor();
		ImGui::Image(self->resources->atlas.id, IMVEC2_VEC2(ATLAS_SIZES[textureType]), IMVEC2_ATLAS_UV_GET(textureType));
		ImGui::SameLine();
		ImGui::Text(nameVisible);
		ImGui::EndDragDropSource();
	}

	if (self->reference->type == reference.type && ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(STRING_IMGUI_TIMELINE_ITEM_DRAG_DROP))
		{
			Anm2Reference checkReference = *(Anm2Reference*)payload->Data;
			if (checkReference != reference) 
			{
				self->isSwap = true;
				self->swapReference = reference;
			}
		}
		ImGui::EndDragDropTarget();
	}

	switch (reference.type)
	{
		case ANM2_ROOT:
			_imgui_tooltip(STRING_IMGUI_TOOLTIP_TIMELINE_ELEMENT_ROOT);
			break;
		case ANM2_LAYER:
			_imgui_tooltip(STRING_IMGUI_TOOLTIP_TIMELINE_ELEMENT_LAYER);
			break;
		case ANM2_NULL:
			_imgui_tooltip(STRING_IMGUI_TOOLTIP_TIMELINE_ELEMENT_NULL);
			break;
		case ANM2_TRIGGERS:
			_imgui_tooltip(STRING_IMGUI_TOOLTIP_TIMELINE_ELEMENT_TRIGGERS);
			break;
		default:
			break;
	}

	ImGui::EndChild();

	/* IsVisible */
	ImVec2 cursorPos;
	TextureType visibleTextureType = item->isVisible ? TEXTURE_VISIBLE : TEXTURE_INVISIBLE;

	ImGui::SameLine();
		
	cursorPos = ImGui::GetCursorPos();
	ImGui::SetCursorPosX(cursorPos.x + ImGui::GetContentRegionAvail().x - IMGUI_ICON_BUTTON_SIZE.x - ImGui::GetStyle().FramePadding.x * 2);

	if (ImGui::ImageButton(STRING_IMGUI_TIMELINE_VISIBLE, self->resources->atlas.id, IMVEC2_VEC2(ATLAS_SIZES[visibleTextureType]), IMVEC2_ATLAS_UV_GET(visibleTextureType)))
		item->isVisible = !item->isVisible;
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_TIMELINE_ELEMENT_VISIBLE);

	ImGui::SetCursorPos(cursorPos);

	/* Spritesheet IDs */
	if (spritesheetID)
	{
		char spritesheetIDName[ANM2_STRING_FORMATTED_MAX];

		if (*spritesheetID == -1)
			snprintf(spritesheetIDName, ANM2_STRING_FORMATTED_MAX, STRING_IMGUI_TIMELINE_SPRITESHEET_UNKNOWN);
		else
			snprintf(spritesheetIDName, ANM2_STRING_FORMATTED_MAX, STRING_IMGUI_TIMELINE_SPRITESHEET_FORMAT, *spritesheetID);

		ImGui::BeginChild(STRING_IMGUI_TIMELINE_ELEMENT_SPRITESHEET_ID_LABEL, IMGUI_TIMELINE_ELEMENT_SPRITESHEET_ID_SIZE);
			
		ImGui::Image(self->resources->atlas.id, IMVEC2_VEC2(ATLAS_SIZES[TEXTURE_SPRITESHEET]), IMVEC2_ATLAS_UV_GET(TEXTURE_SPRITESHEET));
		ImGui::SameLine();

		if (isSpritesheetTextEntry)
		{
			if (ImGui::InputInt(STRING_IMGUI_TIMELINE_ELEMENT_SPRITESHEET_ID_LABEL, spritesheetID, 0, 0, ImGuiInputTextFlags_None))
				textEntrySpritesheetIndex = -1; 

			if (!ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
				textEntrySpritesheetIndex = -1;
		}
		else
		{
			ImGui::Selectable(spritesheetIDName);

			if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
				textEntrySpritesheetIndex = *index;
		}
		_imgui_tooltip(STRING_IMGUI_TOOLTIP_TIMELINE_ELEMENT_SPRITESHEET);

		ImGui::EndChild();
	}
	
	/* ShowRect */
	if (isShowRect)
	{
		TextureType rectTextureType = *isShowRect ? TEXTURE_RECT : TEXTURE_RECT_HIDE;

		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - ((IMGUI_ICON_BUTTON_SIZE.x - ImGui::GetStyle().FramePadding.x * 2) * 4));
		
		if (ImGui::ImageButton(STRING_IMGUI_TIMELINE_RECT, self->resources->atlas.id, IMVEC2_VEC2(ATLAS_SIZES[rectTextureType]), IMVEC2_ATLAS_UV_GET(rectTextureType)))
			*isShowRect = !*isShowRect;

		_imgui_tooltip(STRING_IMGUI_TOOLTIP_TIMELINE_ELEMENT_RECT);
	}
	
	ImGui::EndChild();
	
	(*index)++; 

	ImGui::PopID();

	ImGui::SetCursorPosY(cursorPosY + IMGUI_TIMELINE_ELEMENT_SIZE.y);
}

/* Timeline */
static void
_imgui_timeline(Imgui* self)
{
	/* -- Timeline -- */
	ImGui::Begin(STRING_IMGUI_TIMELINE);

	Anm2Animation* animation = anm2_animation_from_id(self->anm2, *self->animationID);

	if (animation)
	{
		ImVec2 cursorPos;
		ImVec2 mousePos;
		ImVec2 mousePosRelative;
		s32 index = 0;
		ImVec2 frameSize = IMGUI_TIMELINE_FRAME_SIZE;
		ImVec2 pickerPos;
		ImVec2 lineStart;
		ImVec2 lineEnd;
		ImDrawList* drawList;
		static f32 itemScrollX = 0;
		static f32 itemScrollY = 0;
		static bool isPickerDragging;
		ImVec2 frameIndicesSize = {frameSize.x * animation->frameNum, IMGUI_TIMELINE_FRAME_INDICES_SIZE.y};
		s32 idDefault = 0;
		const char* buttonText = self->preview->isPlaying ? STRING_IMGUI_TIMELINE_PAUSE : STRING_IMGUI_TIMELINE_PLAY;
		const char* buttonTooltipText = self->preview->isPlaying ? STRING_IMGUI_TOOLTIP_TIMELINE_PAUSE : STRING_IMGUI_TOOLTIP_TIMELINE_PLAY;
		ImVec2 region = ImGui::GetContentRegionAvail();
		ImVec2 windowSize;

		ImVec2 timelineSize = {region.x, region.y - IMGUI_TIMELINE_OFFSET_Y};
		
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::BeginChild(STRING_IMGUI_TIMELINE_CHILD, timelineSize, true);
		windowSize = ImGui::GetWindowSize();

		cursorPos = ImGui::GetCursorPos();

		drawList = ImGui::GetWindowDrawList();
	
		ImGui::SetCursorPos(ImVec2(cursorPos.x + IMGUI_TIMELINE_ELEMENT_SIZE.x, cursorPos.y + IMGUI_TIMELINE_VIEWER_SIZE.y));
		ImGui::BeginChild(STRING_IMGUI_TIMELINE_ELEMENT_FRAMES, {0, 0}, false, ImGuiWindowFlags_HorizontalScrollbar);
		ImGui::PopStyleVar();
		ImGui::PopStyleVar();
		itemScrollX = ImGui::GetScrollX();
		itemScrollY = ImGui::GetScrollY();

		_imgui_timeline_item_frames(self, Anm2Reference{ANM2_ROOT, 0, 0}, &index);

		for (auto it = animation->layerAnimations.rbegin(); it != animation->layerAnimations.rend(); it++)
		{
			s32 id = it->first;

			_imgui_timeline_item_frames(self, Anm2Reference{ANM2_LAYER, id, 0}, &index);
		}

		for (auto & [id, null] : animation->nullAnimations)
			_imgui_timeline_item_frames(self, Anm2Reference{ANM2_NULL, id, 0}, &index);

		_imgui_timeline_item_frames(self, Anm2Reference{ANM2_TRIGGERS, 0, 0}, &index);

		if (self->isSwap)
		{
			Anm2Item* item = anm2_item_from_reference(self->anm2, self->reference, *self->animationID);
			Anm2Frame* aFrame;
			Anm2Frame* bFrame;
			Anm2Frame oldFrame;

			aFrame = anm2_frame_from_reference(self->anm2, self->reference, *self->animationID);
			bFrame = anm2_frame_from_reference(self->anm2, &self->swapReference, *self->animationID);

			oldFrame = *aFrame;
			*aFrame = *bFrame;
			*bFrame = oldFrame;

			self->isSwap = false;
			self->reference->index = self->swapReference.index;
			self->swapReference = Anm2Reference{};
		}

		ImGui::EndChild();

		ImGui::SetCursorPos(cursorPos);

		ImGui::PushStyleColor(ImGuiCol_ChildBg, IMGUI_TIMELINE_HEADER_COLOR);
		ImGui::BeginChild(STRING_IMGUI_TIMELINE_HEADER, IMGUI_TIMELINE_ELEMENT_SIZE, true);
		ImGui::EndChild();
		ImGui::PopStyleColor();

		if (animation->frameNum > 0)
		{
			bool isMouseInElementsRegion = false;

			ImVec2 cursorScreenPos = ImGui::GetCursorScreenPos();
			ImVec2 clipRectMin = {cursorScreenPos.x + IMGUI_TIMELINE_ELEMENT_SIZE.x, 0};
			ImVec2 clipRectMax = {cursorScreenPos.x + timelineSize.x + IMGUI_TIMELINE_FRAME_SIZE.x, cursorScreenPos.y + timelineSize.y};
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

			ImGui::SameLine();
			
			ImGui::PushClipRect(clipRectMin, clipRectMax, true);

			ImGui::BeginChild(STRING_IMGUI_TIMELINE_FRAME_INDICES, {0, IMGUI_TIMELINE_FRAME_SIZE.y});
			ImGui::SetScrollX(itemScrollX);
			
			ImVec2 itemsRectMin = ImGui::GetWindowPos();
			ImVec2 itemsRectMax = ImVec2(itemsRectMin.x + frameIndicesSize.x, itemsRectMin.y + frameIndicesSize.y);
			
			cursorPos = ImGui::GetCursorScreenPos();
			mousePos = ImGui::GetMousePos();
			mousePosRelative = ImVec2(ImGui::GetMousePos().x - cursorPos.x, ImGui::GetMousePos().y - cursorPos.y);

			isMouseInElementsRegion =
				mousePos.x >= itemsRectMin.x && mousePos.x < itemsRectMax.x &&
				mousePos.y >= itemsRectMin.y && mousePos.y < itemsRectMax.y;
			
			if ((isMouseInElementsRegion && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) || isPickerDragging)
			{
					s32 frameIndex = CLAMP((s32)(mousePosRelative.x / frameSize.x), 0, (f32)(animation->frameNum - 1));
					self->preview->time = frameIndex;

					isPickerDragging = true;
			}

			if (isPickerDragging && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
				isPickerDragging = false;

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

				ImGui::Image(self->resources->atlas.id, IMVEC2_VEC2(ATLAS_SIZES[TEXTURE_FRAME]), IMVEC2_ATLAS_UV_GET(TEXTURE_FRAME));

				ImGui::SameLine();
			}

			ImGui::PopStyleVar();
			ImGui::PopStyleVar();

			pickerPos = ImVec2(cursorPos.x + self->preview->time * frameSize.x, cursorPos.y);
			lineStart = ImVec2(pickerPos.x + frameSize.x / 2.0f, pickerPos.y + frameSize.y);
			lineEnd = ImVec2(lineStart.x, lineStart.y + timelineSize.y - IMGUI_TIMELINE_FRAME_SIZE.y);

			ImGui::GetWindowDrawList()->AddImage
			(
				self->resources->atlas.id,
				pickerPos,
				ImVec2(pickerPos.x + frameSize.x, pickerPos.y + frameSize.y),
				IMVEC2_ATLAS_UV_GET(TEXTURE_PICKER)
			);
			
			drawList->AddRectFilled
			(
				ImVec2(lineStart.x - IMGUI_PICKER_LINE_SIZE, lineStart.y),
				ImVec2(lineStart.x + IMGUI_PICKER_LINE_SIZE, lineEnd.y),
				IMGUI_PICKER_LINE_COLOR
			);

			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
			ImGui::EndChild();
			ImGui::PopClipRect();
			ImGui::PopStyleVar();
			ImGui::PopStyleVar();
		}
		else
		{
			ImGui::SameLine();
			ImGui::Dummy(frameIndicesSize);		
		}

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::BeginChild(STRING_IMGUI_TIMELINE_ELEMENT_LIST, IMGUI_TIMELINE_ELEMENT_LIST_SIZE, true,  ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
		ImGui::PopStyleVar();
		ImGui::PopStyleVar();
		ImGui::SetScrollY(itemScrollY);

		index = 0;

		_imgui_timeline_item(self, Anm2Reference{ANM2_ROOT, 0, 0}, &index);

		for (auto it = animation->layerAnimations.rbegin(); it != animation->layerAnimations.rend(); it++)
		{
			s32 id = it->first;
			_imgui_timeline_item(self, Anm2Reference{ANM2_LAYER, id, 0}, &index); 
		}

		for (auto & [id, null] : animation->nullAnimations)
			_imgui_timeline_item(self, Anm2Reference{ANM2_NULL, id, 0}, &index); 

		_imgui_timeline_item(self, Anm2Reference{ANM2_TRIGGERS, 0, 0}, &index); 

		if (self->isSwap)
		{
			Anm2Animation* animation = anm2_animation_from_id(self->anm2, *self->animationID);

			switch (self->reference->type)
			{
				case ANM2_LAYER:
					map_swap(self->anm2->layers, self->reference->id, self->swapReference.id);
					map_swap(animation->layerAnimations, self->reference->id, self->swapReference.id);
					break;
				case ANM2_NULL:
					map_swap(self->anm2->nulls, self->reference->id, self->swapReference.id);
					map_swap(animation->nullAnimations, self->reference->id, self->swapReference.id);
					break;
				default:
					break;
			}

			self->isSwap = false;
			self->reference->id = self->swapReference.id;
			self->swapReference = Anm2Reference{};
		}

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

		if (ImGui::Button(STRING_IMGUI_TIMELINE_ELEMENT_REMOVE))
		{
			switch (self->reference->type)
			{
				case ANM2_LAYER:
					anm2_layer_remove(self->anm2, self->reference->id);
					break;
				case ANM2_NULL:
					anm2_null_remove(self->anm2, self->reference->id);
					break;
				default:
					break;
			}

			*self->reference = Anm2Reference{};
		}
		_imgui_tooltip(STRING_IMGUI_TOOLTIP_TIMELINE_ELEMENT_REMOVE);

		ImGui::SameLine();

		if (ImGui::Button(STRING_IMGUI_TIMELINE_FRAME_ADD))
			anm2_frame_add(self->anm2, self->reference, *self->animationID, (s32)self->preview->time);
		_imgui_tooltip(STRING_IMGUI_TOOLTIP_TIMELINE_FRAME_ADD);

		ImGui::SameLine();

		if (ImGui::Button(STRING_IMGUI_TIMELINE_FRAME_REMOVE))
		{
			Anm2Frame* frame = anm2_frame_from_reference(self->anm2, self->reference, *self->animationID);

			if (frame)
			{
				Anm2Item* item = anm2_item_from_reference(self->anm2, self->reference, *self->animationID);
				item->frames.erase(item->frames.begin() + index);
				self->reference->index = -1;
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
			*self->reference = Anm2Reference{};
			anm2_new(self->anm2);
		}

		if (ImGui::Selectable(STRING_IMGUI_FILE_OPEN))
			dialog_anm2_open(self->dialog);

		if (ImGui::Selectable(STRING_IMGUI_FILE_SAVE))
		{
			if (!strcmp(self->anm2->path, STRING_EMPTY) == 0)
				anm2_serialize(self->anm2, self->anm2->path);
			else 
				dialog_anm2_save(self->dialog);
		}

		if (ImGui::Selectable(STRING_IMGUI_FILE_SAVE_AS))
			dialog_anm2_save(self->dialog);
		
		ImGui::EndPopup();
	}

	ImGui::End();
}

/* Tools */
static void
_imgui_tools(Imgui* self)
{
	ImGui::Begin(STRING_IMGUI_TOOLS);

	ImVec2 availableSize = ImGui::GetContentRegionAvail();
	f32 availableWidth = availableSize.x;

	s32 buttonsPerRow = availableWidth / TEXTURE_SIZE.x + IMGUI_TOOLS_WIDTH_INCREMENT;
	buttonsPerRow = MIN(buttonsPerRow, 1);

	for (s32 i = 0; i < TOOL_COUNT; i++)
	{
		const char* string = NULL;
		const char* tooltip = NULL;
		TextureType textureType;

		if (i > 0 && i % buttonsPerRow != 0)
			ImGui::SameLine();

		ImVec4 buttonColor = self->tool->type == (ToolType)i ? ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered] : ImGui::GetStyle().Colors[ImGuiCol_Button];
		ImGui::PushStyleColor(ImGuiCol_Button, buttonColor);

		switch (i)
		{
			case TOOL_PAN:
				string = STRING_IMGUI_TOOLS_PAN;
				tooltip = STRING_IMGUI_TOOLTIP_TOOLS_PAN;
				textureType = TEXTURE_PAN;
				break;
			case TOOL_MOVE:
				string = STRING_IMGUI_TOOLS_MOVE;
				tooltip = STRING_IMGUI_TOOLTIP_TOOLS_MOVE;
				textureType = TEXTURE_MOVE;
				break;
			case TOOL_ROTATE:
				string = STRING_IMGUI_TOOLS_ROTATE;
				tooltip = STRING_IMGUI_TOOLTIP_TOOLS_ROTATE;
				textureType = TEXTURE_ROTATE;
				break;
			case TOOL_SCALE:
				string = STRING_IMGUI_TOOLS_SCALE;
				tooltip = STRING_IMGUI_TOOLTIP_TOOLS_SCALE;
				textureType = TEXTURE_SCALE;
				break;
			case TOOL_CROP:
				string = STRING_IMGUI_TOOLS_CROP;
				tooltip = STRING_IMGUI_TOOLTIP_TOOLS_CROP;
				textureType = TEXTURE_CROP;
				break;
			default:
				break;
		}

		if (ImGui::ImageButton(string, self->resources->atlas.id, IMVEC2_VEC2(ATLAS_SIZES[textureType]), IMVEC2_ATLAS_UV_GET(textureType)))
				self->tool->type = (ToolType)i;

		_imgui_tooltip(tooltip);

		ImGui::PopStyleColor();
	}

	ImGui::End();

}

/* Animations */
static void
_imgui_animations(Imgui* self)
{
	static s32 textEntryAnimationID = -1;

	ImGui::Begin(STRING_IMGUI_ANIMATIONS);

	/* Iterate through all animations, can be selected and names can be edited */
	for (auto & [id, animation] : self->anm2->animations)
	{
		char name[ANM2_STRING_FORMATTED_MAX];
		char oldName[ANM2_STRING_MAX];
		bool isSelected =  *self->animationID == id; 
		bool isTextEntry = textEntryAnimationID == id;

		/* Distinguish default animation */
		if (strcmp(animation.name, self->anm2->defaultAnimation) == 0)
			snprintf(name, ANM2_STRING_FORMATTED_MAX, STRING_IMGUI_ANIMATIONS_DEFAULT_ANIMATION_FORMAT, animation.name);
		else
			strncpy(name, animation.name, ANM2_STRING_FORMATTED_MAX - 1);

		ImGui::PushID(id);

		ImGui::Image(self->resources->atlas.id, IMVEC2_VEC2(ATLAS_SIZES[TEXTURE_ANIMATION]), IMVEC2_ATLAS_UV_GET(TEXTURE_ANIMATION));
		ImGui::SameLine();

		if (isTextEntry)
		{
			if (ImGui::InputText(STRING_IMGUI_ANIMATIONS_ANIMATION_LABEL, animation.name, ANM2_STRING_MAX, ImGuiInputTextFlags_EnterReturnsTrue))
				textEntryAnimationID = -1;

			if (!ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
				textEntryAnimationID = -1;
		}
		else
		{
			if (ImGui::Selectable(name, isSelected))
			{
				*self->animationID = id;
				*self->reference = Anm2Reference{};
				self->preview->isPlaying = false;
				self->preview->time = 0.0f;
			}

			if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
				textEntryAnimationID = id;

			if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
			{
				ImGui::SetDragDropPayload(STRING_IMGUI_ANIMATIONS_DRAG_DROP, &id, sizeof(s32));
				ImGui::Image(self->resources->atlas.id, IMVEC2_VEC2(ATLAS_SIZES[TEXTURE_ANIMATION]), IMVEC2_ATLAS_UV_GET(TEXTURE_ANIMATION));
				ImGui::SameLine();
				ImGui::Text(name);
				ImGui::EndDragDropSource();
			}

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(STRING_IMGUI_ANIMATIONS_DRAG_DROP))
				{
					s32 sourceID = *(s32*)payload->Data;
					if (sourceID != id)
						map_swap(self->anm2->animations, sourceID, id);
				}
				ImGui::EndDragDropTarget();
			}
		}
		_imgui_tooltip(STRING_IMGUI_TOOLTIP_ANIMATIONS_SELECT);

		ImGui::PopID();
	}
	
	if (ImGui::Button(STRING_IMGUI_ANIMATIONS_ADD))
	{
		bool isDefault = (s32)self->anm2->animations.size() == 0; /* first animation is default automatically */
		s32 id = anm2_animation_add(self->anm2);
		
		*self->animationID = id;

		if (isDefault)
			strncpy(self->anm2->defaultAnimation, self->anm2->animations[id].name, ANM2_STRING_MAX);
	}
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_ANIMATIONS_ADD);
		
	ImGui::SameLine();

	/* Remove */
	if (ImGui::Button(STRING_IMGUI_ANIMATIONS_REMOVE))
	{
		if (*self->animationID > -1)
		{
			anm2_animation_remove(self->anm2, *self->animationID);
			*self->animationID = -1;
		}
	}
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_ANIMATIONS_REMOVE);
	
	ImGui::SameLine();

	/* Duplicate */
	if (ImGui::Button(STRING_IMGUI_ANIMATIONS_DUPLICATE))
	{
		if (*self->animationID > -1)
		{
			s32 id = map_next_id_get(self->anm2->animations);
			self->anm2->animations.insert({id, self->anm2->animations[*self->animationID]});
			*self->animationID = id;
		}
	}
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_ANIMATIONS_DUPLICATE);

	ImGui::SameLine();
	
	/* Set as default */
	if (ImGui::Button(STRING_IMGUI_ANIMATIONS_SET_AS_DEFAULT))
	{
		if (*self->animationID > -1)
			strncpy(self->anm2->defaultAnimation, self->anm2->animations[*self->animationID].name, ANM2_STRING_MAX);
	}
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_ANIMATIONS_SET_AS_DEFAULT);

	if (ImGui::IsMouseClicked(0) && !ImGui::IsAnyItemHovered() && ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
    	*self->animationID = -1;

	ImGui::End();
}

/* Events */
static void
_imgui_events(Imgui* self)
{
	static s32 selectedEventID = -1;
	static s32 textEntryEventID = -1;

	ImGui::Begin(STRING_IMGUI_EVENTS);

	/* Iterate through all events, can be selected and names can be edited */
	for (auto & [id, event] : self->anm2->events)
	{
		char eventString[ANM2_STRING_FORMATTED_MAX];
		bool isSelected = selectedEventID == id;
		bool isTextEntry = textEntryEventID == id;
		
		snprintf(eventString, ANM2_STRING_FORMATTED_MAX, STRING_IMGUI_EVENT_FORMAT, (s32)id, event.name); 

		ImGui::PushID(id);
		
		ImGui::Image(self->resources->atlas.id, IMVEC2_VEC2(ATLAS_SIZES[TEXTURE_EVENT]), IMVEC2_ATLAS_UV_GET(TEXTURE_EVENT));
		ImGui::SameLine();

		if (isTextEntry)
		{
			if (ImGui::InputText(STRING_IMGUI_ANIMATIONS_ANIMATION_LABEL, event.name, ANM2_STRING_MAX, ImGuiInputTextFlags_EnterReturnsTrue))
			{
				selectedEventID = -1;
				textEntryEventID = -1;
			}

			if (!ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
				textEntryEventID = -1;
		}
		else
		{
			if (ImGui::Selectable(eventString, isSelected))
				selectedEventID = id;

			if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
				textEntryEventID = id;

			if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
			{
				ImGui::SetDragDropPayload(STRING_IMGUI_EVENTS_DRAG_DROP, &id, sizeof(s32));
				ImGui::Image(self->resources->atlas.id, IMVEC2_VEC2(ATLAS_SIZES[TEXTURE_ANIMATION]), IMVEC2_ATLAS_UV_GET(TEXTURE_EVENT));
				ImGui::SameLine();
				ImGui::Text(eventString);
				ImGui::EndDragDropSource();
			}

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(STRING_IMGUI_EVENTS_DRAG_DROP))
				{
					s32 sourceID = *(s32*)payload->Data;
					if (sourceID != id)
						map_swap(self->anm2->events, sourceID, id);
				}
				ImGui::EndDragDropTarget();
			}
		}
		_imgui_tooltip(STRING_IMGUI_TOOLTIP_EVENTS_SELECT);

		ImGui::PopID();
	}
	
	if (ImGui::Button(STRING_IMGUI_EVENTS_ADD))
	{
		s32 id = map_next_id_get(self->anm2->events);
		self->anm2->events[id] = Anm2Event{}; 
		selectedEventID = id;
	}
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_EVENTS_ADD);
	
	ImGui::SameLine();
	
	if (ImGui::Button(STRING_IMGUI_EVENTS_REMOVE))
	{
		if (selectedEventID != -1)
		{
			self->anm2->events.erase(selectedEventID);
			selectedEventID = -1;
		}
	}
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_EVENTS_REMOVE);

	if (ImGui::IsMouseClicked(0) && !ImGui::IsAnyItemHovered() && ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
		selectedEventID = -1;

	ImGui::End();
}

/* Spritesheets */
static void
_imgui_spritesheets(Imgui* self)
{
	static s32 selectedSpritesheetID = -1;

	ImGui::Begin(STRING_IMGUI_SPRITESHEETS);
	
	for (auto [id, spritesheet] : self->anm2->spritesheets)
	{
		ImVec2 spritesheetPreviewSize = IMGUI_SPRITESHEET_PREVIEW_SIZE;
		char spritesheetString[ANM2_STRING_FORMATTED_MAX];
		bool isSelected = selectedSpritesheetID == id; 
		Texture* texture = &self->resources->textures[id];
		snprintf(spritesheetString, ANM2_PATH_FORMATTED_MAX, STRING_IMGUI_SPRITESHEET_FORMAT, (s32)id, spritesheet.path);
	
		ImGui::BeginChild(spritesheetString, IMGUI_SPRITESHEET_SIZE, true, ImGuiWindowFlags_None);
		
		ImGui::PushID(id);

		ImGui::Image(self->resources->atlas.id, IMVEC2_VEC2(ATLAS_SIZES[TEXTURE_SPRITESHEET]), IMVEC2_ATLAS_UV_GET(TEXTURE_SPRITESHEET));
		ImGui::SameLine();

		if (ImGui::Selectable(spritesheetString, isSelected))
			selectedSpritesheetID = id;
		_imgui_tooltip(STRING_IMGUI_TOOLTIP_SPRITESHEETS_SELECT);

		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
		{
			ImGui::SetDragDropPayload(STRING_IMGUI_SPRITESHEETS_DRAG_DROP, &id, sizeof(s32));
			ImGui::Image(self->resources->atlas.id, IMVEC2_VEC2(ATLAS_SIZES[TEXTURE_SPRITESHEET]), IMVEC2_ATLAS_UV_GET(TEXTURE_SPRITESHEET));
			ImGui::SameLine();
			ImGui::Text(spritesheetString);
			ImGui::EndDragDropSource();
		}

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(STRING_IMGUI_SPRITESHEETS_DRAG_DROP))
			{
				s32 sourceID = *(s32*)payload->Data;
				if (sourceID != id)
				{
					map_swap(self->anm2->spritesheets, sourceID, id);
					map_swap(self->resources->textures, sourceID, id);
				}
			}
			ImGui::EndDragDropTarget();
		}

		f32 spritesheetAspect = (f32)self->resources->textures[id].size.x / self->resources->textures[id].size.y;

		if ((IMGUI_SPRITESHEET_PREVIEW_SIZE.x / IMGUI_SPRITESHEET_PREVIEW_SIZE.y) > spritesheetAspect)
			spritesheetPreviewSize.x = IMGUI_SPRITESHEET_PREVIEW_SIZE.y * spritesheetAspect;
		else
			spritesheetPreviewSize.y = IMGUI_SPRITESHEET_PREVIEW_SIZE.x / spritesheetAspect;

		if (texture->isInvalid)
			ImGui::Image(self->resources->atlas.id, IMVEC2_VEC2(ATLAS_SIZES[TEXTURE_ERROR]), IMVEC2_ATLAS_UV_GET(TEXTURE_ERROR));
		else
			ImGui::Image(texture->id, spritesheetPreviewSize);
			
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
		if (selectedSpritesheetID > -1)
		{
			self->resources->textures.erase(selectedSpritesheetID);
			self->anm2->spritesheets.erase(selectedSpritesheetID);
			selectedSpritesheetID = -1;
		}
	}
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_SPRITESHEETS_REMOVE);

	ImGui::SameLine();
	
	/* Reload */
	if (ImGui::Button(STRING_IMGUI_SPRITESHEETS_RELOAD))
	{
		if (selectedSpritesheetID > -1)
			anm2_spritesheet_texture_load(self->anm2, self->resources, self->anm2->spritesheets[selectedSpritesheetID].path, selectedSpritesheetID);
	}
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_SPRITESHEETS_RELOAD);
	ImGui::SameLine();
	
	/* Replace */
	if (ImGui::Button(STRING_IMGUI_SPRITESHEETS_REPLACE))
	{
		if (selectedSpritesheetID > -1)
		{
			self->dialog->replaceID = selectedSpritesheetID;
			dialog_png_replace(self->dialog);
		}
	}
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_SPRITESHEETS_REPLACE);

	if (ImGui::IsMouseClicked(0) && !ImGui::IsAnyItemHovered() && ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
    	selectedSpritesheetID = -1;
		
	ImGui::End();
}

/* Animation Preview */
static void
_imgui_animation_preview(Imgui* self)
{
	static bool isPreviewHover = false;
	static bool isPreviewCenter = false;
	static vec2 mousePos = {0, 0};
	char mousePositionString[IMGUI_POSITION_STRING_MAX];

	memset(mousePositionString, '\0', IMGUI_POSITION_STRING_MAX);

	snprintf(mousePositionString, IMGUI_POSITION_STRING_MAX, STRING_IMGUI_ANIMATION_PREVIEW_POSITION_FORMAT, mousePos.x, mousePos.y);

	ImGui::Begin(STRING_IMGUI_ANIMATION_PREVIEW, NULL, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
	
	/* Grid settings */
	ImGui::BeginChild(STRING_IMGUI_ANIMATION_PREVIEW_GRID_SETTINGS, IMGUI_ANIMATION_PREVIEW_SETTINGS_CHILD_SIZE, true);

	/* Grid toggle */
	ImGui::Checkbox(STRING_IMGUI_ANIMATION_PREVIEW_GRID, &self->settings->previewIsGrid);
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_ANIMATION_PREVIEW_GRID);

	ImGui::SameLine();
	
	/* Grid Color */
	ImGui::ColorEdit4(STRING_IMGUI_ANIMATION_PREVIEW_GRID_COLOR, (f32*)&self->settings->previewGridColorR, ImGuiColorEditFlags_NoInputs);
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_ANIMATION_PREVIEW_GRID_COLOR);
	
	/* Grid Size */
	ImGui::InputInt2(STRING_IMGUI_ANIMATION_PREVIEW_GRID_SIZE, (s32*)&self->settings->previewGridSizeX);
	self->settings->previewGridSizeX = CLAMP(self->settings->previewGridSizeX, PREVIEW_GRID_MIN, PREVIEW_GRID_MAX);
	self->settings->previewGridSizeY = CLAMP(self->settings->previewGridSizeY, PREVIEW_GRID_MIN, PREVIEW_GRID_MAX);
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_ANIMATION_PREVIEW_GRID_SIZE);
	
	/* Grid Offset */
	ImGui::InputInt2(STRING_IMGUI_ANIMATION_PREVIEW_GRID_OFFSET, (s32*)&self->settings->previewGridOffsetX);
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_ANIMATION_PREVIEW_GRID_OFFSET);

	ImGui::EndChild();
	
	ImGui::SameLine();
	
	ImGui::SameLine();
	
	/* View settings */
	ImGui::BeginChild(STRING_IMGUI_ANIMATION_PREVIEW_VIEW_SETTINGS, IMGUI_ANIMATION_PREVIEW_SETTINGS_CHILD_SIZE, true);

	/* Zoom */
	ImGui::DragFloat(STRING_IMGUI_ANIMATION_PREVIEW_ZOOM, &self->settings->previewZoom, 1, PREVIEW_ZOOM_MIN, PREVIEW_ZOOM_MAX, "%.0f");
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_ANIMATION_PREVIEW_ZOOM);
	
	/* Center view */
	if (ImGui::Button(STRING_IMGUI_ANIMATION_PREVIEW_CENTER_VIEW))
		isPreviewCenter = true;
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_ANIMATION_PREVIEW_CENTER_VIEW);
	
	/* Mouse position */
	ImGui::Text(mousePositionString);

	ImGui::EndChild();

	ImGui::SameLine();
	
	/* Background settings */
	ImGui::BeginChild(STRING_IMGUI_ANIMATION_PREVIEW_BACKGROUND_SETTINGS, IMGUI_ANIMATION_PREVIEW_SETTINGS_CHILD_SIZE, true);

	/* Background color */
	ImGui::ColorEdit4(STRING_IMGUI_ANIMATION_PREVIEW_BACKGROUND_COLOR, (f32*)&self->settings->previewBackgroundColorR, ImGuiColorEditFlags_NoInputs);
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_ANIMATION_PREVIEW_BACKGROUND_COLOR);
	
	ImGui::EndChild();

	ImGui::SameLine();

	/* Helper settings */
	ImGui::BeginChild(STRING_IMGUI_ANIMATION_PREVIEW_HELPER_SETTINGS, IMGUI_ANIMATION_PREVIEW_SETTINGS_CHILD_SIZE, true);
	
	/* Axis toggle */
	ImGui::Checkbox(STRING_IMGUI_ANIMATION_PREVIEW_AXIS, &self->settings->previewIsAxis);
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_ANIMATION_PREVIEW_AXIS);

	ImGui::SameLine();

	/* Axis colors */
	ImGui::ColorEdit4(STRING_IMGUI_ANIMATION_PREVIEW_AXIS_COLOR, (f32*)&self->settings->previewAxisColorR, ImGuiColorEditFlags_NoInputs);
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_ANIMATION_PREVIEW_AXIS_COLOR);
	
	/* Root transform */
	ImGui::Checkbox(STRING_IMGUI_ANIMATION_PREVIEW_ROOT_TRANSFORM, &self->settings->previewIsRootTransform);
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_ANIMATION_PREVIEW_ROOT_TRANSFORM);

	/* Show pivot */
	ImGui::Checkbox(STRING_IMGUI_ANIMATION_PREVIEW_SHOW_PIVOT, &self->settings->previewIsShowPivot);
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_ANIMATION_PREVIEW_SHOW_PIVOT);

	ImGui::EndChild();

	/* Animation preview texture */
	vec2 previewPos = VEC2_IMVEC2(ImGui::GetCursorPos());

	ImGui::Image(self->preview->texture, IMVEC2_VEC2(PREVIEW_SIZE));

	/* Using tools when hovered */
	if (ImGui::IsItemHovered())
	{
		vec2 windowPos = VEC2_IMVEC2(ImGui::GetWindowPos());

		mousePos = VEC2_IMVEC2(ImGui::GetMousePos());

		mousePos -= (windowPos + previewPos);
		mousePos -= (PREVIEW_SIZE / 2.0f);
		mousePos.x += self->settings->previewPanX;
		mousePos.y += self->settings->previewPanY;
		mousePos.x /= (self->settings->previewZoom / 100.0f);
		mousePos.y /= (self->settings->previewZoom / 100.0f);

		Anm2Frame* frame = anm2_frame_from_reference(self->anm2, self->reference, *self->animationID);

		if (self->reference->type == ANM2_TRIGGERS) 
			frame = NULL;

		/* allow use of keybinds for tools */
		self->tool->isEnabled = true;

		switch (self->tool->type)
		{
			case TOOL_PAN:
				SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_POINTER));
				
				if (mouse_held(&self->input->mouse, MOUSE_LEFT))
				{
					self->settings->previewPanX -= self->input->mouse.delta.x;
					self->settings->previewPanY -= self->input->mouse.delta.y;
				}
				break;
			case TOOL_MOVE:
				SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_MOVE));

				if (frame)
				{
					if (mouse_held(&self->input->mouse, MOUSE_LEFT)) 
						frame->position = VEC2_IMVEC2(mousePos);

					if (key_held(&self->input->keyboard, INPUT_KEYS[INPUT_LEFT])) 
						frame->position.x -= PREVIEW_MOVE_STEP;

					if (key_held(&self->input->keyboard, INPUT_KEYS[INPUT_RIGHT])) 
						frame->position.x += PREVIEW_MOVE_STEP;

					if (key_held(&self->input->keyboard, INPUT_KEYS[INPUT_UP])) 
						frame->position.y -= PREVIEW_MOVE_STEP;

					if (key_held(&self->input->keyboard, INPUT_KEYS[INPUT_DOWN])) 
						frame->position.y += PREVIEW_MOVE_STEP;
				}
				break;
			case TOOL_ROTATE:
				SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_CROSSHAIR));
				if (frame)
				{
					if (mouse_held(&self->input->mouse, MOUSE_LEFT))
						frame->rotation += (s32)self->input->mouse.delta.x;

					if 
					(
						key_held(&self->input->keyboard, INPUT_KEYS[INPUT_LEFT]) || 
						key_held(&self->input->keyboard, INPUT_KEYS[INPUT_UP]) ||
						key_held(&self->input->keyboard, INPUT_KEYS[INPUT_ROTATE_LEFT])
					)
						frame->rotation -= PREVIEW_ROTATE_STEP;

					if 
					(
						key_held(&self->input->keyboard, INPUT_KEYS[INPUT_RIGHT]) || 
						key_held(&self->input->keyboard, INPUT_KEYS[INPUT_DOWN]) ||
						key_held(&self->input->keyboard, INPUT_KEYS[INPUT_ROTATE_RIGHT])
					)
						frame->rotation += PREVIEW_ROTATE_STEP;
				}
				break;
			case TOOL_SCALE:
				SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NE_RESIZE));
				if (mouse_held(&self->input->mouse, MOUSE_LEFT) && frame)
				{
					frame->scale.x += (s32)self->input->mouse.delta.x;
					frame->scale.y += (s32)self->input->mouse.delta.y;
				}
				break;
			case TOOL_CROP:
				SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_CROSSHAIR));
				break;
			default:
				break;
		};

		isPreviewHover = true;

		/* Used to not be annoying when at lowest zoom */
		self->settings->previewZoom = self->settings->previewZoom == EDITOR_ZOOM_MIN ? 0 : self->settings->previewZoom;

		/* Zoom in */
		if (self->input->mouse.wheelDeltaY > 0 || key_press(&self->input->keyboard, INPUT_KEYS[INPUT_ZOOM_IN]))
			self->settings->previewZoom += PREVIEW_ZOOM_STEP;

		/* Zoom out */
		if (self->input->mouse.wheelDeltaY < 0 || key_press(&self->input->keyboard, INPUT_KEYS[INPUT_ZOOM_OUT]))
			self->settings->previewZoom -= PREVIEW_ZOOM_STEP;

    	self->settings->previewZoom = CLAMP(self->settings->previewZoom, PREVIEW_ZOOM_MIN, PREVIEW_ZOOM_MAX);
	}
	else
	{
		if (isPreviewHover)
		{
			SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_DEFAULT));
			isPreviewHover = false;
		}
	}

	if (isPreviewCenter)
	{
		ImVec2 previewWindowRectSize = ImGui::GetCurrentWindow()->ClipRect.GetSize();

		/* Based on the preview's crop in its window, adjust the pan */
		self->settings->previewPanX = -(previewWindowRectSize.x - PREVIEW_SIZE.x) / 2.0f;
		self->settings->previewPanY = -((previewWindowRectSize.y - PREVIEW_SIZE.y) / 2.0f) + (IMGUI_ANIMATION_PREVIEW_SETTINGS_SIZE.y / 2.0f);

		isPreviewCenter = false;
	}

	ImGui::End();
}

/* Spritesheet Editor */
static void
_imgui_spritesheet_editor(Imgui* self)
{
	static bool isEditorHover = false;
	static bool isEditorCenter = false;
	static bool isCropDrag = false;
	static vec2 mousePos = {0, 0};
	char mousePositionString[IMGUI_POSITION_STRING_MAX];

	memset(mousePositionString, '\0', IMGUI_POSITION_STRING_MAX);

	snprintf(mousePositionString, IMGUI_POSITION_STRING_MAX, STRING_IMGUI_SPRITESHEET_EDITOR_POSITION_FORMAT, mousePos.x, mousePos.y);

	ImGui::Begin(STRING_IMGUI_SPRITESHEET_EDITOR, NULL, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
	
	/* Grid settings */
	ImGui::BeginChild(STRING_IMGUI_SPRITESHEET_EDITOR_GRID_SETTINGS, IMGUI_SPRITESHEET_EDITOR_SETTINGS_CHILD_SIZE, true);

	/* Grid toggle */
	ImGui::Checkbox(STRING_IMGUI_SPRITESHEET_EDITOR_GRID, &self->settings->editorIsGrid);
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_SPRITESHEET_EDITOR_GRID);

	ImGui::SameLine();
	
	/* Grid snap */
	ImGui::Checkbox(STRING_IMGUI_SPRITESHEET_EDITOR_GRID_SNAP, &self->settings->editorIsGridSnap);
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_SPRITESHEET_EDITOR_GRID_SNAP);

	ImGui::SameLine();

	/* Grid Color */
	ImGui::ColorEdit4(STRING_IMGUI_SPRITESHEET_EDITOR_GRID_COLOR, (f32*)&self->settings->editorGridColorR, ImGuiColorEditFlags_NoInputs);
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_SPRITESHEET_EDITOR_GRID_COLOR);
	
	/* Grid Size */
	ImGui::InputInt2(STRING_IMGUI_SPRITESHEET_EDITOR_GRID_SIZE, (s32*)&self->settings->editorGridSizeX);
	self->settings->editorGridSizeX = CLAMP(self->settings->editorGridSizeX, PREVIEW_GRID_MIN, PREVIEW_GRID_MAX);
	self->settings->editorGridSizeY = CLAMP(self->settings->editorGridSizeY, PREVIEW_GRID_MIN, PREVIEW_GRID_MAX);
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_SPRITESHEET_EDITOR_GRID_SIZE);
	
	/* Grid Offset */
	ImGui::InputInt2(STRING_IMGUI_SPRITESHEET_EDITOR_GRID_OFFSET, (s32*)&self->settings->editorGridOffsetX);
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_SPRITESHEET_EDITOR_GRID_OFFSET);

	ImGui::EndChild();
	
	ImGui::SameLine();
	
	/* View settings */
	ImGui::BeginChild(STRING_IMGUI_SPRITESHEET_EDITOR_VIEW_SETTINGS, IMGUI_SPRITESHEET_EDITOR_SETTINGS_CHILD_SIZE, true);

	/* Zoom */
	ImGui::DragFloat(STRING_IMGUI_SPRITESHEET_EDITOR_ZOOM, &self->settings->editorZoom, 1, PREVIEW_ZOOM_MIN, PREVIEW_ZOOM_MAX, "%.0f");
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_SPRITESHEET_EDITOR_ZOOM);
	
	/* Center view */
	if (ImGui::Button(STRING_IMGUI_SPRITESHEET_EDITOR_CENTER_VIEW))
		isEditorCenter = true;
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_SPRITESHEET_EDITOR_CENTER_VIEW);
	
	/* Info position */
	ImGui::Text(mousePositionString);

	ImGui::EndChild();

	ImGui::SameLine();
	
	/* Background settings */
	ImGui::BeginChild(STRING_IMGUI_SPRITESHEET_EDITOR_BACKGROUND_SETTINGS, IMGUI_SPRITESHEET_EDITOR_SETTINGS_CHILD_SIZE, true);

	/* Background color */
	ImGui::ColorEdit4(STRING_IMGUI_SPRITESHEET_EDITOR_BACKGROUND_COLOR, (f32*)&self->settings->editorBackgroundColorR, ImGuiColorEditFlags_NoInputs);
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_SPRITESHEET_EDITOR_BACKGROUND_COLOR);
	
	/* Border */
	ImGui::Checkbox(STRING_IMGUI_SPRITESHEET_EDITOR_BORDER, &self->settings->editorIsBorder);
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_SPRITESHEET_EDITOR_BORDER);

	ImGui::EndChild();
	
	vec2 editorPos = VEC2_IMVEC2(ImGui::GetCursorPos());
	ImGui::Image(self->editor->texture, IMVEC2_VEC2(EDITOR_SIZE));
	
	/* Panning + zoom */
	if (ImGui::IsItemHovered())
	{
		vec2 windowPos = VEC2_IMVEC2(ImGui::GetWindowPos());
		mousePos = VEC2_IMVEC2(ImGui::GetMousePos());

		mousePos -= (windowPos + editorPos);
		mousePos -= (EDITOR_SIZE / 2.0f);
		mousePos.x += self->settings->editorPanX;
		mousePos.y += self->settings->editorPanY;
		mousePos.x /= (self->settings->editorZoom / 100.0f);
		mousePos.y /= (self->settings->editorZoom / 100.0f);

		isEditorHover = true;

		Anm2Frame* frame = anm2_frame_from_reference(self->anm2, self->reference, *self->animationID);

		/* allow use of keybinds for tools */
		self->tool->isEnabled = true;

		if (self->reference->type != ANM2_LAYER)
			frame = NULL;

		if (self->tool->type == TOOL_CROP && frame)
		{
				SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_CROSSHAIR));

				if (mouse_press(&self->input->mouse, MOUSE_LEFT))
				{
					vec2 cropPosition = mousePos + IMGUI_SPRITESHEET_EDITOR_CROP_FORGIVENESS;

					if (self->settings->editorIsGridSnap)
					{
						cropPosition.x = (s32)(cropPosition.x / self->settings->editorGridSizeX) * self->settings->editorGridSizeX;
						cropPosition.y = (s32)(cropPosition.y / self->settings->editorGridSizeX) * self->settings->editorGridSizeY;
					}

					frame->crop = cropPosition;
					frame->size = {0, 0};
				}
				else if (mouse_held(&self->input->mouse, MOUSE_LEFT))
				{
					vec2 sizePosition = mousePos + IMGUI_SPRITESHEET_EDITOR_CROP_FORGIVENESS;

					if (self->settings->editorIsGridSnap)
					{
						sizePosition.x = (s32)(sizePosition.x / self->settings->editorGridSizeX) * self->settings->editorGridSizeX;
						sizePosition.y = (s32)(sizePosition.y / self->settings->editorGridSizeX) * self->settings->editorGridSizeY;
					}

					frame->size = sizePosition - frame->crop;
				}
		}
		else
		{
			SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_POINTER));
			
			if (mouse_held(&self->input->mouse, MOUSE_LEFT))
			{
				self->settings->editorPanX -= self->input->mouse.delta.x;
				self->settings->editorPanY -= self->input->mouse.delta.y;
			}
		}

		/* Used to not be annoying when at lowest zoom */
		self->settings->editorZoom = self->settings->editorZoom == EDITOR_ZOOM_MIN ? 0 : self->settings->editorZoom;
		
		/* Zoom in */
		if (self->input->mouse.wheelDeltaY > 0 || key_press(&self->input->keyboard, INPUT_KEYS[INPUT_ZOOM_IN]))
			self->settings->editorZoom += PREVIEW_ZOOM_STEP;

		/* Zoom out */
		if (self->input->mouse.wheelDeltaY < 0 || key_press(&self->input->keyboard, INPUT_KEYS[INPUT_ZOOM_OUT]))
			self->settings->editorZoom -= PREVIEW_ZOOM_STEP;

    	self->settings->editorZoom = CLAMP(self->settings->editorZoom, EDITOR_ZOOM_MIN, EDITOR_ZOOM_MAX);
	}
	else
	{
		if (isEditorHover)
		{
			SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_DEFAULT));
			isEditorHover = false;
		}
	}

	if (isEditorCenter)
	{
		self->settings->editorPanX = EDITOR_SIZE.x / 2.0f;
		self->settings->editorPanY = EDITOR_SIZE.y / 2.0f;

		isEditorCenter = false;
	}
	
	ImGui::End();
}

/* Frame Properties */
static void
_imgui_frame_properties(Imgui* self)
{
	ImGui::Begin(STRING_IMGUI_FRAME_PROPERTIES);
	
	Anm2Frame* frame = anm2_frame_from_reference(self->anm2, self->reference, *self->animationID);

	if (frame)
	{
		Anm2Animation* animation = anm2_animation_from_id(self->anm2, *self->animationID);

		std::vector<const char*> eventNames;
		std::vector<s32> eventIDs;
		static s32 selectedEventIndex = -1;

		switch (self->reference->type)
		{
			case ANM2_ROOT:
				ImGui::Text(STRING_IMGUI_FRAME_PROPERTIES_ROOT);
				break;
			case ANM2_LAYER:
				ImGui::Text(STRING_IMGUI_FRAME_PROPERTIES_LAYER);
				break;
			case ANM2_NULL:
				ImGui::Text(STRING_IMGUI_FRAME_PROPERTIES_NULL);
				break;
			case ANM2_TRIGGERS:
				ImGui::Text(STRING_IMGUI_FRAME_PROPERTIES_TRIGGER);
				break;
			default:
				break;
		}

		switch (self->reference->type)
		{
			case ANM2_ROOT:
			case ANM2_NULL:
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
				frame->delay = CLAMP(frame->delay, ANM2_FRAME_DELAY_MIN, animation->frameNum + 1);
				
				/* Tint */
				ImGui::ColorEdit4(STRING_IMGUI_FRAME_PROPERTIES_TINT, value_ptr(frame->tintRGBA));
				_imgui_tooltip(STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_TINT);
				
				/* Color Offset */
				ImGui::ColorEdit3(STRING_IMGUI_FRAME_PROPERTIES_COLOR_OFFSET, value_ptr(frame->offsetRGB));
				_imgui_tooltip(STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_COLOR_OFFSET);
				
				/* Visible */
				ImGui::Checkbox(STRING_IMGUI_FRAME_PROPERTIES_VISIBLE, &frame->isVisible);
				_imgui_tooltip(STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_VISIBLE);
	
				/* Flip X */
				if (ImGui::Button(STRING_IMGUI_FRAME_PROPERTIES_FLIP_X))
					frame->scale.x = -frame->scale.x;
				_imgui_tooltip(STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_FLIP_X);
					
				ImGui::SameLine();
	
				/* Flip Y */
				if (ImGui::Button(STRING_IMGUI_FRAME_PROPERTIES_FLIP_Y))
					frame->scale.y = -frame->scale.y;
				_imgui_tooltip(STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_FLIP_Y);
				
				ImGui::SameLine();
				
				/* Interpolation */
				ImGui::Checkbox(STRING_IMGUI_FRAME_PROPERTIES_INTERPOLATED, &frame->isInterpolated);
				_imgui_tooltip(STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_INTERPOLATED);
				break;
			case ANM2_LAYER:
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
				frame->delay = CLAMP(frame->delay, ANM2_FRAME_DELAY_MIN, animation->frameNum + 1);
				
				/* Tint */
				ImGui::ColorEdit4(STRING_IMGUI_FRAME_PROPERTIES_TINT, value_ptr(frame->tintRGBA));
				_imgui_tooltip(STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_TINT);
				
				/* Color Offset */
				ImGui::ColorEdit3(STRING_IMGUI_FRAME_PROPERTIES_COLOR_OFFSET, value_ptr(frame->offsetRGB));
				_imgui_tooltip(STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_COLOR_OFFSET);
	
				/* Flip X */
				if (ImGui::Button(STRING_IMGUI_FRAME_PROPERTIES_FLIP_X))
					frame->scale.x = -frame->scale.x;
				_imgui_tooltip(STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_FLIP_X);
					
				ImGui::SameLine();
	
				/* Flip Y */
				if (ImGui::Button(STRING_IMGUI_FRAME_PROPERTIES_FLIP_Y))
					frame->scale.y = -frame->scale.y;
				_imgui_tooltip(STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_FLIP_Y);
				
				/* Visible */
				ImGui::Checkbox(STRING_IMGUI_FRAME_PROPERTIES_VISIBLE, &frame->isVisible);
				_imgui_tooltip(STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_VISIBLE);
				
				ImGui::SameLine();
				
				/* Interpolation */
				ImGui::Checkbox(STRING_IMGUI_FRAME_PROPERTIES_INTERPOLATED, &frame->isInterpolated);
				_imgui_tooltip(STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_INTERPOLATED);
				break;
			case ANM2_TRIGGERS:
				/* Events drop down; pick one! */
				for (auto & [id, event] : self->anm2->events) 
				{
					eventIDs.push_back(id);
					eventNames.push_back(event.name);
					if (id == frame->eventID)
						selectedEventIndex = eventIDs.size() - 1;
				}
				
				if (ImGui::Combo(STRING_IMGUI_FRAME_PROPERTIES_EVENT, &selectedEventIndex, eventNames.data(), eventNames.size())) 
				{
    				frame->eventID = eventIDs[selectedEventIndex];
					selectedEventIndex = -1;
				}
				_imgui_tooltip(STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_EVENT);

				/* At Frame */
				ImGui::InputInt(STRING_IMGUI_FRAME_PROPERTIES_AT_FRAME, &frame->atFrame);
				_imgui_tooltip(STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_AT_FRAME);
				frame->atFrame = CLAMP(frame->atFrame, 0, animation->frameNum- 1);
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
    Anm2Reference* reference,
    s32* animationID,
    s32* spritesheetID,
    Editor* editor,
    Preview* preview,
    Settings* settings,
    Tool* tool,
    Snapshots* snapshots,
    SDL_Window* window,
    SDL_GLContext* glContext
)
{
	IMGUI_CHECKVERSION();

	self->dialog = dialog;
	self->resources = resources;
	self->input = input;
	self->anm2 = anm2;
	self->reference = reference;
	self->animationID = animationID;
	self->spritesheetID = spritesheetID;
	self->editor = editor;
	self->preview = preview;
	self->settings = settings;
	self->tool = tool;
	self->snapshots = snapshots;
	self->window = window;
	self->glContext = glContext;

	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	ImGui_ImplSDL3_InitForOpenGL(self->window, *self->glContext);
	ImGui_ImplOpenGL3_Init(STRING_OPENGL_VERSION);

	ImGuiIO& io = ImGui::GetIO();
	io.IniFilename = NULL;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigWindowsMoveFromTitleBarOnly = true;

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

	_imgui_tools(self);
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

	self->tool->isEnabled = false;

	_imgui_taskbar(self);
	_imgui_dock(self);

	/*
	if (key_press(&self->input->keyboard, INPUT_KEYS[INPUT_UNDO]))
		undo_stack_pop(self->snapshots, self->anm2);
	*/
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
