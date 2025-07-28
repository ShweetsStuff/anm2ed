// Handles everything imgui

#include "imgui.h"

template<typename T>
static void _imgui_clipboard_hovered_item_set(Imgui* self, const T& data)
{
	self->clipboard->hoveredItem = ClipboardItem(data);
}

static void _imgui_atlas_image(Imgui* self, TextureType texture)
{
	ImGui::Image(self->resources->atlas.id, VEC2_TO_IMVEC2(ATLAS_SIZES[texture]), IMVEC2_ATLAS_UV_GET(texture));
}

static void _imgui_item_text(const ImguiItem& item)
{
	ImGui::Text(item.label.c_str());
}

static void _imgui_text_string(const std::string& string)
{
	ImGui::Text(string.c_str());
}

static bool _imgui_is_window_hovered(void)
{
	return ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
}

static bool _imgui_is_no_click_on_item(void)
{
    return ImGui::IsMouseClicked(0) && !ImGui::IsAnyItemHovered() && _imgui_is_window_hovered();
}

static void _imgui_item(Imgui* self, const ImguiItem& item, bool* isActivated)
{
	if (item.is_mnemonic())
	{
		ImVec2 pos = ImGui::GetItemRectMin();
		ImFont* font = ImGui::GetFont();
		f32 fontSize = ImGui::GetFontSize();
		const char* start = item.label.c_str();
		const char* charPointer = start + item.mnemonicIndex;

		pos.x += ImGui::GetStyle().FramePadding.x;
		
		f32 offset = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, start, charPointer).x;
		f32 charWidth = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, charPointer, charPointer + 1).x;

		ImVec2 lineStart = ImVec2(pos.x + offset, pos.y + fontSize + 1.0f);
		ImVec2 lineEnd = ImVec2(lineStart.x + charWidth, lineStart.y);

		ImU32 color = ImGui::GetColorU32(ImGuiCol_Text);
		ImGui::GetWindowDrawList()->AddLine(lineStart, lineEnd, color, 1.0f);
		
		if (isActivated && ImGui::IsKeyChordPressed(ImGuiMod_Alt | item.mnemonicKey))
		{
			*isActivated = true;
			ImGui::CloseCurrentPopup();
		}
	}

	if (isActivated && self->isHotkeysEnabled && (item.is_chord() && ImGui::IsKeyChordPressed(item.chord)))
	{
		bool isFocus = !item.is_focus_window() || (imgui_nav_window_root_get() == item.focusWindow);
		
		if (isFocus)
			*isActivated = true;
	}

	if (item.is_tooltip() && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) 
		ImGui::SetTooltip(item.tooltip.c_str());

	if (isActivated && *isActivated)
	{
		if (item.isUndoable) imgui_undo_stack_push(self);
        if (item.function) item.function(self);
		
		if (item.is_popup())
		{
			ImGui::OpenPopup(item.popup.c_str());

			switch (item.popupType)
			{
				case POPUP_CENTER_SCREEN:
					ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
					break;
				case POPUP_BY_ITEM:
				default:
					ImGui::SetNextWindowPos(ImVec2(ImGui::GetItemRectMin().x, ImGui::GetItemRectMin().y + ImGui::GetItemRectSize().y));
					break;
			}
		}
	}
}

static bool _imgui_item_combo(Imgui* self, const ImguiItem& item, s32* current, const char* const items[], s32 count)
{
	bool isActivated = ImGui::Combo(item.label.c_str(), current, items, count);
	_imgui_item(self, item, &isActivated);
	return isActivated;
}

static bool _imgui_item_selectable(Imgui* self, const ImguiItem& item)
{
    const char* label = item.label.c_str();
	s32 flags = item.flags;

	if (item.isInactive || item.color.is_normal())
	{
		ImVec4 color = item.isInactive ? IMGUI_INACTIVE_COLOR : item.color.normal;
		ImGui::PushStyleColor(ImGuiCol_Text, color);
		flags |= ImGuiSelectableFlags_Disabled;
	}

	ImVec2 size = item.is_size() ? item.size : item.isSizeToText ? ImGui::CalcTextSize(label) :ImVec2(0, 0);
	bool isActivated = ImGui::Selectable(label, item.isSelected, flags, size);
	_imgui_item(self, item, &isActivated);

	if (item.isInactive || item.color.is_normal()) ImGui::PopStyleColor();

	return isActivated;
}

static bool _imgui_item_inputint(Imgui* self, const ImguiItem& item, s32* value)
{
	if (item.is_size()) ImGui::SetNextItemWidth(item.size.x);

	bool isActivated = ImGui::InputInt(item.label.c_str(), value, item.step, item.stepFast, item.flags);
	
	if (item.min != 0) *value = MIN(*value, (s32)item.min);
	if (item.max != 0) *value = MAX(*value, (s32)item.max);
		
	_imgui_item(self, item, nullptr);

    return isActivated;
}

static void _imgui_item_inputint2(Imgui* self, const ImguiItem& item, s32 value[2])
{
	if (item.is_size()) ImGui::SetNextItemWidth(item.size.x);

	ImGui::InputInt2(item.label.c_str(), value);
	
	if (item.min > 0)
	{
		value[0] = MIN(value[0], (s32)item.min);
		value[1] = MIN(value[1], (s32)item.min);
	}

	if (item.max > 0)
	{
		value[0] = MAX(value[0], (s32)item.max);
		value[1] = MAX(value[1], (s32)item.max);
	}
		
	_imgui_item(self, item, nullptr);
}

static bool _imgui_item_inputtext(Imgui* self, const ImguiItem& item, std::string* buffer)
{
	if ((s32)buffer->size() < item.max) buffer->resize(item.max); 

	if (item.is_size()) 
		ImGui::SetNextItemWidth(item.size.x);
	else
		ImGui::SetNextItemWidth(ImGui::CalcTextSize(buffer->c_str()).x);

	bool isActivated = ImGui::InputText(item.label.c_str(), &(*buffer)[0], item.max, item.flags);
	_imgui_item(self, item, nullptr);

    return isActivated;
}

static void _imgui_item_checkbox(Imgui* self, const ImguiItem& item, bool* value)
{
	ImGui::Checkbox(item.label.c_str(), value);

	if (item.is_mnemonic() && ImGui::IsKeyChordPressed(ImGuiMod_Alt | item.mnemonicKey))
	{
		*value = !*value;
		ImGui::CloseCurrentPopup();
	}

	_imgui_item(self, item, nullptr);
}

static bool _imgui_item_radio_button(Imgui* self, const ImguiItem& item, s32* value)
{
	if (item.is_size()) ImGui::SetNextItemWidth(item.size.x);
	
	bool isActivated = ImGui::RadioButton(item.label.c_str(), value, item.value);
	_imgui_item(self, item, &isActivated);
	return isActivated;
}

static void _imgui_item_dragfloat(Imgui* self, const ImguiItem& item, f32* value)
{
	ImGui::DragFloat(item.label.c_str(), value, item.speed, item.min, item.max, item.format.c_str());
	_imgui_item(self, item, nullptr);
}

static void _imgui_item_coloredit3(Imgui* self, const ImguiItem& item, f32 value[3])
{
	ImGui::ColorEdit3(item.label.c_str(), value, item.flags);
	_imgui_item(self, item, nullptr);
}

static void _imgui_item_coloredit4(Imgui* self, const ImguiItem& item, f32 value[4])
{
	ImGui::ColorEdit4(item.label.c_str(), value, item.flags);
	_imgui_item(self, item, nullptr);
}

static void _imgui_item_dragfloat2(Imgui* self, const ImguiItem& item, f32 value[2])
{
	ImGui::DragFloat2(item.label.c_str(), value, item.speed, item.min, item.max, item.format.c_str());
	_imgui_item(self, item, nullptr);
}

static bool _imgui_item_button(Imgui* self, const ImguiItem& item)
{
	bool isActivated = ImGui::Button(item.label.c_str(), item.size);
	_imgui_item(self, item, &isActivated);
	return isActivated;
}

static bool _imgui_item_selectable_inputtext(Imgui* self, const ImguiItem& item, std::string* string, s32 id)
{
    static s32 renameID = ID_NONE;
    static s32 itemID = ID_NONE;
    const char* label = item.label.c_str();
	bool isActivated = false;
	static std::string buffer{};
    
	ImVec2 size = item.is_size() ? item.size : item.isSizeToText ? ImGui::CalcTextSize(label) :ImVec2(0, 0);

    if (renameID == id && itemID == item.id)
    {
        ImGui::PushID(id);

		ImguiItem itemRenamable = IMGUI_RENAMABLE;
		itemRenamable.size = size;
		self->isRename = true;

		isActivated = _imgui_item_inputtext(self, itemRenamable, &buffer);

		if (isActivated || _imgui_is_no_click_on_item())
        {
			*string = buffer;
            renameID = ID_NONE;
            itemID = ID_NONE;
			self->isRename = false;
        }

        ImGui::PopID();
    }
    else
    {
    	isActivated = ImGui::Selectable(label, item.isSelected, 0, size);
		
		if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
        {
			buffer = *string;
            renameID = id;
            itemID = item.id;
            ImGui::SetKeyboardFocusHere(-1);
        }
    }

	_imgui_item(self, item, &isActivated);

	return isActivated;
}

static bool _imgui_item_selectable_inputint(Imgui* self, const ImguiItem& item, s32* value, s32 id)
{
    static s32 itemID = ID_NONE;
    static s32 changeID = ID_NONE;
    const char* label = item.label.c_str();
	bool isActivated = false;
	ImVec2 size = item.is_size() ? item.size : item.isSizeToText ? ImGui::CalcTextSize(label) :ImVec2(0, 0);

    if (changeID == id && itemID == item.id)
    {
        ImGui::PushID(id);

		ImguiItem itemChangeable = IMGUI_CHANGEABLE;
		itemChangeable.size = size;
		self->isChangeValue = true;

		if (_imgui_item_inputint(self, itemChangeable, value))
        {
            itemID = ID_NONE;
			changeID = ID_NONE;
			self->isChangeValue = false;
        }

		if (_imgui_is_no_click_on_item())
        {
            itemID = ID_NONE;
			changeID = ID_NONE;
			self->isChangeValue = false;
        }

        ImGui::PopID();
    }
    else
    {
        isActivated = ImGui::Selectable(label, item.isSelected, 0, size);
        
		if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
        {
            itemID = item.id;
            changeID = id;
            ImGui::SetKeyboardFocusHere(-1);
        }
    }

	_imgui_item(self, item, &isActivated);

	return isActivated;
}

static void _imgui_item_atlas_image_text(Imgui* self, const ImguiItem& item)
{
	_imgui_atlas_image(self, item.texture);
	ImGui::SameLine();
	ImGui::Text(item.label.c_str());
}

static bool _imgui_item_atlas_image_selectable(Imgui* self, const ImguiItem& item)
{
	_imgui_atlas_image(self, item.texture);
	ImGui::SameLine();
	return _imgui_item_selectable(self, item);
}


static bool _imgui_item_atlas_image_selectable_inputtext(Imgui* self, ImguiItem& item, std::string* string, s32 id)
{
	_imgui_atlas_image(self, item.texture);
	ImGui::SameLine();
	return _imgui_item_selectable_inputtext(self, item, string, id);
}

static bool _imgui_item_atlas_image_selectable_inputint(Imgui* self, ImguiItem& item, s32* value, s32 id)
{
	_imgui_atlas_image(self, item.texture);
	ImGui::SameLine();
	return _imgui_item_selectable_inputint(self, item, value, id);
}

static bool _imgui_item_atlas_image_button(Imgui* self, const ImguiItem& item)
{
	bool isActivated = false;
	ImVec2 imageSize = VEC2_TO_IMVEC2(ATLAS_SIZES[item.texture]);
	ImVec2 buttonSize = item.is_size() ? item.size : imageSize;

    if (item.color.is_normal()) ImGui::PushStyleColor(ImGuiCol_Button, item.color.normal);
    if (item.color.is_active()) ImGui::PushStyleColor(ImGuiCol_ButtonActive, item.color.active);
    if (item.color.is_hovered()) ImGui::PushStyleColor(ImGuiCol_ButtonHovered, item.color.hovered);
    if (item.isSelected) ImGui::PushStyleColor(ImGuiCol_Button, item.color.active);
    if (item.is_border()) ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, item.border);
    
	if (item.is_size())
	{
		isActivated = ImGui::Button(item.label.c_str(), buttonSize);
		
		ImVec2 pos = ImGui::GetItemRectMin();
		ImVec2 imageMin = pos + item.contentOffset;
		ImVec2 imageMax = imageMin + imageSize;

		ImGui::GetWindowDrawList()->AddImage(self->resources->atlas.id, imageMin, imageMax, IMVEC2_ATLAS_UV_GET(item.texture), IM_COL32_WHITE);
	}
	else
		isActivated = ImGui::ImageButton(item.label.c_str(), self->resources->atlas.id, buttonSize, IMVEC2_ATLAS_UV_GET(item.texture));
	_imgui_item(self, item, &isActivated);
	
    if (item.color.is_normal()) ImGui::PopStyleColor();
    if (item.color.is_active()) ImGui::PopStyleColor();
    if (item.color.is_hovered()) ImGui::PopStyleColor();
    if (item.isSelected) ImGui::PopStyleColor();
    if (item.is_border()) ImGui::PopStyleVar();

    return isActivated;
}

static void _imgui_item_begin(const ImguiItem& item)
{
    if (item.is_size()) ImGui::SetNextWindowSize(item.size);
	ImGui::Begin(item.label.c_str(), nullptr, item.flags);
}

static void _imgui_item_end(void)
{
	ImGui::End();
}

static void _imgui_item_begin_child(const ImguiItem& item)
{
    if (item.color.is_normal()) ImGui::PushStyleColor(ImGuiCol_ChildBg, item.color.normal);
	ImGui::BeginChild(item.label.c_str(), item.size, item.flags, item.flagsAlt);
    if (item.color.is_normal()) ImGui::PopStyleColor();
}

static void _imgui_item_end_child(void)
{
	ImGui::EndChild();
}

static void _imgui_item_dockspace(const ImguiItem& item)
{
    ImGui::DockSpace(ImGui::GetID(item.label.c_str()), item.size, item.flags);
}

static void _imgui_keyboard_navigation_set(bool value)
{
	if (value) ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	if (!value) ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;
}

static bool _imgui_item_yes_no_popup(Imgui* self, const ImguiItem& item)
{
	ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	
	if (ImGui::BeginPopupModal(item.popup.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text(item.label.c_str());
		ImGui::Separator();

		if (_imgui_item_button(self, IMGUI_POPUP_YES_BUTTON))
		{
			ImGui::CloseCurrentPopup();
			ImGui::EndPopup();
			return true;
		}

		ImGui::SameLine();
		
		if (_imgui_item_button(self, IMGUI_POPUP_NO_BUTTON))
			ImGui::CloseCurrentPopup();

		ImGui::EndPopup();
	}

	return false;
}

static void _imgui_spritesheet_editor_set(Imgui* self, s32 id)
{
	if (self->anm2->spritesheets.contains(id)) self->editor->spritesheetID = id;
}

static void _imgui_timeline(Imgui* self)
{
	static const ImU32 frameColor = ImGui::GetColorU32(IMGUI_TIMELINE_FRAME_COLOR);
	static const ImU32 frameMultipleColor = ImGui::GetColorU32(IMGUI_TIMELINE_FRAME_MULTIPLE_COLOR);
	static const ImU32 headerFrameColor = ImGui::GetColorU32(IMGUI_TIMELINE_HEADER_FRAME_COLOR);
	static const ImU32 headerFrameMultipleColor = ImGui::GetColorU32(IMGUI_TIMELINE_HEADER_FRAME_MULTIPLE_COLOR);
	static const ImU32 textColor = ImGui::GetColorU32(ImGuiCol_Text);
	static ImVec2 scroll{};
	static ImVec2 pickerPos{};
	static Anm2Reference swapItemReference;
	static bool isItemSwap = false; 
	static ImVec2 itemMin{};
	static ImVec2 mousePos{};
	static ImVec2 localMousePos{};
	static s32 frameIndex = INDEX_NONE;
	static Anm2Reference hoverReference;

	static const ImVec2& frameSize = IMGUI_TIMELINE_FRAME_SIZE;
	
	_imgui_item_begin(IMGUI_TIMELINE);
	
	Anm2Animation* animation = anm2_animation_from_reference(self->anm2, self->reference);

	if (!animation)
	{
		ImGui::Text(IMGUI_TIMELINE_NO_ANIMATION);
		_imgui_item_end(); // IMGUI_TIMELINE
		return;
	}

	ImVec2 defaultItemSpacing = ImGui::GetStyle().ItemSpacing;
	ImVec2 defaultWindowPadding = ImGui::GetStyle().WindowPadding;
	ImVec2 defaultFramePadding = ImGui::GetStyle().FramePadding;

	ImguiItem timelineChild = IMGUI_TIMELINE_CHILD;
	timelineChild.size.y = ImGui::GetContentRegionAvail().y - IMGUI_TIMELINE_FOOTER_HEIGHT;

	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

	_imgui_item_begin_child(timelineChild);
	ImVec2 clipRectMin = ImGui::GetWindowDrawList()->GetClipRectMin();
	ImVec2 clipRectMax = ImGui::GetWindowDrawList()->GetClipRectMax();
	clipRectMin.x += IMGUI_TIMELINE_ITEM_SIZE.x;
	
	ImVec2 scrollDelta = {0, 0};
	
	if (_imgui_is_window_hovered())
	{
		ImGuiIO& io = ImGui::GetIO();
		f32 lineHeight = ImGui::GetTextLineHeight();

		scrollDelta.x -= io.MouseWheelH * lineHeight;
		scrollDelta.y -= io.MouseWheel * lineHeight * 3.0f;
	}

	std::function<void()> timeline_header = [&]()
	{
		static bool isHeaderClicked = false;
		_imgui_item_begin_child(IMGUI_TIMELINE_HEADER);

		ImGui::SetScrollX(scroll.x);
		
		itemMin = ImGui::GetItemRectMin();
		mousePos = ImGui::GetMousePos();
		localMousePos = ImVec2(mousePos.x - itemMin.x + scroll.x, mousePos.y - itemMin.y);
		frameIndex = CLAMP((s32)(localMousePos.x / frameSize.x), 0, (f32)(animation->frameNum - 1));

		if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && _imgui_is_window_hovered())
			isHeaderClicked = true;

		if (isHeaderClicked)
		{
			self->preview->time = frameIndex;

			if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) 
				isHeaderClicked = false;
		}

		ImVec2 cursorPos = ImGui::GetCursorScreenPos();
		pickerPos = {cursorPos.x + (self->preview->time * frameSize.x), cursorPos.y};
		
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		
		for (s32 i = 0; i < animation->frameNum; i++)
		{
			bool isMultiple = i % IMGUI_TIMELINE_FRAME_MULTIPLE == 0;
		
			ImVec2 framePos = ImGui::GetCursorScreenPos();
			ImVec2 bgMin = framePos;
			ImVec2 bgMax = {framePos.x + frameSize.x, framePos.y + frameSize.y};
			ImU32 bgColor = isMultiple ? headerFrameMultipleColor : headerFrameColor;

			drawList->AddRectFilled(bgMin, bgMax, bgColor);
			
			if (i % IMGUI_TIMELINE_FRAME_MULTIPLE == 0)
			{
				std::string frameIndexString = std::to_string(i);
				ImVec2 textSize = ImGui::CalcTextSize(frameIndexString.c_str());
				ImVec2 textPos = {framePos.x + (frameSize.x - textSize.x) * 0.5f, framePos.y + (frameSize.y - textSize.y) * 0.5f};
				drawList->AddText(textPos, textColor, frameIndexString.c_str());
			}
		
			_imgui_atlas_image(self, TEXTURE_FRAME);
			
			if (i < animation->frameNum - 1)
				ImGui::SameLine();
		}

		ImVec2& pos = pickerPos;
		
		ImDrawList* foregroundDrawList = ImGui::GetForegroundDrawList();
		
		ImVec2 lineStart = {pos.x + (frameSize.x * 0.5f) - (IMGUI_TIMELINE_PICKER_LINE_WIDTH * 0.5f), pos.y + frameSize.y};
		ImVec2 lineEnd = {lineStart.x + IMGUI_TIMELINE_PICKER_LINE_WIDTH, lineStart.y + timelineChild.size.y - frameSize.y};
		
		foregroundDrawList->PushClipRect(clipRectMin, clipRectMax, true);

		foregroundDrawList->AddImage(self->resources->atlas.id, pos, ImVec2(pos.x + frameSize.x, pos.y + frameSize.y), IMVEC2_ATLAS_UV_GET(TEXTURE_PICKER));
		foregroundDrawList->AddRectFilled(lineStart, lineEnd, IMGUI_PICKER_LINE_COLOR);

		foregroundDrawList->PopClipRect();
				
		_imgui_item_end_child(); // IMGUI_TIMELINE_HEADER
	};

	std::function<void(Anm2Reference, s32*)> timeline_item_child = [&](Anm2Reference reference, s32* index)
	{
		Anm2Item* item = anm2_item_from_reference(self->anm2, &reference);

		if (!item) return;

		ImVec2 buttonSize = VEC2_TO_IMVEC2(TEXTURE_SIZE) + (defaultFramePadding * ImVec2(2, 2));
		
		Anm2Type& type = reference.itemType;
		Anm2Layer* layer = nullptr;
		Anm2Null* null = nullptr;
		s32 buttonCount = type == ANM2_NULL ? 2 : 1;
		f32 buttonAreaWidth = buttonCount * buttonSize.x + (buttonCount - 1) * defaultItemSpacing.x;

		ImguiItem imguiItem = *IMGUI_TIMELINE_ITEMS[type];
		ImguiItem imguiItemSelectable = *IMGUI_TIMELINE_ITEM_SELECTABLES[type];
		imguiItemSelectable.isSelected = self->reference->itemID == reference.itemID && self->reference->itemType == type;
		
		ImGui::PushID(reference.itemID);
				
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, defaultItemSpacing);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, defaultWindowPadding);
			
		ImVec2 childPos = ImGui::GetCursorScreenPos();
		_imgui_item_begin_child(imguiItem);
		ImVec2 childSize = ImGui::GetContentRegionAvail();

		switch (type)
		{
			case ANM2_ROOT:
			case ANM2_TRIGGERS:
				if (_imgui_item_atlas_image_selectable(self, imguiItemSelectable))
					*self->reference = reference;
				break;
			case ANM2_LAYER:
				layer = &self->anm2->layers[reference.itemID];
				imguiItemSelectable.label = std::format(IMGUI_TIMELINE_CHILD_ID_LABEL, reference.itemID, layer->name);
				if (_imgui_item_atlas_image_selectable_inputtext(self, imguiItemSelectable, &layer->name, *index))
					*self->reference = reference;
				break;
			case ANM2_NULL:
				null = &self->anm2->nulls[reference.itemID];
				imguiItemSelectable.label = std::format(IMGUI_TIMELINE_CHILD_ID_LABEL, reference.itemID, null->name);
				if (_imgui_item_atlas_image_selectable_inputtext(self, imguiItemSelectable, &null->name, *index))
					*self->reference = reference;
				break;
			default:
				break;
		}

		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
		{
			*self->reference = reference;
			
			ImGui::SetDragDropPayload(imguiItemSelectable.dragDrop.c_str(), &reference, sizeof(Anm2Reference));
			timeline_item_child(reference, index);
			ImGui::EndDragDropSource();
		}

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(imguiItemSelectable.dragDrop.c_str()))
			{
				Anm2Reference checkReference = *(Anm2Reference*)payload->Data;
				if (checkReference != reference)
				{
					swapItemReference = reference;
					isItemSwap = true;
				}
			}

			ImGui::EndDragDropTarget();
		}

		if (type == ANM2_LAYER)
		{
			ImguiItem spritesheetIDItem = IMGUI_TIMELINE_SPRITESHEET_ID;
			spritesheetIDItem.label = std::format(IMGUI_TIMELINE_SPRITESHEET_ID_FORMAT, layer->spritesheetID);
			ImGui::SameLine();
			_imgui_item_atlas_image_selectable_inputint(self, spritesheetIDItem, &layer->spritesheetID, *index);
		}

		ImGui::SetCursorScreenPos({childPos.x + childSize.x - buttonAreaWidth, childPos.y + defaultWindowPadding.y});

		if (type == ANM2_NULL)
		{
			ImguiItem rectItem = null->isShowRect ? IMGUI_TIMELINE_ITEM_SHOW_RECT : IMGUI_TIMELINE_ITEM_HIDE_RECT;
			if (_imgui_item_atlas_image_button(self, rectItem))
				null->isShowRect = !null->isShowRect;

			ImGui::SameLine(0.0f, defaultItemSpacing.x);
		}
		
		ImguiItem visibleItem = item->isVisible ? IMGUI_TIMELINE_ITEM_VISIBLE : IMGUI_TIMELINE_ITEM_INVISIBLE;
		if (_imgui_item_atlas_image_button(self, visibleItem))
			item->isVisible = !item->isVisible;

		ImGui::PopStyleVar(2);
		
		_imgui_item_end_child(); // imguiItem
		
		ImGui::PopID();

		(*index)++;
	};

	std::function<void()> timeline_items_child = [&]()
	{
		s32 index = 0;

		s32& animationID = self->reference->animationID;

		_imgui_item_begin_child(IMGUI_TIMELINE_ITEMS_CHILD);
		ImGui::SetScrollY(scroll.y);

		timeline_item_child({animationID, ANM2_ROOT, ID_NONE, INDEX_NONE}, &index);

		for (auto it = self->anm2->layerMap.rbegin(); it != self->anm2->layerMap.rend(); it++)
			timeline_item_child({animationID, ANM2_LAYER, it->second, INDEX_NONE}, &index);

		for (auto & [id, null] : animation->nullAnimations)
			timeline_item_child({animationID, ANM2_NULL, id, INDEX_NONE}, &index);

		timeline_item_child({animationID, ANM2_TRIGGERS, ID_NONE, INDEX_NONE}, &index);

		_imgui_item_end_child(); // IMGUI_TIMELINE_ITEMS_CHILD
	};

	std::function<void(Anm2Reference, s32* index)> timeline_item_frames = [&](Anm2Reference reference, s32* index)
	{
		ImDrawList* drawList = ImGui::GetWindowDrawList();

		Anm2Item* item = anm2_item_from_reference(self->anm2, &reference);
		Anm2Type& type = reference.itemType;

		ImGui::PushID(*index);
		
		ImguiItem itemFramesChild = IMGUI_TIMELINE_ITEM_FRAMES_CHILD;
		itemFramesChild.size.x = frameSize.x * animation->frameNum;

		_imgui_item_begin_child(itemFramesChild);

		if (_imgui_is_window_hovered())
		{
			hoverReference = reference;
			hoverReference.frameIndex = frameIndex;
			self->clipboard->location = hoverReference;
		}

		ImVec2 startPos = ImGui::GetCursorPos();

		for (s32 i = 0; i < animation->frameNum; i++)
		{
			bool isMultiple = i % IMGUI_TIMELINE_FRAME_MULTIPLE == 0;
		
			ImVec2 framePos = ImGui::GetCursorScreenPos();
			ImVec2 bgMin = framePos;
			ImVec2 bgMax = {framePos.x + frameSize.x, framePos.y + frameSize.y};
			
			ImU32 bgColor = isMultiple ? frameMultipleColor : frameColor;

			drawList->AddRectFilled(bgMin, bgMax, bgColor);
			
			_imgui_atlas_image(self, TEXTURE_FRAME_ALT);

			if (i < animation->frameNum - 1)
				ImGui::SameLine();
		}

		ImGui::SetCursorPos(startPos);

		std::function<void(s32, Anm2Frame&)> timeline_item_frame = [&](s32 i, Anm2Frame& frame)
		{
			ImGui::PushID(i);
			reference.frameIndex = i;
			ImguiItem frameButton = *IMGUI_TIMELINE_FRAMES[type];
			ImVec2 framePos = ImGui::GetCursorPos();
			frameButton.texture = frame.isInterpolated ? TEXTURE_INTERPOLATED : TEXTURE_UNINTERPOLATED;
			frameButton.size = {frameSize.x * frame.delay, frameSize.y};
			frameButton.isSelected = reference == *self->reference;

			if (type == ANM2_TRIGGERS)
			{
				framePos.x = startPos.x + (frameSize.x * frame.atFrame);
				frameButton.texture = TEXTURE_TRIGGER;
			}

			ImGui::SetCursorPos(framePos);
			
			if (_imgui_item_atlas_image_button(self, frameButton))
			{
				*self->reference = reference;

					_imgui_spritesheet_editor_set(self, self->anm2->layers[self->reference->itemID].spritesheetID);
			}

			if (ImGui::IsItemHovered())
			{
				Anm2FrameWithReference frameWithReference = {reference, frame};
				_imgui_clipboard_hovered_item_set(self, frameWithReference);
			}

			if (type == ANM2_TRIGGERS)
			{
				if (ImGui::IsItemActivated()) 
					imgui_undo_stack_push(self);

				if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceNoPreviewTooltip))
				{
					frame.atFrame = frameIndex;
					ImGui::EndDragDropSource();
				}
			}
			else
			{
				if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
				{
					ImGui::SetDragDropPayload(frameButton.dragDrop.c_str(), &reference, sizeof(Anm2Reference));
					timeline_item_frame(i, frame);
					ImGui::EndDragDropSource();
				}
			}

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(frameButton.dragDrop.c_str()))
				{
					Anm2Reference swapReference = *(Anm2Reference*)payload->Data;
					if (swapReference != reference)
					{
						imgui_undo_stack_push(self);

						Anm2Frame* swapFrame = anm2_frame_from_reference(self->anm2, &reference);
						Anm2Frame* dragFrame = anm2_frame_from_reference(self->anm2, &swapReference);
						
						if (swapFrame && dragFrame)
						{
							Anm2Frame oldFrame = *swapFrame;

							*swapFrame = *dragFrame;
							*dragFrame = oldFrame;

							*self->reference = swapReference;
						}
					}
				}
				ImGui::EndDragDropTarget();
			}
					
			if (i < (s32)item->frames.size() - 1)
				ImGui::SameLine();

			if (_imgui_is_no_click_on_item())
				anm2_reference_frame_clear(self->reference);

			ImGui::PopID();
		};

		for (auto [i, frame] : std::views::enumerate(item->frames))
			timeline_item_frame(i, frame);

		_imgui_item_end_child(); // itemFramesChild
		
		ImGui::PopID();
		
		(*index)++;
	};

	std::function<void()> timeline_frames_child = [&]()
	{
		s32& animationID = self->reference->animationID;
		s32 index = 0;

		_imgui_item_begin_child(IMGUI_TIMELINE_FRAMES_CHILD);
		scroll.x = ImGui::GetScrollX() + scrollDelta.x;
		scroll.y = ImGui::GetScrollY() + scrollDelta.y;
		ImGui::SetScrollX(scroll.x);
		ImGui::SetScrollY(scroll.y);

		timeline_item_frames({animationID, ANM2_ROOT, ID_NONE, INDEX_NONE}, &index);

		for (auto it = self->anm2->layerMap.rbegin(); it != self->anm2->layerMap.rend(); it++)
			timeline_item_frames({animationID, ANM2_LAYER, it->second, INDEX_NONE}, &index);

		for (auto & [id, null] : animation->nullAnimations)
			timeline_item_frames({animationID, ANM2_NULL, id, INDEX_NONE}, &index);

		timeline_item_frames({animationID, ANM2_TRIGGERS, ID_NONE, INDEX_NONE}, &index);

		_imgui_item_end_child(); // IMGUI_TIMELINE_FRAMES_CHILD
	};

	// In order to set scroll properly, timeline_frames_child must be called first
	ImGui::SetCursorPos(ImVec2(IMGUI_TIMELINE_ITEM.size));
	timeline_frames_child();
	ImGui::SetCursorPos(ImVec2(0, 0));	
	
	_imgui_item_begin_child(IMGUI_TIMELINE_ITEM);
	_imgui_item_end_child(); // IMGUI_TIMELINE_ITEM
	ImGui::SameLine();

	timeline_header();
	timeline_items_child();
	
	ImGui::PopStyleVar(2);
	
	_imgui_item_end_child(); // IMGUI_TIMELINE_CHILD
	
	if (isItemSwap)
	{
		Anm2Animation* animation = anm2_animation_from_reference(self->anm2, self->reference);

		switch (swapItemReference.itemType)
		{
			case ANM2_LAYER:
				map_swap(self->anm2->layers, self->reference->itemID, swapItemReference.itemID);
				map_swap(animation->layerAnimations, self->reference->itemID, swapItemReference.itemID);
				break;
			case ANM2_NULL:
				map_swap(self->anm2->nulls, self->reference->itemID, swapItemReference.itemID);
				map_swap(animation->nullAnimations, self->reference->itemID, swapItemReference.itemID);
				break;
			default:
				break;
		}

		self->reference->itemID = swapItemReference.itemID;
		anm2_reference_clear(&swapItemReference);
		isItemSwap = false;
	}

	if(_imgui_item_button(self, IMGUI_TIMELINE_ADD_ELEMENT))
		ImGui::OpenPopup(IMGUI_TIMELINE_ADD_ELEMENT_POPUP);
	
	if (ImGui::BeginPopup(IMGUI_TIMELINE_ADD_ELEMENT_POPUP))
	{
		if (_imgui_item_selectable(self, IMGUI_TIMELINE_ADD_ELEMENT_LAYER))
			anm2_layer_add(self->anm2);

		if (_imgui_item_selectable(self, IMGUI_TIMELINE_ADD_ELEMENT_NULL))
			anm2_null_add(self->anm2);

		ImGui::EndPopup();
	}

	ImGui::SameLine();

	if (_imgui_item_button(self, IMGUI_TIMELINE_REMOVE_ELEMENT))
	{
		switch (self->reference->itemType)
		{
			case ANM2_LAYER:
				anm2_layer_remove(self->anm2, self->reference->itemID);
				break;
			case ANM2_NULL:
				anm2_null_remove(self->anm2, self->reference->itemID);
				break;
			default:
				break;
		}

		anm2_reference_item_clear(self->reference);
	}
	
	ImGui::SameLine();

	ImguiItem playPauseItem = self->preview->isPlaying ? IMGUI_TIMELINE_PAUSE : IMGUI_TIMELINE_PLAY;
	if (_imgui_item_button(self, playPauseItem))
		self->preview->isPlaying = !self->preview->isPlaying;

	ImGui::SameLine();
	
	if (_imgui_item_button(self, IMGUI_TIMELINE_ADD_FRAME))
		anm2_frame_add(self->anm2, self->reference, (s32)self->preview->time);

	ImGui::SameLine();

	if(_imgui_item_button(self, IMGUI_TIMELINE_REMOVE_FRAME))
	{
		imgui_undo_stack_push(self);
		if (anm2_frame_from_reference(self->anm2, self->reference))
		{
			Anm2Item* item = anm2_item_from_reference(self->anm2, self->reference);
			item->frames.erase(item->frames.begin() + self->reference->frameIndex);
			anm2_reference_frame_clear(self->reference);
		}
	}

	ImGui::SameLine();

	if (_imgui_item_button(self, IMGUI_TIMELINE_FIT_ANIMATION_LENGTH))
		anm2_animation_length_set(animation);

	ImGui::SameLine();
	_imgui_item_inputint(self, IMGUI_TIMELINE_ANIMATION_LENGTH, &animation->frameNum);
	ImGui::SameLine();
	_imgui_item_inputint(self, IMGUI_TIMELINE_FPS, &self->anm2->fps);
	ImGui::SameLine();
	_imgui_item_checkbox(self, IMGUI_TIMELINE_LOOP, &animation->isLoop);
	ImGui::SameLine();
	_imgui_item_inputtext(self, IMGUI_TIMELINE_CREATED_BY, &self->anm2->createdBy);
	ImGui::SameLine();
	_imgui_text_string(IMGUI_TIMELINE_CREATED_ON.label + self->anm2->createdOn);
	ImGui::SameLine();
	_imgui_text_string(IMGUI_TIMELINE_VERSION.label + std::to_string(self->anm2->version));

	if (_imgui_is_no_click_on_item())
    	anm2_reference_item_clear(self->reference);
	
	_imgui_item_end(); // IMGUI_TIMELINE

	self->preview->time = CLAMP(self->preview->time, 0.0f, animation->frameNum - 1);
}

static void _imgui_taskbar(Imgui* self)
{
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImguiItem taskbarItem = IMGUI_TASKBAR;
	taskbarItem.size = {viewport->Size.x, IMGUI_TASKBAR.size.y};
	ImGui::SetNextWindowPos(viewport->Pos);
	_imgui_item_begin(taskbarItem);

	_imgui_item_selectable(self, IMGUI_TASKBAR_FILE);

	if (ImGui::BeginPopup(IMGUI_FILE_POPUP))
	{
		_imgui_item_selectable(self, IMGUI_FILE_NEW);
		_imgui_item_selectable(self, IMGUI_FILE_OPEN);
		_imgui_item_selectable(self, IMGUI_FILE_SAVE);
		_imgui_item_selectable(self, IMGUI_FILE_SAVE_AS);
		ImGui::EndPopup();
	}

	ImGui::SameLine();

	_imgui_item_selectable(self, IMGUI_TASKBAR_PLAYBACK);
		
	if (ImGui::BeginPopup(IMGUI_PLAYBACK_POPUP))
	{
		_imgui_item_checkbox(self, IMGUI_PLAYBACK_ALWAYS_LOOP, &self->settings->playbackIsLoop);
		ImGui::EndPopup();
	}

	ImGui::SameLine();

	_imgui_item_selectable(self, IMGUI_TASKBAR_WIZARD);
	
	if (ImGui::BeginPopup(IMGUI_WIZARD_POPUP))
	{
		_imgui_item_selectable(self, IMGUI_WIZARD_RECORD_GIF_ANIMATION);
		ImGui::EndPopup();
	}

	_imgui_item_end();
}

static void _imgui_tools(Imgui* self)
{
	_imgui_item_begin(IMGUI_TOOLS);

	f32 availableWidth = ImGui::GetContentRegionAvail().x;
	f32 usedWidth = ImGui::GetStyle().FramePadding.x;

	for (s32 i = 0; i < TOOL_COUNT; i++)
	{
		const ImguiItem item = *IMGUI_TOOL_ITEMS[i];

		if (i > 0 && usedWidth < availableWidth)
			ImGui::SameLine();
		else
			usedWidth = 0;

		if (i != TOOL_COLOR)
		{
			ImVec4 buttonColor = self->tool == (ToolType)i ? ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered] : ImGui::GetStyle().Colors[ImGuiCol_Button];
			ImGui::PushStyleColor(ImGuiCol_Button, buttonColor);

			_imgui_item_atlas_image_button(self, item);

			ImGui::PopStyleColor();
		}
		else
			_imgui_item_coloredit4(self, IMGUI_TOOL_COLOR, &self->settings->toolColorR);
		
		usedWidth += ImGui::GetItemRectSize().x + ImGui::GetStyle().ItemSpacing.x;
	}

	_imgui_item_end(); // IMGUI_TOOLS
}

static void _imgui_animations(Imgui* self)
{
	_imgui_item_begin(IMGUI_ANIMATIONS);
	ImVec2 windowSize = ImGui::GetContentRegionAvail();

	std::function<void(s32, Anm2Animation&)> animation_item = [&](s32 id, Anm2Animation& animation)
	{
		ImGui::PushID(id);
		
		ImguiItem animationItem = IMGUI_ANIMATION;
		animationItem.isSelected =  self->reference->animationID == id; 
		animationItem.size.x = windowSize.x;

		if (animation.name == self->anm2->defaultAnimation)
			animationItem.label = std::format(IMGUI_ANIMATION_DEFAULT_FORMAT, animation.name);
		else
			animationItem.label = animation.name;

		if (_imgui_item_atlas_image_selectable_inputtext(self, animationItem, &animation.name, id))
		{
			self->reference->animationID = id;
			anm2_reference_item_clear(self->reference);
			self->preview->isPlaying = false;
			self->preview->time = 0.0f;
		}
		
		if (ImGui::IsItemHovered())
		{
			Anm2AnimationWithID animationWithID = {id, animation};
			_imgui_clipboard_hovered_item_set(self, animationWithID);
			self->clipboard->location = (s32)id;
		}

		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
		{
			ImGui::SetDragDropPayload(animationItem.dragDrop.c_str(), &id, sizeof(s32));
			animation_item(id, animation);
			ImGui::EndDragDropSource();	
		}

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(animationItem.dragDrop.c_str()))
			{
				s32 sourceID = *(s32*)payload->Data;
				if (sourceID != id)
				{
					imgui_undo_stack_push(self);
					map_swap(self->anm2->animations, sourceID, id);
				}
			}

			ImGui::EndDragDropTarget();
		}
		
		ImGui::PopID();
	};

	for (auto & [id, animation] : self->anm2->animations)
		animation_item(id, animation);
	
	Anm2Animation* animation = anm2_animation_from_reference(self->anm2, self->reference);
		
	if (_imgui_item_button(self, IMGUI_ANIMATION_ADD))
	{
		bool isDefault = (s32)self->anm2->animations.size() == 0; 
		s32 id = anm2_animation_add(self->anm2);
		
		self->reference->animationID = id;

		if (isDefault)
			self->anm2->defaultAnimation = self->anm2->animations[id].name;
	}
		
	ImGui::SameLine();

	if (_imgui_item_button(self, IMGUI_ANIMATION_DUPLICATE) && animation)
	{
		s32 id = map_next_id_get(self->anm2->animations);
		self->anm2->animations.insert({id, *animation});
		self->reference->animationID = id;
	}

	ImGui::SameLine();

	_imgui_item_button(self, IMGUI_ANIMATION_MERGE);

	if (ImGui::BeginPopupModal(IMGUI_MERGE_POPUP, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		static s32 selectedRadioButton = ID_NONE;
		static bool isDeleteAnimationsAfter = false;

		_imgui_item_begin_child(IMGUI_MERGE_ANIMATIONS_CHILD);

		for (auto& [id, animation] : self->anm2->animations)
			animation_item(id, animation);

		_imgui_item_end_child(); //IMGUI_MERGE_ANIMATIONS_CHILD
			
		_imgui_item_begin_child(IMGUI_MERGE_ON_CONFLICT_CHILD);
		
		_imgui_item_text(IMGUI_MERGE_ON_CONFLICT);

		_imgui_item_radio_button(self, IMGUI_MERGE_APPEND_FRAMES, &selectedRadioButton);
		ImGui::SameLine();
		_imgui_item_radio_button(self, IMGUI_MERGE_REPLACE_FRAMES, &selectedRadioButton);
		_imgui_item_radio_button(self, IMGUI_MERGE_PREPEND_FRAMES, &selectedRadioButton);
		ImGui::SameLine();
		_imgui_item_radio_button(self, IMGUI_MERGE_IGNORE, &selectedRadioButton);

		_imgui_item_end_child(); //IMGUI_MERGE_ON_CONFLICT_CHILD
		
		_imgui_item_begin_child(IMGUI_MERGE_OPTIONS_CHILD);

		_imgui_item_checkbox(self, IMGUI_MERGE_DELETE_ANIMATIONS_AFTER, &isDeleteAnimationsAfter);

		_imgui_item_end_child(); //IMGUI_MERGE_OPTIONS_CHILD
		
		if (_imgui_item_button(self, IMGUI_MERGE_CONFIRM))
			ImGui::CloseCurrentPopup();
		
		ImGui::SameLine();

		if (_imgui_item_button(self, IMGUI_MERGE_CANCEL))
			ImGui::CloseCurrentPopup();

		ImGui::EndPopup();
	}

	ImGui::SameLine();
	
	if (_imgui_item_button(self, IMGUI_ANIMATION_REMOVE) && animation)
	{
		anm2_animation_remove(self->anm2, self->reference->animationID);
		anm2_reference_clear(self->reference);
	}
	
	ImGui::SameLine();
	
	if (_imgui_item_button(self, IMGUI_ANIMATION_DEFAULT) && animation)
		self->anm2->defaultAnimation = animation->name;

	if (_imgui_is_no_click_on_item())
    	anm2_reference_clear(self->reference);

	_imgui_item_end();
}

static void _imgui_events(Imgui* self)
{
	static s32 selectedEventID = ID_NONE;
	
	_imgui_item_begin(IMGUI_EVENTS);
	ImVec2 windowSize = ImGui::GetContentRegionAvail();
	
	std::function<void(s32, Anm2Event&)> event_item = [&](s32 id, Anm2Event& event)
	{
		ImGui::PushID(id);
		
		ImguiItem eventItem = IMGUI_EVENT;
		eventItem.label = std::format(IMGUI_EVENT_FORMAT, id, event.name);
		eventItem.isSelected = selectedEventID == id;
		eventItem.size.x = windowSize.x;

		if (_imgui_item_atlas_image_selectable_inputtext(self, eventItem, &event.name, id))
			selectedEventID = id;

		if (ImGui::IsItemHovered())
		{
			Anm2EventWithID eventWithID = {id, event};
			_imgui_clipboard_hovered_item_set(self, eventWithID);
			self->clipboard->location = (s32)id;
		}
			
		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
		{
			ImGui::SetDragDropPayload(eventItem.dragDrop.c_str(), &id, sizeof(s32));
			event_item(id, event);
			ImGui::EndDragDropSource();	
		}

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(eventItem.dragDrop.c_str()))
			{
				s32 sourceID = *(s32*)payload->Data;
				if (sourceID != id)
				{
					imgui_undo_stack_push(self);
					map_swap(self->anm2->events, sourceID, id);
				}
			}
			ImGui::EndDragDropTarget();
		}

		ImGui::PopID();
	};

	for (auto& [id, event] : self->anm2->events)
		event_item(id, event);
	
	if (_imgui_item_button(self, IMGUI_EVENT_ADD))
	{
		s32 id = map_next_id_get(self->anm2->events);
		self->anm2->events[id] = Anm2Event{}; 
		selectedEventID = id;
	}

	ImGui::SameLine();
	
	if (selectedEventID == ID_NONE)
	{
		_imgui_item_end();
		return;
	}
	
	if (_imgui_item_button(self, IMGUI_EVENT_REMOVE))
	{
		self->anm2->events.erase(selectedEventID);
		selectedEventID = ID_NONE;
	}

	if (_imgui_is_no_click_on_item())
		selectedEventID = ID_NONE;

	_imgui_item_end();
}

static void _imgui_spritesheets(Imgui* self)
{
	static s32 selectedSpritesheetID = ID_NONE;

	_imgui_item_begin(IMGUI_SPRITESHEETS);
	
	std::function<void(s32, Anm2Spritesheet&)> spritesheet_item = [&](s32 id, Anm2Spritesheet& spritesheet)
	{
		ImGui::PushID(id);
		
		Texture* texture = &self->resources->textures[id];
		ImguiItem spritesheetItem = IMGUI_SPRITESHEET_CHILD;

		ImguiItem spritesheetItemSelectable = IMGUI_SPRITESHEET_SELECTABLE;
		spritesheetItemSelectable.label = std::format(IMGUI_SPRITESHEET_FORMAT, id, spritesheet.path);
		spritesheetItemSelectable.isSelected = selectedSpritesheetID == id;
		
		_imgui_item_begin_child(spritesheetItem);
		if (_imgui_item_atlas_image_selectable(self, spritesheetItemSelectable))
		{
			selectedSpritesheetID = id;
			_imgui_spritesheet_editor_set(self, selectedSpritesheetID);
		}

		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
		{
			ImGui::SetDragDropPayload(spritesheetItem.dragDrop.c_str(), &id, sizeof(s32));
			spritesheet_item(id, spritesheet);
			ImGui::EndDragDropSource();
		}

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(spritesheetItem.dragDrop.c_str()))
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

		ImVec2 spritesheetPreviewSize = IMGUI_SPRITESHEET_PREVIEW_SIZE;
		f32 spritesheetAspect = (f32)self->resources->textures[id].size.x / self->resources->textures[id].size.y;

		if ((IMGUI_SPRITESHEET_PREVIEW_SIZE.x / IMGUI_SPRITESHEET_PREVIEW_SIZE.y) > spritesheetAspect)
			spritesheetPreviewSize.x = IMGUI_SPRITESHEET_PREVIEW_SIZE.y * spritesheetAspect;
		else
			spritesheetPreviewSize.y = IMGUI_SPRITESHEET_PREVIEW_SIZE.x / spritesheetAspect;

		if (texture->isInvalid)
			_imgui_atlas_image(self, TEXTURE_NONE);
		else
			ImGui::Image(texture->id, spritesheetPreviewSize);
			
		_imgui_item_end_child(); // spritesheetItem

		ImGui::PopID();
	};
	
	for (auto [id, spritesheet] : self->anm2->spritesheets)
		spritesheet_item(id, spritesheet);

	// TODO: match Nicalis

	_imgui_item_button(self, IMGUI_SPRITESHEET_ADD);
	
	ImGui::SameLine();

	if (selectedSpritesheetID == ID_NONE)
	{
		_imgui_item_end();
		return;
	}

	if (_imgui_item_button(self, IMGUI_SPRITESHEET_REMOVE))
	{
		texture_free(&self->resources->textures[selectedSpritesheetID]);
		self->resources->textures.erase(selectedSpritesheetID);
		self->anm2->spritesheets.erase(selectedSpritesheetID);
		selectedSpritesheetID = ID_NONE;
	}

	ImGui::SameLine();
	
	if (_imgui_item_button(self, IMGUI_SPRITESHEET_RELOAD))
	{
		std::filesystem::path workingPath = std::filesystem::current_path();
		working_directory_from_file_set(self->anm2->path);

		resources_texture_init(self->resources, self->anm2->spritesheets[selectedSpritesheetID].path, selectedSpritesheetID);

		std::filesystem::current_path(workingPath);
	}
	ImGui::SameLine();
	
	if (_imgui_item_button(self, IMGUI_SPRITESHEET_REPLACE))
	{
		self->dialog->replaceID = selectedSpritesheetID;
		dialog_png_replace(self->dialog);
	}

	if (_imgui_is_no_click_on_item())
    	selectedSpritesheetID = ID_NONE;
		
	_imgui_item_end();
}

static void _imgui_animation_preview(Imgui* self)
{
	static bool isPreviewHover = false;
	static vec2 mousePos{};
	static vec2 previewPos{};
	static ImVec2 previewScreenPos{};
	
	_imgui_item_begin(IMGUI_ANIMATION_PREVIEW);
	ImVec2 windowSize = ImGui::GetWindowSize();
	ImVec2 previewWindowRectSize = ImGui::GetCurrentWindow()->ClipRect.GetSize();
	
	_imgui_item_begin_child(IMGUI_ANIMATION_PREVIEW_GRID_SETTINGS);
	_imgui_item_checkbox(self, IMGUI_ANIMATION_PREVIEW_GRID, &self->settings->previewIsGrid);
	ImGui::SameLine();
	_imgui_item_coloredit4(self, IMGUI_ANIMATION_PREVIEW_GRID_COLOR, (f32*)&self->settings->previewGridColorR);
	_imgui_item_inputint2(self, IMGUI_ANIMATION_PREVIEW_GRID_SIZE, (s32*)&self->settings->previewGridSizeX);
	_imgui_item_inputint2(self, IMGUI_ANIMATION_PREVIEW_GRID_OFFSET, (s32*)&self->settings->previewGridOffsetX);
	_imgui_item_end_child();
	
	ImGui::SameLine();
	
	_imgui_item_begin_child(IMGUI_ANIMATION_PREVIEW_VIEW_SETTINGS);
	_imgui_item_dragfloat(self, IMGUI_ANIMATION_PREVIEW_ZOOM, &self->settings->previewZoom);
	if (_imgui_item_button(self, IMGUI_ANIMATION_PREVIEW_CENTER_VIEW))
	{
		self->settings->previewPanX = -(previewWindowRectSize.x - PREVIEW_SIZE.x) * 0.5f;
		self->settings->previewPanY = -((previewWindowRectSize.y - PREVIEW_SIZE.y) * 0.5f) + (previewPos.y * 0.5f); 
	}
	std::string mousePositionString = std::format(IMGUI_POSITION_FORMAT, (s32)mousePos.x, (s32)mousePos.y);
	ImGui::Text(mousePositionString.c_str());
	_imgui_item_end_child();

	ImGui::SameLine();
	
	_imgui_item_begin_child(IMGUI_ANIMATION_PREVIEW_BACKGROUND_SETTINGS);
	_imgui_item_coloredit4(self, IMGUI_ANIMATION_PREVIEW_BACKGROUND_COLOR, (f32*)&self->settings->previewBackgroundColorR);
	_imgui_item_end_child();

	ImGui::SameLine();

	_imgui_item_begin_child(IMGUI_ANIMATION_PREVIEW_HELPER_SETTINGS);
	_imgui_item_checkbox(self, IMGUI_ANIMATION_PREVIEW_AXIS, &self->settings->previewIsAxis);
	ImGui::SameLine();
	_imgui_item_coloredit4(self, IMGUI_ANIMATION_PREVIEW_AXIS_COLOR, (f32*)&self->settings->previewAxisColorR);
	_imgui_item_checkbox(self, IMGUI_ANIMATION_PREVIEW_ROOT_TRANSFORM, &self->settings->previewIsRootTransform);
	_imgui_item_checkbox(self, IMGUI_ANIMATION_PREVIEW_SHOW_PIVOT, &self->settings->previewIsShowPivot);
	_imgui_item_end_child();

	previewPos = IMVEC2_TO_VEC2(ImGui::GetCursorPos());
	previewScreenPos = ImGui::GetCursorScreenPos();
	ImGui::Image(self->preview->texture, VEC2_TO_IMVEC2(PREVIEW_SIZE));
	self->preview->recordSize = vec2(windowSize.x, windowSize.y - IMGUI_CANVAS_CHILD_SIZE.y);
	
	if (ImGui::IsItemHovered())
	{
		if (!isPreviewHover)
			SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_DEFAULT));

		isPreviewHover = true;
	}
	else
		isPreviewHover = false;

	Anm2Frame trigger{};
	anm2_frame_from_time(self->anm2, &trigger, Anm2Reference{self->reference->animationID, ANM2_TRIGGERS}, self->preview->time);

	if (trigger.eventID != ID_NONE)
	{
		static const ImU32 textColor = ImGui::GetColorU32(IMGUI_TIMELINE_FRAME_COLOR);
		ImGui::GetWindowDrawList()->AddText(previewScreenPos, textColor, self->anm2->events[trigger.eventID].name.c_str());
	}

	if (!isPreviewHover)
	{
		_imgui_keyboard_navigation_set(true);
		_imgui_item_end();
		return;
	}

	_imgui_keyboard_navigation_set(false);
	
	vec2 windowPos = IMVEC2_TO_VEC2(ImGui::GetWindowPos());
	mousePos = IMVEC2_TO_VEC2(ImGui::GetMousePos());

	mousePos -= (windowPos + previewPos);
	mousePos -= (PREVIEW_SIZE * 0.5f);
	mousePos += vec2(self->settings->previewPanX, self->settings->previewPanY);
	mousePos /= PERCENT_TO_UNIT(self->settings->previewZoom);

	ToolType tool = self->tool;
	bool isLeft  = ImGui::IsKeyDown(IMGUI_INPUT_LEFT);
	bool isRight = ImGui::IsKeyDown(IMGUI_INPUT_RIGHT);
	bool isUp    = ImGui::IsKeyDown(IMGUI_INPUT_UP);
	bool isDown  = ImGui::IsKeyDown(IMGUI_INPUT_DOWN);
	bool isMod   = ImGui::IsKeyDown(IMGUI_INPUT_MOD);
	bool isMouseClick = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
	bool isMouseDown = ImGui::IsMouseDown(ImGuiMouseButton_Left);
	bool isMouseMiddleDown = ImGui::IsMouseDown(ImGuiMouseButton_Middle);
	ImVec2 mouseDelta = ImGui::GetIO().MouseDelta;
	f32 mouseWheel = ImGui::GetIO().MouseWheel;

	SDL_SetCursor(SDL_CreateSystemCursor(IMGUI_TOOL_MOUSE_CURSORS[tool]));

	if (self->tool == TOOL_MOVE || self->tool == TOOL_SCALE || self->tool == TOOL_ROTATE)
		if (isMouseClick || isLeft || isRight || isUp || isDown)
			imgui_undo_stack_push(self);
	
	if ((self->tool == TOOL_PAN && isMouseDown) || isMouseMiddleDown)
	{
		self->settings->previewPanX -= mouseDelta.x;
		self->settings->previewPanY -= mouseDelta.y;
	}

	Anm2Frame* frame = nullptr;
	if (self->reference->itemType != ANM2_TRIGGERS) 
		frame = anm2_frame_from_reference(self->anm2, self->reference);

	if (frame)
	{
		f32 step = isMod ? IMGUI_TOOL_STEP_MOD : IMGUI_TOOL_STEP;
		
		switch (tool)
		{
			case TOOL_MOVE:
				if (isMouseDown) 
					frame->position = IMVEC2_TO_VEC2(mousePos);
				else 
				{
					if (isLeft)  frame->position.x -= step;
					if (isRight) frame->position.x += step;
					if (isUp)    frame->position.y -= step;
					if (isDown)  frame->position.y += step;
				}
				break;
			case TOOL_ROTATE:
				if (isMouseDown)
					frame->rotation += mouseDelta.x;
				else
				{
					if (isLeft || isUp) frame->rotation -= step;
					if (isRight || isDown) frame->rotation += step;
				}
				break;
			case TOOL_SCALE:
				if (isMouseDown)
					frame->scale += IMVEC2_TO_VEC2(ImGui::GetIO().MouseDelta);
				else
				{
					if (isLeft)  frame->scale.x -= step;
					if (isRight) frame->scale.x += step;
					if (isUp)    frame->scale.y -= step;
					if (isDown)  frame->scale.y += step;
				}
				break;
			default:
				break;
		}
	}

	if (mouseWheel != 0 || ImGui::IsKeyPressed(IMGUI_INPUT_ZOOM_IN) || ImGui::IsKeyPressed(IMGUI_INPUT_ZOOM_OUT))
	{
		const f32 delta = (mouseWheel > 0 || ImGui::IsKeyPressed(IMGUI_INPUT_ZOOM_IN)) ? CANVAS_ZOOM_STEP : -CANVAS_ZOOM_STEP;
		self->settings->previewZoom = ROUND_NEAREST_FLOAT(self->settings->previewZoom + delta, CANVAS_ZOOM_STEP);
		self->settings->previewZoom = CLAMP(self->settings->previewZoom, CANVAS_ZOOM_MIN, CANVAS_ZOOM_MAX);
	}

	_imgui_item_end();
}

static void _imgui_spritesheet_editor(Imgui* self)
{
	static bool isEditorHover = false;
	static vec2 mousePos = {0, 0};
	std::string mousePositionString = std::format(IMGUI_POSITION_FORMAT, (s32)mousePos.x, (s32)mousePos.y);

	_imgui_item_begin(IMGUI_SPRITESHEET_EDITOR);
	
	_imgui_item_begin_child(IMGUI_SPRITESHEET_EDITOR_GRID_SETTINGS);
	_imgui_item_checkbox(self, IMGUI_SPRITESHEET_EDITOR_GRID, &self->settings->editorIsGrid);
	ImGui::SameLine();
	_imgui_item_checkbox(self, IMGUI_SPRITESHEET_EDITOR_GRID_SNAP, &self->settings->editorIsGridSnap);
	ImGui::SameLine();
	_imgui_item_coloredit4(self, IMGUI_SPRITESHEET_EDITOR_GRID_COLOR, (f32*)&self->settings->editorGridColorR);
	_imgui_item_inputint2(self, IMGUI_SPRITESHEET_EDITOR_GRID_SIZE, (s32*)&self->settings->editorGridSizeX);
	_imgui_item_inputint2(self, IMGUI_SPRITESHEET_EDITOR_GRID_OFFSET, (s32*)&self->settings->editorGridOffsetX);
	_imgui_item_end_child();
	
	ImGui::SameLine();
	
	_imgui_item_begin_child(IMGUI_SPRITESHEET_EDITOR_VIEW_SETTINGS);
	_imgui_item_dragfloat(self, IMGUI_SPRITESHEET_EDITOR_ZOOM, &self->settings->editorZoom);
	if (_imgui_item_button(self, IMGUI_SPRITESHEET_EDITOR_CENTER_VIEW))
	{
		self->settings->editorPanX = EDITOR_SIZE.x / 2.0f;
		self->settings->editorPanY = EDITOR_SIZE.y / 2.0f;
	}
	ImGui::Text(mousePositionString.c_str());
	_imgui_item_end_child();
	
	ImGui::SameLine();
	
	_imgui_item_begin_child(IMGUI_SPRITESHEET_EDITOR_BACKGROUND_SETTINGS);
	_imgui_item_coloredit4(self, IMGUI_SPRITESHEET_EDITOR_BACKGROUND_COLOR, (f32*)&self->settings->editorBackgroundColorR);
	_imgui_item_checkbox(self, IMGUI_SPRITESHEET_EDITOR_BORDER, &self->settings->editorIsBorder);
	_imgui_item_end_child();
	
	vec2 editorPos = IMVEC2_TO_VEC2(ImGui::GetCursorPos());
	ImGui::Image(self->editor->texture, VEC2_TO_IMVEC2(EDITOR_SIZE));
	
	if (!ImGui::IsItemHovered())
	{
		if (isEditorHover)
		{
			SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_DEFAULT));
			isEditorHover = false;
		}
		_imgui_keyboard_navigation_set(true);
		_imgui_item_end();
		return;
	}

	isEditorHover = true;
	_imgui_keyboard_navigation_set(false);
	
	vec2 windowPos = IMVEC2_TO_VEC2(ImGui::GetWindowPos());
	mousePos = IMVEC2_TO_VEC2(ImGui::GetMousePos());

	mousePos -= (windowPos + editorPos);
	mousePos -= (EDITOR_SIZE * 0.5f);
	mousePos += vec2(self->settings->editorPanX, self->settings->editorPanY);
	mousePos /= PERCENT_TO_UNIT(self->settings->editorZoom);

	bool isMouseClick = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
	bool isMouseDown = ImGui::IsMouseDown(ImGuiMouseButton_Left);
	bool isMouseMiddleDown = ImGui::IsMouseDown(ImGuiMouseButton_Middle);
	f32 mouseWheel = ImGui::GetIO().MouseWheel;
	ImVec2 mouseDelta = ImGui::GetIO().MouseDelta;
	
	SDL_SetCursor(SDL_CreateSystemCursor(IMGUI_TOOL_MOUSE_CURSORS[self->tool]));

	if ((self->tool == TOOL_PAN && isMouseDown) || isMouseMiddleDown)
	{
		self->settings->editorPanX -= mouseDelta.x;
		self->settings->editorPanY -= mouseDelta.y;
	}

	Anm2Frame* frame = nullptr;
	if (self->reference->itemType == ANM2_LAYER) 
		frame = anm2_frame_from_reference(self->anm2, self->reference);

	if (frame && self->tool == TOOL_CROP)
	{
		if (isMouseClick)
		{
			imgui_undo_stack_push(self);
			
			vec2 cropPosition = mousePos + IMGUI_SPRITESHEET_EDITOR_CROP_FORGIVENESS;

			if (self->settings->editorIsGridSnap)
			{
				cropPosition.x = (s32)(cropPosition.x / self->settings->editorGridSizeX) * self->settings->editorGridSizeX;
				cropPosition.y = (s32)(cropPosition.y / self->settings->editorGridSizeX) * self->settings->editorGridSizeY;
			}

			frame->crop = cropPosition;
			frame->size = {0, 0};
		}
		else if (isMouseDown)
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
			
	if (mouseWheel != 0 || ImGui::IsKeyPressed(IMGUI_INPUT_ZOOM_IN) || ImGui::IsKeyPressed(IMGUI_INPUT_ZOOM_OUT))
	{
		const f32 delta = (mouseWheel > 0 || ImGui::IsKeyPressed(IMGUI_INPUT_ZOOM_IN)) ? CANVAS_ZOOM_STEP : -CANVAS_ZOOM_STEP;
		self->settings->editorZoom = ROUND_NEAREST_FLOAT(self->settings->editorZoom + delta, CANVAS_ZOOM_STEP);
		self->settings->editorZoom = CLAMP(self->settings->editorZoom, CANVAS_ZOOM_MIN, CANVAS_ZOOM_MAX);
	}

	_imgui_item_end();
}

static void _imgui_frame_properties(Imgui* self)
{
	_imgui_item_begin(IMGUI_FRAME_PROPERTIES);

	Anm2Frame* frame = anm2_frame_from_reference(self->anm2, self->reference);

	if (!frame)
	{
		ImGui::Text(IMGUI_FRAME_PROPERTIES_NO_FRAME);
		_imgui_item_end();
		return;
	}
	
	Anm2Type type = self->reference->itemType;
	Anm2Animation* animation = anm2_animation_from_reference(self->anm2, self->reference);

	ImGui::Text(IMGUI_FRAME_PROPERTIES_TITLE[type].c_str());
	
	if (type == ANM2_ROOT || type == ANM2_NULL || type == ANM2_LAYER)
	{
		_imgui_item_dragfloat2(self, IMGUI_FRAME_PROPERTIES_POSITION, value_ptr(frame->position));
		
		if (type == ANM2_LAYER)
		{
			_imgui_item_dragfloat2(self, IMGUI_FRAME_PROPERTIES_CROP, value_ptr(frame->crop));
			_imgui_item_dragfloat2(self, IMGUI_FRAME_PROPERTIES_SIZE, value_ptr(frame->size));
			_imgui_item_dragfloat2(self, IMGUI_FRAME_PROPERTIES_PIVOT, value_ptr(frame->pivot));
		}
		
		_imgui_item_dragfloat2(self, IMGUI_FRAME_PROPERTIES_SCALE, value_ptr(frame->scale));
		_imgui_item_dragfloat(self, IMGUI_FRAME_PROPERTIES_ROTATION, &frame->rotation);
		_imgui_item_inputint(self, IMGUI_FRAME_PROPERTIES_DURATION, &frame->delay);
		_imgui_item_coloredit4(self, IMGUI_FRAME_PROPERTIES_TINT, value_ptr(frame->tintRGBA));
		_imgui_item_coloredit3(self, IMGUI_FRAME_PROPERTIES_COLOR_OFFSET, value_ptr(frame->offsetRGB));
		
		if (_imgui_item_button(self, IMGUI_FRAME_PROPERTIES_FLIP_X))
			frame->scale.x = -frame->scale.x;
		ImGui::SameLine();
		if (_imgui_item_button(self, IMGUI_FRAME_PROPERTIES_FLIP_Y))
			frame->scale.y = -frame->scale.y;

		_imgui_item_checkbox(self, IMGUI_FRAME_PROPERTIES_VISIBLE, &frame->isVisible);
		ImGui::SameLine();
		_imgui_item_checkbox(self, IMGUI_FRAME_PROPERTIES_INTERPOLATED, &frame->isInterpolated);
			
	}
	else if (type == ANM2_TRIGGERS)
	{
		std::vector<std::string> eventStrings;
		std::vector<const char*> eventLabels;
		std::vector<s32> eventIDs;
	
		eventStrings.reserve(self->anm2->events.size() + 1);
		eventIDs.reserve(self->anm2->events.size() + 1);
		eventLabels.reserve(self->anm2->events.size() + 1);

		eventIDs.push_back(ID_NONE);
		eventStrings.push_back(IMGUI_EVENT_NONE);
		eventLabels.push_back(eventStrings.back().c_str());
		
		for (auto & [id, event] : self->anm2->events) 
		{
			eventIDs.push_back(id);
			eventStrings.push_back(std::format(IMGUI_EVENT_FORMAT, id, event.name));
			eventLabels.push_back(eventStrings.back().c_str());
		}

		s32 selectedEventIndex = std::find(eventIDs.begin(), eventIDs.end(), frame->eventID) - eventIDs.begin();

		if (_imgui_item_combo(self, IMGUI_FRAME_PROPERTIES_EVENT, &selectedEventIndex, eventLabels.data(), (s32)eventLabels.size()))
			frame->eventID = eventIDs[selectedEventIndex];
				
		_imgui_item_inputint(self, IMGUI_FRAME_PROPERTIES_AT_FRAME, &frame->atFrame);
		frame->atFrame = CLAMP(frame->atFrame, 0, animation->frameNum - 1);
	}

	_imgui_item_end();
}

static void _imgui_persistent(Imgui* self)
{
	if (self->preview->isRecording)
	{
		ImVec2 mousePos = ImGui::GetMousePos();

		ImGui::SetNextWindowPos(ImVec2(mousePos.x + IMGUI_TOOLTIP_OFFSET.x, mousePos.y + IMGUI_TOOLTIP_OFFSET.y));
		ImGui::BeginTooltip();
		_imgui_item_atlas_image_text(self, IMGUI_RECORDING);
		ImGui::EndTooltip();
	}

	if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
		ImGui::OpenPopup(IMGUI_CONTEXT_MENU);

	if (ImGui::BeginPopup(IMGUI_CONTEXT_MENU))
	{
		ImguiItem pasteItem = IMGUI_PASTE;
		pasteItem.isInactive = self->clipboard->item.type == CLIPBOARD_NONE;

		_imgui_item_selectable(self, IMGUI_CUT);			
		_imgui_item_selectable(self, IMGUI_COPY);			
		_imgui_item_selectable(self, pasteItem);

		ImGui::EndPopup();	
	}
}

static void _imgui_dock(Imgui* self)
{
	ImGuiViewport* viewport = ImGui::GetMainViewport();

    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + IMGUI_TASKBAR.size.y));
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, viewport->Size.y - IMGUI_TASKBAR.size.y));
    ImGui::SetNextWindowViewport(viewport->ID);

	_imgui_item_begin(IMGUI_WINDOW);

	_imgui_item_dockspace(IMGUI_DOCKSPACE);

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

void  imgui_init
(
    Imgui* self,
    Dialog* dialog,
    Resources* resources,
    Anm2* anm2,
    Anm2Reference* reference,
    Editor* editor,
    Preview* preview,
    Settings* settings,
    Snapshots* snapshots,
    Clipboard* clipboard,
    SDL_Window* window,
    SDL_GLContext* glContext
)
{
	IMGUI_CHECKVERSION();

	self->dialog = dialog;
	self->resources = resources;
	self->anm2 = anm2;
	self->reference = reference;
	self->editor = editor;
	self->preview = preview;
	self->settings = settings;
	self->snapshots = snapshots;
	self->clipboard = clipboard;
	self->window = window;
	self->glContext = glContext;

	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	ImGui_ImplSDL3_InitForOpenGL(self->window, *self->glContext);
	ImGui_ImplOpenGL3_Init(IMGUI_OPENGL_VERSION);

	ImGuiIO& io = ImGui::GetIO();
	io.IniFilename = nullptr;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigWindowsMoveFromTitleBarOnly = true;

	ImGui::LoadIniSettingsFromDisk(SETTINGS_PATH);
}

void imgui_tick(Imgui* self)
{
	ImGui_ImplSDL3_NewFrame();
	ImGui_ImplOpenGL3_NewFrame();
	ImGui::NewFrame();

	_imgui_taskbar(self);
	_imgui_dock(self);
	_imgui_persistent(self);

	self->isHotkeysEnabled = !self->isRename && !self->isChangeValue;

	if (self->isHotkeysEnabled)
	{
		for (const auto& hotkey : imgui_hotkey_registry())
		{
			if (ImGui::IsKeyChordPressed(hotkey.chord))
			{
				if (hotkey.is_focus_window() && (imgui_nav_window_root_get() != hotkey.focusWindow)) 
					continue;

				hotkey.function(self);
			}
		}
	}
	
	SDL_Event event;

	while(SDL_PollEvent(&event))
	{
    	ImGui_ImplSDL3_ProcessEvent(&event);
		
		switch (event.type)
		{
			case SDL_EVENT_QUIT:
				if (ImGui::IsPopupOpen(IMGUI_EXIT_CONFIRMATION_POPUP))
				{
					self->isQuit = true;
					break;
				}
				ImGui::OpenPopup(IMGUI_EXIT_CONFIRMATION_POPUP);
				break;
			default:
				break;
		}
	}

	if (_imgui_item_yes_no_popup(self, IMGUI_EXIT_CONFIRMATION))
		self->isQuit = true;
}

void imgui_draw(void)
{
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void imgui_free(void)
{
	ImGui_ImplSDL3_Shutdown();
	ImGui_ImplOpenGL3_Shutdown();

	ImGui::SaveIniSettingsToDisk(SETTINGS_PATH);
	ImGui::DestroyContext();
}