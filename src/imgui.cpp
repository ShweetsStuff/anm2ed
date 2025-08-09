// Handles everything imgui

#include "imgui.h"

static bool _imgui_window_color_from_position_get(SDL_Window* self, vec2 position, vec4* color)
{
    if (!self || !color) return false;

    ImGuiIO& io = ImGui::GetIO();
	ivec2 framebufferPosition = {(s32)(position.x * io.DisplayFramebufferScale.x), (s32)(position.y * io.DisplayFramebufferScale.y)};
	ivec2 framebufferSize{};
    SDL_GetWindowSizeInPixels(self, &framebufferSize.x, &framebufferSize.y);
 
	if 
	(
		framebufferPosition.x < 0 || framebufferPosition.y < 0 || 
		framebufferPosition.x >= framebufferSize.x || 
		framebufferPosition.y >= framebufferSize.y
	)
		return false;

    uint8_t rgba[4];

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(framebufferPosition.x, framebufferSize.y - 1 - framebufferPosition.y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, rgba);

    *color = vec4(U8_TO_FLOAT(rgba[0]), U8_TO_FLOAT(rgba[1]), U8_TO_FLOAT(rgba[2]), U8_TO_FLOAT(rgba[3]));

    return true;
}

template<typename T>
static void _imgui_clipboard_hovered_item_set(Imgui* self, const T& data)
{
	self->clipboard->hoveredItem = ClipboardItem(data);
}

static void _imgui_atlas_image(Imgui* self, TextureType type)
{
	ImGui::Image(self->resources->atlas.id, ATLAS_SIZES[type], ATLAS_UV_ARGS(type));
}

static void _imgui_item_text(const ImguiItem& item)
{
	ImGui::Text(item.label.c_str());
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
		if (item.is_focus_window() && (imgui_nav_window_root_get() == item.focusWindow))
			*isActivated = true;

	if (item.is_tooltip() && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) 
		ImGui::SetTooltip(item.tooltip.c_str());

	if (isActivated && *isActivated)
	{
		if (item.isUndoable) imgui_undo_stack_push(self, item.action);
        if (item.function) item.function(self);

		if (item.is_popup())
		{
			ImGui::OpenPopup(item.popup.c_str());

			switch (item.popupType)
			{
				case IMGUI_POPUP_CENTER_SCREEN:
					ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
					break;
				case IMGUI_POPUP_BY_ITEM:
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
		vec4 color = item.isInactive ? IMGUI_INACTIVE_COLOR : item.color.normal;
		ImGui::PushStyleColor(ImGuiCol_Text, color);
		flags |= ImGuiSelectableFlags_Disabled;
	}

	ImVec2 size = item.is_size() ? item.size : item.isSizeToText ? ImGui::CalcTextSize(label) :ImVec2(0, 0);
	bool isActivated = ImGui::Selectable(label, item.isSelected, flags, size);
	_imgui_item(self, item, &isActivated);

	if (item.isInactive || item.color.is_normal()) ImGui::PopStyleColor();

	return isActivated;
}

static bool _imgui_item_inputint(Imgui* self, const ImguiItem& item, s32& value)
{
	if (item.is_size()) ImGui::SetNextItemWidth(item.size.x);

	bool isActivated = ImGui::InputInt(item.label.c_str(), &value, item.step, item.stepFast, item.flags);

	if (item.min != 0 || item.max != 0)
		value = std::clamp(value, (s32)item.min, (s32)item.max);
		
	_imgui_item(self, item, &isActivated);

    return isActivated;
}

static bool _imgui_item_inputint2(Imgui* self, const ImguiItem& item, ivec2& value)
{
	if (item.is_size()) ImGui::SetNextItemWidth(item.size.x);

	ImGui::InputInt2(item.label.c_str(), value_ptr(value));
	bool isActivated = ImGui::IsItemActivated();

	if (item.min > 0 || item.max > 0)
		for (s32 i = 0; i < 2; i++)
			value[i] = std::clamp(value[i], (s32)item.min, (s32)item.max);
		
	_imgui_item(self, item, &isActivated);

	return isActivated;
}

static bool _imgui_item_inputtext(Imgui* self, const ImguiItem& item, std::string& buffer)
{
	if ((s32)buffer.size() < item.max) buffer.resize(item.max); 

	ImVec2 size = item.is_size() ? item.size : ImVec2(-FLT_MIN, 0);
	
	ImGui::SetNextItemWidth(size.x);
		
	ImGui::InputText(item.label.c_str(), &buffer[0], item.max, item.flags);
	bool isActivated = ImGui::IsItemActivated();
	_imgui_item(self, item, &isActivated);

    return isActivated;
}

static bool _imgui_item_checkbox(Imgui* self, const ImguiItem& item, bool& value)
{
	ImGui::Checkbox(item.label.c_str(), &value);
	bool isActivated = ImGui::IsItemActivated();

	if (item.is_mnemonic() && ImGui::IsKeyChordPressed(ImGuiMod_Alt | item.mnemonicKey))
	{
		value = !value;
		ImGui::CloseCurrentPopup();
	}

	_imgui_item(self, item, &isActivated);

	return isActivated;
}

static bool _imgui_item_radio_button(Imgui* self, const ImguiItem& item, s32& value)
{
	if (item.is_size()) ImGui::SetNextItemWidth(item.size.x);

	bool isActivated = ImGui::RadioButton(item.label.c_str(), &value, item.value);
	_imgui_item(self, item, &isActivated);
	
	return isActivated;
}

static bool _imgui_item_dragfloat(Imgui* self, const ImguiItem& item, f32& value)
{
	ImGui::DragFloat(item.label.c_str(), &value, item.speed, item.min, item.max, item.format.c_str());
	bool isActivated = ImGui::IsItemActivated();
	_imgui_item(self, item, &isActivated);
	return isActivated; 
}

static bool _imgui_item_colorbutton(Imgui* self, const ImguiItem& item, vec4& color)
{
	ImGui::ColorButton(item.label.c_str(), color, item.flags);
	bool isActivated = ImGui::IsItemActivated();
	_imgui_item(self, item, &isActivated);
	return isActivated;
}

static bool _imgui_item_coloredit3(Imgui* self, const ImguiItem& item, vec3& value)
{
	ImGui::ColorEdit3(item.label.c_str(), value_ptr(value), item.flags);
	bool isActivated = ImGui::IsItemActivated();
	_imgui_item(self, item, &isActivated);
	return isActivated;
}

static bool _imgui_item_coloredit4(Imgui* self, const ImguiItem& item, vec4& value)
{
	ImGui::ColorEdit4(item.label.c_str(), value_ptr(value), item.flags);
	bool isActivated = ImGui::IsItemActivated();
	_imgui_item(self, item, &isActivated);
	return isActivated;
}

static bool _imgui_item_dragfloat2(Imgui* self, const ImguiItem& item, vec2& value)
{
	ImGui::DragFloat2(item.label.c_str(), value_ptr(value), item.speed, item.min, item.max, item.format.c_str());
	bool isActivated = ImGui::IsItemActivated();
	_imgui_item(self, item, &isActivated);
	return isActivated;
}

static bool _imgui_item_button(Imgui* self, const ImguiItem& item)
{
	ImVec2 size = item.is_size() ? item.size : ImVec2(0, 0);

	if (item.isSizeToChild)
		size.x = (ImGui::GetWindowSize().x - ImGui::GetStyle().ItemSpacing.x * (item.childRowItemCount + 1)) / item.childRowItemCount;

	bool isActivated = ImGui::Button(item.label.c_str(), size);
	_imgui_item(self, item, &isActivated);
	return isActivated;
}

static bool _imgui_item_selectable_inputtext(Imgui* self, const ImguiItem& item, std::string& string, s32 id)
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

		isActivated = _imgui_item_inputtext(self, itemRenamable, buffer);

		if (isActivated || _imgui_is_no_click_on_item())
        {
			if (itemRenamable.isUndoable) imgui_undo_stack_push(self, itemRenamable.action);

			string = buffer;
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
			buffer = string;
            renameID = id;
            itemID = item.id;
            ImGui::SetKeyboardFocusHere(-1);
        }
    }

	_imgui_item(self, item, &isActivated);

	return isActivated;
}

static bool _imgui_item_selectable_inputint(Imgui* self, const ImguiItem& item, s32& value, s32 id)
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

		isActivated = _imgui_item_inputint(self, itemChangeable, value);
	
		if (isActivated || _imgui_is_no_click_on_item())
        {
			if (itemChangeable.isUndoable) imgui_undo_stack_push(self, itemChangeable.action);
			
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

static bool _imgui_item_text_inputtext(Imgui* self, const ImguiItem& item, std::string& string)
{
	ImguiItem copyItem = item;
	copyItem.label = string;
	bool isActivated = _imgui_item_selectable_inputtext(self, copyItem, string, item.id);
	ImGui::SameLine();
	ImGui::Text(item.label.c_str());

	return isActivated;
}

static bool _imgui_item_atlas_image_selectable_inputtext(Imgui* self, ImguiItem& item, std::string& string, s32 id)
{
	_imgui_atlas_image(self, item.texture);
	ImGui::SameLine();
	return _imgui_item_selectable_inputtext(self, item, string, id);
}

static bool _imgui_item_atlas_image_selectable_inputint(Imgui* self, ImguiItem& item, s32& value, s32 id)
{
	_imgui_atlas_image(self, item.texture);
	ImGui::SameLine();
	return _imgui_item_selectable_inputint(self, item, value, id);
}

static bool _imgui_item_atlas_image_checkbox_selectable(Imgui* self, const ImguiItem& item, bool& value)
{
	ImguiItem checkboxItem = item;
	checkboxItem.label = IMGUI_INVISIBLE_LABEL_MARKER + item.label;

	_imgui_item_checkbox(self, checkboxItem, value);
	ImGui::SameLine();
	_imgui_atlas_image(self, item.texture);
	ImGui::SameLine();
	return _imgui_item_selectable(self, item);
}

static bool _imgui_item_atlas_image_button(Imgui* self, const ImguiItem& item)
{
	bool isActivated = false;
	ImVec2 imageSize = (ATLAS_SIZES[item.texture]);
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

		ImGui::GetWindowDrawList()->AddImage(self->resources->atlas.id, imageMin, imageMax, ATLAS_UV_ARGS(item.texture));
	}
	else
		isActivated = ImGui::ImageButton(item.label.c_str(), self->resources->atlas.id, buttonSize, ATLAS_UV_ARGS(item.texture));
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
	static const ImU32 headerFrameInactiveColor = ImGui::GetColorU32(IMGUI_TIMELINE_HEADER_FRAME_INACTIVE_COLOR);
	static const ImU32 headerFrameMultipleInactiveColor = ImGui::GetColorU32(IMGUI_TIMELINE_HEADER_FRAME_MULTIPLE_INACTIVE_COLOR);
	static const ImU32 textColor = ImGui::GetColorU32(ImGuiCol_Text);
	static ImVec2 scroll{};
	static ImVec2 pickerPos{};
	static Anm2Reference swapItemReference;
	static bool isItemSwap = false; 
	static ImVec2 itemMin{};
	static ImVec2 mousePos{};
	static ImVec2 localMousePos{};
	static s32 frameTime = INDEX_NONE;
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

	s32 actualLength = anm2_animation_length_get(animation);
	ImVec2 actualFramesSize = {actualLength * frameSize.x, frameSize.y};
	ImVec2 framesSize = {animation->frameNum * frameSize.x, frameSize.y};

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
		frameTime = std::clamp((s32)(localMousePos.x / frameSize.x), 0, animation->frameNum - 1);

		if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && _imgui_is_window_hovered())
			isHeaderClicked = true;

		if (isHeaderClicked)
		{
			self->preview->time = frameTime;

			if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) 
				isHeaderClicked = false;
		}

		ImVec2 cursorPos = ImGui::GetCursorScreenPos();
		pickerPos = {cursorPos.x + (self->preview->time * frameSize.x), cursorPos.y};

		ImDrawList* drawList = ImGui::GetWindowDrawList();

		ImVec2 dummySize = actualFramesSize.x > framesSize.x ? actualFramesSize : framesSize;
		ImGui::Dummy(dummySize);

		f32 viewWidth    = ImGui::GetWindowContentRegionMax().x - ImGui::GetWindowContentRegionMin().x;
		s32 start = (s32)std::floor(scroll.x / frameSize.x) - 1;
		start = (start < 0) ? 0 : start;

		s32 end   = (s32)std::ceil((scroll.x + viewWidth) / frameSize.x) + 1;
		end = (end > ANM2_FRAME_NUM_MAX) ? ANM2_FRAME_NUM_MAX : end;

		pickerPos = ImVec2(cursorPos.x + self->preview->time * frameSize.x, cursorPos.y);

		for (s32 i = start; i < end; i++)
		{
			bool isMultiple = (i % IMGUI_TIMELINE_FRAME_MULTIPLE) == 0;
			bool isInactive = i >= animation->frameNum;

			f32 startX = cursorPos.x + i * frameSize.x;
			f32 endX = startX + frameSize.x;
			ImVec2 positionStart(startX, cursorPos.y);
			ImVec2 positionEnd(endX, cursorPos.y + frameSize.y);

			ImU32 bgColor = isInactive
				? (isMultiple ? headerFrameMultipleInactiveColor : headerFrameInactiveColor)
				: (isMultiple ? headerFrameMultipleColor         : headerFrameColor);

			drawList->AddRectFilled(positionStart, positionEnd, bgColor);

			if (isMultiple)
			{
				std::string buffer = std::to_string(i);
				ImVec2 textSize = ImGui::CalcTextSize(buffer.c_str());
				ImVec2 textPosition(startX + (frameSize.x - textSize.x) * 0.5f, cursorPos.y + (frameSize.y - textSize.y) * 0.5f);
				drawList->AddText(textPosition, textColor, buffer.c_str());
			}

			drawList->AddImage(self->resources->atlas.id, positionStart, positionEnd, ATLAS_UV_ARGS(TEXTURE_FRAME));
		}

		_imgui_item_end_child(); // IMGUI_TIMELINE_HEADER
	
		ImGui::SetNextWindowPos(ImGui::GetWindowPos());
		ImGui::SetNextWindowSize(ImGui::GetWindowSize());
		_imgui_item_begin(IMGUI_TIMELINE_PICKER);

		ImVec2& pos = pickerPos;
		
		ImVec2 lineStart = {pos.x + (frameSize.x * 0.5f) - (IMGUI_TIMELINE_PICKER_LINE_WIDTH * 0.5f), pos.y + frameSize.y};
		ImVec2 lineEnd = {lineStart.x + IMGUI_TIMELINE_PICKER_LINE_WIDTH, lineStart.y + timelineChild.size.y - frameSize.y};
		
		drawList = ImGui::GetWindowDrawList();

		drawList->PushClipRect(clipRectMin, clipRectMax, true);
		drawList->AddImage(self->resources->atlas.id, pos, ImVec2(pos.x + frameSize.x, pos.y + frameSize.y), ATLAS_UV_ARGS(TEXTURE_PICKER));
		drawList->AddRectFilled(lineStart, lineEnd, IMGUI_PICKER_LINE_COLOR);
		drawList->PopClipRect();

		_imgui_item_end();
	};

	std::function<void(Anm2Reference, s32&)> timeline_item_child = [&](Anm2Reference reference, s32& index)
	{
		Anm2Item* item = anm2_item_from_reference(self->anm2, &reference);

		if (!item) return;

		ImVec2 buttonSize = ImVec2(TEXTURE_SIZE) + (defaultFramePadding * ImVec2(2, 2));
		
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
				if (_imgui_item_atlas_image_selectable_inputtext(self, imguiItemSelectable, layer->name, index))
					*self->reference = reference;
				break;
			case ANM2_NULL:
				null = &self->anm2->nulls[reference.itemID];
				imguiItemSelectable.label = std::format(IMGUI_TIMELINE_CHILD_ID_LABEL, reference.itemID, null->name);
				if (_imgui_item_atlas_image_selectable_inputtext(self, imguiItemSelectable, null->name, index))
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
			_imgui_item_atlas_image_selectable_inputint(self, spritesheetIDItem, layer->spritesheetID, index);
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

		index++;
	};

	std::function<void()> timeline_items_child = [&]()
	{
		s32 index = 0;

		s32& animationID = self->reference->animationID;

		_imgui_item_begin_child(IMGUI_TIMELINE_ITEMS_CHILD);
		ImGui::SetScrollY(scroll.y);

		timeline_item_child({animationID, ANM2_ROOT, ID_NONE}, index);

		for (auto& [i, id] : std::ranges::reverse_view(self->anm2->layerMap))
			timeline_item_child({animationID, ANM2_LAYER, id}, index);

		for (auto & [id, null] : animation->nullAnimations)
			timeline_item_child({animationID, ANM2_NULL, id}, index);

		timeline_item_child({animationID, ANM2_TRIGGERS, ID_NONE}, index);

		_imgui_item_end_child(); // IMGUI_TIMELINE_ITEMS_CHILD

		if (isItemSwap)
		{
			Anm2Animation* animation = anm2_animation_from_reference(self->anm2, self->reference);

			switch (swapItemReference.itemType)
			{
				case ANM2_LAYER:
				{
					s32 indexA = INDEX_NONE;
					s32 indexB = INDEX_NONE;

					for (const auto& [index, id] : self->anm2->layerMap)
					{
						if (id == self->reference->itemID) 
							indexA = index;
						else if (id == swapItemReference.itemID) 
							indexB = index;
					}

					if ((indexA != INDEX_NONE) && (indexB != INDEX_NONE))
						std::swap(self->anm2->layerMap[indexA], self->anm2->layerMap[indexB]);
					break;
				}
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
	};

	std::function<void(Anm2Reference, s32&)> timeline_item_frames = [&](Anm2Reference reference, s32& index)
	{
		Anm2Item* item = anm2_item_from_reference(self->anm2, &reference);
		Anm2Type& type = reference.itemType;

		ImGui::PushID(index);

		ImDrawList* drawList = ImGui::GetWindowDrawList();

		f32 viewWidth = ImGui::GetWindowContentRegionMax().x - ImGui::GetWindowContentRegionMin().x;
		
		ImguiItem imguiItemFrames = IMGUI_TIMELINE_ITEM_FRAMES_CHILD;
		imguiItemFrames.size = actualFramesSize.x > framesSize.x ? actualFramesSize : framesSize;

		_imgui_item_begin_child(imguiItemFrames);
		
		ImVec2 startPos  = ImGui::GetCursorPos();      
		ImVec2 cursorPos = ImGui::GetCursorScreenPos();

		if (_imgui_is_window_hovered())
		{
			hoverReference = reference;
			hoverReference.frameIndex = anm2_frame_index_from_time(self->anm2, reference, frameTime);

			if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
			{
				*self->reference = reference;
				self->clipboard->location = hoverReference;
			}
		}

		s32 start = (s32)std::floor(scroll.x / frameSize.x) - 1;
		if (start < 0) start = 0;

		s32 end = (s32)std::ceil((scroll.x + viewWidth) / frameSize.x) + 1;
		if (end > ANM2_FRAME_NUM_MAX) end = ANM2_FRAME_NUM_MAX;

		for (s32 i = start; i < end; i++)
		{
			bool isMultiple = (i % IMGUI_TIMELINE_FRAME_MULTIPLE) == 0;
			ImU32 bgColor = isMultiple ? frameMultipleColor : frameColor;

			f32 startX = cursorPos.x + i * frameSize.x;
			f32 endX = startX + frameSize.x;
			ImVec2 startPosition(startX, cursorPos.y);
			ImVec2 endPosition(endX, cursorPos.y + frameSize.y);

			drawList->AddRectFilled(startPosition, endPosition, bgColor);
			drawList->AddImage(self->resources->atlas.id, startPosition, endPosition, ATLAS_UV_ARGS(TEXTURE_FRAME_ALT));
		}

		ImGui::SetCursorPos(startPos);

		std::function<void(s32, Anm2Frame&)> timeline_item_frame = [&](s32 i, Anm2Frame& frame)
		{
			ImGui::PushID(i);
			reference.frameIndex = i;
			ImguiItem frameButton = *IMGUI_TIMELINE_FRAMES[type];
			ImVec2 framePos = ImGui::GetCursorPos();
			frameButton.texture = frame.isInterpolated ? TEXTURE_CIRCLE : TEXTURE_SQUARE;
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
					imgui_undo_stack_push(self, IMGUI_ACTION_TRIGGER_MOVE);

				if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceNoPreviewTooltip))
				{
					frame.atFrame = frameTime;
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
						imgui_undo_stack_push(self, IMGUI_ACTION_FRAME_SWAP);

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

			ImGui::PopID();
		};

		for (auto [i, frame] : std::views::enumerate(item->frames))
			timeline_item_frame(i, frame);

		_imgui_item_end_child(); // itemFramesChild

		ImGui::PopID();
		
		index++;
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

		timeline_item_frames(Anm2Reference(animationID, ANM2_ROOT), index);

		for (auto& [i, id] : std::ranges::reverse_view(self->anm2->layerMap))
			timeline_item_frames(Anm2Reference(animationID, ANM2_LAYER, id), index);

		for (auto & [id, null] : animation->nullAnimations)
			timeline_item_frames(Anm2Reference(animationID, ANM2_NULL, id), index);

		timeline_item_frames(Anm2Reference(animationID, ANM2_TRIGGERS), index);

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
	

	Anm2Frame* frame = anm2_frame_from_reference(self->anm2, self->reference);
	
	_imgui_item_begin_child(IMGUI_TIMELINE_FOOTER_ITEM_CHILD);

	if(_imgui_item_button(self, IMGUI_TIMELINE_ADD_ITEM))
		ImGui::OpenPopup(IMGUI_TIMELINE_ADD_ITEM.popup.c_str());
	
	if (ImGui::BeginPopup(IMGUI_TIMELINE_ADD_ITEM.popup.c_str()))
	{
		if (_imgui_item_selectable(self, IMGUI_TIMELINE_ADD_ITEM_LAYER))
			anm2_layer_add(self->anm2);

		if (_imgui_item_selectable(self, IMGUI_TIMELINE_ADD_ITEM_NULL))
			anm2_null_add(self->anm2);

		ImGui::EndPopup();
	}

	ImGui::SameLine();

	if (_imgui_item_button(self, IMGUI_TIMELINE_REMOVE_ITEM))
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
	
	_imgui_item_end_child(); //IMGUI_TIMELINE_FOOTER_ITEM_CHILD
	
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
	ImGui::SameLine();
	ImGui::PopStyleVar();
	
	_imgui_item_begin_child(IMGUI_TIMELINE_FOOTER_OPTIONS_CHILD);
	
	ImguiItem playPauseItem = self->preview->isPlaying ? IMGUI_TIMELINE_PAUSE : IMGUI_TIMELINE_PLAY;
	if (_imgui_item_button(self, playPauseItem))
		self->preview->isPlaying = !self->preview->isPlaying;

	ImGui::SameLine();
	
	if (_imgui_item_button(self, IMGUI_TIMELINE_ADD_FRAME))
		anm2_frame_add(self->anm2, self->reference, (s32)self->preview->time);

	ImGui::SameLine();

	if(_imgui_item_button(self, IMGUI_TIMELINE_REMOVE_FRAME))
	{
		if (anm2_frame_from_reference(self->anm2, self->reference))
		{
			Anm2Item* item = anm2_item_from_reference(self->anm2, self->reference);
			item->frames.erase(item->frames.begin() + self->reference->frameIndex);
			anm2_reference_frame_clear(self->reference);
		}
	}

	ImGui::SameLine();

	if (_imgui_item_button(self, IMGUI_TIMELINE_BAKE))
	{
		if (frame)
			ImGui::OpenPopup(IMGUI_TIMELINE_BAKE.popup.c_str());
		else
			ImGui::CloseCurrentPopup();
	}

	if (ImGui::BeginPopupModal(IMGUI_TIMELINE_BAKE.popup.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		static s32 interval = 1;
		static bool isRoundScale = true;
		static bool isRoundRotation = true;

		if (frame)
		{
			_imgui_item_begin_child(IMGUI_TIMELINE_BAKE_CHILD);

			ImguiItem bakeIntervalItem = IMGUI_TIMELINE_BAKE_INTERVAL;
			bakeIntervalItem.max = frame->delay;

			_imgui_item_inputint(self, bakeIntervalItem, interval);

			_imgui_item_checkbox(self, IMGUI_TIMELINE_BAKE_ROUND_SCALE, isRoundScale);
			_imgui_item_checkbox(self, IMGUI_TIMELINE_BAKE_ROUND_ROTATION, isRoundRotation);

			if (_imgui_item_button(self, IMGUI_TIMELINE_BAKE_CONFIRM))
			{
				anm2_frame_bake(self->anm2, self->reference, interval, isRoundScale, isRoundRotation);
				ImGui::CloseCurrentPopup();
			}

			ImGui::SameLine();
						
			if (_imgui_item_button(self, IMGUI_TIMELINE_BAKE_CANCEL))
				ImGui::CloseCurrentPopup();

			_imgui_item_end_child(); //IMGUI_TIMELINE_BAKE_CHILD)
				
			ImGui::EndPopup();
		}
		else
		{
			ImGui::CloseCurrentPopup();
			ImGui::EndPopup();
		}
	}

	ImGui::SameLine();
		
	if (_imgui_item_button(self, IMGUI_TIMELINE_FIT_ANIMATION_LENGTH))
		anm2_animation_length_set(animation);

	ImGui::SameLine();
	_imgui_item_inputint(self, IMGUI_TIMELINE_ANIMATION_LENGTH, animation->frameNum);
	ImGui::SameLine();
	_imgui_item_inputint(self, IMGUI_TIMELINE_FPS, self->anm2->fps);
	ImGui::SameLine();
	_imgui_item_checkbox(self, IMGUI_TIMELINE_LOOP, animation->isLoop);
	ImGui::SameLine();
	_imgui_item_text_inputtext(self, IMGUI_TIMELINE_CREATED_BY, self->anm2->createdBy);

	_imgui_item_end_child(); //IMGUI_TIMELINE_FOOTER_OPTIONS_CHILD
	
	_imgui_item_end(); // IMGUI_TIMELINE

	self->preview->time = std::clamp(self->preview->time, 0.0f, (f32)(animation->frameNum - 1));
}

static void _imgui_taskbar(Imgui* self)
{
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImguiItem taskbarItem = IMGUI_TASKBAR;
	taskbarItem.size = {viewport->Size.x, IMGUI_TASKBAR.size.y};
	ImGui::SetNextWindowPos(viewport->Pos);
	_imgui_item_begin(taskbarItem);

	_imgui_item_selectable(self, IMGUI_TASKBAR_FILE);

	if (ImGui::BeginPopup(IMGUI_TASKBAR_FILE.popup.c_str()))
	{
		_imgui_item_selectable(self, IMGUI_FILE_NEW);
		_imgui_item_selectable(self, IMGUI_FILE_OPEN);
		_imgui_item_selectable(self, IMGUI_FILE_SAVE);
		_imgui_item_selectable(self, IMGUI_FILE_SAVE_AS);
		ImGui::EndPopup();
	}

	ImGui::SameLine();

	_imgui_item_selectable(self, IMGUI_TASKBAR_WIZARD);
	
	if (ImGui::BeginPopup(IMGUI_TASKBAR_WIZARD.popup.c_str()))
	{
		_imgui_item_selectable(self, IMGUI_TASKBAR_WIZARD_GENERATE_ANIMATION_FROM_GRID);
		_imgui_item_selectable(self, IMGUI_TASKBAR_WIZARD_RECORD_GIF_ANIMATION);
		ImGui::EndPopup();
	}
	
	if (ImGui::BeginPopupModal(IMGUI_TASKBAR_WIZARD_GENERATE_ANIMATION_FROM_GRID.popup.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::EndPopup();
	}

	ImGui::SameLine();
	
	_imgui_item_selectable(self, IMGUI_TASKBAR_PLAYBACK);
		
	if (ImGui::BeginPopup(IMGUI_TASKBAR_PLAYBACK.popup.c_str()))
	{
		_imgui_item_checkbox(self, IMGUI_PLAYBACK_ALWAYS_LOOP, self->settings->playbackIsLoop);
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
			vec4 buttonColor = self->settings->tool == (ToolType)i ? ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered] : ImGui::GetStyle().Colors[ImGuiCol_Button];
			ImGui::PushStyleColor(ImGuiCol_Button, buttonColor);

			_imgui_item_atlas_image_button(self, item);

			ImGui::PopStyleColor();
		}
		else
			_imgui_item_coloredit4(self, IMGUI_TOOL_COLOR, self->settings->toolColor);
		
		usedWidth += ImGui::GetItemRectSize().x + ImGui::GetStyle().ItemSpacing.x;
	}

	_imgui_item_end(); // IMGUI_TOOLS
	
	if (self->settings->tool == TOOL_COLOR_PICKER)
	{
		if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
		{
			vec2 mousePos{};
			SDL_GetMouseState(&mousePos.x, &mousePos.y);

			vec4 color{};
			if (_imgui_window_color_from_position_get(self->window, mousePos, &color))
			{
				self->settings->toolColor = color;

    			ImGui::BeginTooltip();
				_imgui_item_colorbutton(self, IMGUI_TOOL_COLOR_PICKER_TOOLTIP_COLOR, color);
				ImGui::EndTooltip();
			}
		}
	}

	if (TOOL_MOUSE_CURSOR_IS_CONSTANT[self->settings->tool])
		SDL_SetCursor(SDL_CreateSystemCursor(TOOL_MOUSE_CURSORS[self->settings->tool]));
}

static void _imgui_animations(Imgui* self)
{
	_imgui_item_begin(IMGUI_ANIMATIONS);

	ImguiItem animationsChild = IMGUI_ANIMATIONS_CHILD;
	animationsChild.size.y = ImGui::GetContentRegionAvail().y - IMGUI_ANIMATIONS_FOOTER_HEIGHT;
	_imgui_item_begin_child(animationsChild);

	std::function<void(s32, Anm2Animation&)> animation_item = [&](s32 id, Anm2Animation& animation)
	{
		ImGui::PushID(id);
		
		ImguiItem animationItem = IMGUI_ANIMATION;
		animationItem.isSelected =  self->reference->animationID == id; 

		if (id == self->anm2->defaultAnimationID)
			animationItem.label = std::format(IMGUI_ANIMATION_DEFAULT_FORMAT, animation.name);
		else
			animationItem.label = animation.name;

		if (_imgui_item_atlas_image_selectable_inputtext(self, animationItem, animation.name, id))
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
					imgui_undo_stack_push(self, IMGUI_ACTION_ANIMATION_SWAP);
					map_swap(self->anm2->animations, sourceID, id);
				}
			}

			ImGui::EndDragDropTarget();
		}
		
		ImGui::PopID();
	};

	for (auto & [id, animation] : self->anm2->animations)
		animation_item(id, animation);

	_imgui_item_end_child(); // animationsChild
	
	Anm2Animation* animation = anm2_animation_from_reference(self->anm2, self->reference);
	
	_imgui_item_begin_child(IMGUI_ANIMATIONS_OPTIONS_CHILD);
	
	if (_imgui_item_button(self, IMGUI_ANIMATION_ADD))
	{
		bool isDefault = (s32)self->anm2->animations.size() == 0; 
		s32 id = anm2_animation_add(self->anm2);
		
		self->reference->animationID = id;

		if (isDefault)
			self->anm2->defaultAnimationID = id;
	}
		
	ImGui::SameLine();

	if (_imgui_item_button(self, IMGUI_ANIMATION_DUPLICATE) && animation)
	{
		s32 id = map_next_id_get(self->anm2->animations);
		self->anm2->animations.insert({id, *animation});
		self->reference->animationID = id;
	}

	ImGui::SameLine();

	_imgui_item_button(self, IMGUI_ANIMATIONS_MERGE);

	if (ImGui::BeginPopupModal(IMGUI_ANIMATIONS_MERGE.popup.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		static s32 mergeType = (s32)ANM2_MERGE_APPEND_FRAMES;
		static bool isDeleteAnimationsAfter = false;
		static std::vector<s32> animationIDs;
		static s32 lastClickedID = ID_NONE;

		const bool isModCtrl  = ImGui::IsKeyDown(IMGUI_INPUT_CTRL);
		const bool isModShift = ImGui::IsKeyDown(IMGUI_INPUT_SHIFT);

		static std::vector<s32> sortedIDs;
		static size_t lastAnimationCount = 0;
		if (self->anm2->animations.size() != lastAnimationCount)
		{
			sortedIDs.clear();
			for (const auto& [id, _] : self->anm2->animations)
				sortedIDs.push_back(id);
			std::sort(sortedIDs.begin(), sortedIDs.end());
			lastAnimationCount = self->anm2->animations.size();
		}

		_imgui_item_begin_child(IMGUI_ANIMATIONS_MERGE_CHILD);

		for (const auto& [id, animation] : self->anm2->animations)
		{
			ImGui::PushID(id);

			ImguiItem animationItem = IMGUI_ANIMATION;
			animationItem.label = animation.name;
			animationItem.isSelected = std::find(animationIDs.begin(), animationIDs.end(), id) != animationIDs.end();

			if (_imgui_item_atlas_image_selectable(self, animationItem))
			{
				if (isModCtrl)
				{
					auto it = std::find(animationIDs.begin(), animationIDs.end(), id);
					if (it != animationIDs.end())
						animationIDs.erase(it);
					else
						animationIDs.push_back(id);

					lastClickedID = id;
				}
				else if (isModShift)
				{
					auto it1 = std::find(sortedIDs.begin(), sortedIDs.end(), lastClickedID);
					auto it2 = std::find(sortedIDs.begin(), sortedIDs.end(), id);
					if (it1 != sortedIDs.end() && it2 != sortedIDs.end())
					{
						auto begin = std::min(it1, it2);
						auto end   = std::max(it1, it2);
						for (auto it = begin; it <= end; ++it)
						{
							if (std::find(animationIDs.begin(), animationIDs.end(), *it) == animationIDs.end())
								animationIDs.push_back(*it);
						}
					}
				}
				else
				{
					animationIDs.clear();
					animationIDs.push_back(id);
					lastClickedID = id;
				}
			}

			ImGui::PopID();
		}

		_imgui_item_end_child(); //IMGUI_ANIMATIONS_MERGE_CHILD
			
		_imgui_item_begin_child(IMGUI_ANIMATIONS_MERGE_ON_CONFLICT_CHILD);
		
		_imgui_item_text(IMGUI_ANIMATIONS_MERGE_ON_CONFLICT);

		_imgui_item_radio_button(self, IMGUI_ANIMATIONS_MERGE_APPEND_FRAMES, mergeType);
		ImGui::SameLine();
		_imgui_item_radio_button(self, IMGUI_ANIMATIONS_MERGE_REPLACE_FRAMES, mergeType);
		_imgui_item_radio_button(self, IMGUI_ANIMATIONS_MERGE_PREPEND_FRAMES, mergeType);
		ImGui::SameLine();
		_imgui_item_radio_button(self, IMGUI_ANIMATIONS_MERGE_IGNORE, mergeType);

		_imgui_item_end_child(); //IMGUI_ANIMATIONS_MERGE_ON_CONFLICT_CHILD
		
		_imgui_item_begin_child(IMGUI_ANIMATIONS_MERGE_OPTIONS_CHILD);

		_imgui_item_checkbox(self, IMGUI_ANIMATIONS_MERGE_DELETE_ANIMATIONS_AFTER, isDeleteAnimationsAfter);

		_imgui_item_end_child(); //IMGUI_ANIMATIONS_MERGE_OPTIONS_CHILD
		
		if (_imgui_item_button(self, IMGUI_ANIMATIONS_MERGE_CONFIRM))
		{
			anm2_animation_merge(self->anm2, self->reference->animationID, animationIDs, (Anm2MergeType)mergeType);

			if (isDeleteAnimationsAfter)
				for (s32 id : animationIDs)
					if (id != self->reference->animationID)
						self->anm2->animations.erase(id);

			animationIDs.clear();
			ImGui::CloseCurrentPopup();
		}

		ImGui::SameLine();

		if (_imgui_item_button(self, IMGUI_ANIMATIONS_MERGE_CANCEL))
		{
			animationIDs.clear();
			ImGui::CloseCurrentPopup();
		}

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
		self->anm2->defaultAnimationID = self->reference->animationID; 

	_imgui_item_end_child(); // IMGUI_ANIMATIONS_OPTIONS_CHILD)
	_imgui_item_end();
}

static void _imgui_events(Imgui* self)
{
	static s32 selectedID = ID_NONE;
	
	_imgui_item_begin(IMGUI_EVENTS);
	
	ImguiItem eventsChild = IMGUI_EVENTS_CHILD;
	eventsChild.size.y = ImGui::GetContentRegionAvail().y - IMGUI_EVENTS_FOOTER_HEIGHT;
	_imgui_item_begin_child(eventsChild);

	std::function<void(s32, Anm2Event&)> event_item = [&](s32 id, Anm2Event& event)
	{
		ImGui::PushID(id);
		
		ImguiItem eventItem = IMGUI_EVENT;
		eventItem.label = event.name;
		eventItem.isSelected = id == selectedID;

		if (_imgui_item_atlas_image_selectable_inputtext(self, eventItem, event.name, id))
			selectedID = id;

		ImGui::PopID();
	};

	for (auto& [id, event] : self->anm2->events)
		event_item(id, event);

	_imgui_item_end_child(); // eventsChild
	
	_imgui_item_begin_child(IMGUI_EVENTS_OPTIONS_CHILD);
	
	if (_imgui_item_button(self, IMGUI_EVENT_ADD))
	{
		s32 id = map_next_id_get(self->anm2->events);
		self->anm2->events[id] = Anm2Event{}; 
		selectedID = id;
	}

	ImGui::SameLine();

	if (_imgui_item_button(self, IMGUI_EVENT_REMOVE_UNUSED))
	{
		std::unordered_set<s32> usedEventIDs;

		for (auto& [id, animation] : self->anm2->animations)
			for (auto& trigger : animation.triggers.frames)
				if (trigger.eventID != ID_NONE)
					usedEventIDs.insert(trigger.eventID);

		for (auto it = self->anm2->events.begin(); it != self->anm2->events.end(); )
		{
			if (!usedEventIDs.count(it->first))
				it = self->anm2->events.erase(it);
			else
				it++;
		}
	}
	
	_imgui_item_end_child(); // IMGUI_ANIMATIONS_OPTIONS_CHILD)
	_imgui_item_end();
}

static void _imgui_spritesheets(Imgui* self)
{
	static std::unordered_map<s32, bool> isSelectedIDs;
	static s32 highlightedID = ID_NONE;

	_imgui_item_begin(IMGUI_SPRITESHEETS);

	ImguiItem spritesheetsChild = IMGUI_SPRITESHEETS_CHILD;
	spritesheetsChild.size.y = ImGui::GetContentRegionAvail().y - IMGUI_SPRITESHEETS_FOOTER_HEIGHT;

	_imgui_item_begin_child(spritesheetsChild);
	
	std::function<void(s32, Anm2Spritesheet&)> spritesheet_item = [&](s32 id, Anm2Spritesheet& spritesheet)
	{
		ImGui::PushID(id);
		
		ImguiItem spritesheetItem = IMGUI_SPRITESHEET;
		Texture* texture = &self->resources->textures[id];

		spritesheetItem.label = std::format(IMGUI_SPRITESHEET_FORMAT, id, spritesheet.path);
		spritesheetItem.isSelected = id == highlightedID;

		_imgui_item_begin_child(IMGUI_SPRITESHEET_CHILD);

		if (_imgui_item_atlas_image_checkbox_selectable(self, spritesheetItem, isSelectedIDs[id]))
			highlightedID = id;

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
			
		_imgui_item_end_child(); // IMGUI_SPRITESHEET_CHILD

		ImGui::PopID();
	};
	
	for (auto [id, spritesheet] : self->anm2->spritesheets)
		spritesheet_item(id, spritesheet);

	_imgui_item_end_child(); // spritesheetsChild
		
	_imgui_item_begin_child(IMGUI_SPRITESHEETS_OPTIONS_CHILD);
	
	_imgui_item_button(self, IMGUI_SPRITESHEETS_ADD);
	
	ImGui::SameLine();

	if (_imgui_item_button(self, IMGUI_SPRITESHEETS_RELOAD))
	{
		for (auto& [id, isSelected] : isSelectedIDs)
		{
			if (isSelected)
			{
				std::filesystem::path workingPath = std::filesystem::current_path();
				working_directory_from_file_set(self->anm2->path);
				resources_texture_init(self->resources, self->anm2->spritesheets[id].path, id);
				std::filesystem::current_path(workingPath);
			}
		}
	}

	ImGui::SameLine();
	
	if (_imgui_item_button(self, IMGUI_SPRITESHEETS_REPLACE))
	{
		if (highlightedID != ID_NONE)
		{
			self->dialog->replaceID = highlightedID;
			dialog_png_replace(self->dialog);
		}
	}

	ImGui::SameLine();

	if (_imgui_item_button(self, IMGUI_SPRITESHEETS_REMOVE_UNUSED))
	{
		std::unordered_set<s32> usedSpritesheetIDs;

		for (auto& [layerID, layer] : self->anm2->layers)
			if (layer.spritesheetID != ID_NONE)
				usedSpritesheetIDs.insert(layer.spritesheetID);

		for (auto it = self->anm2->spritesheets.begin(); it != self->anm2->spritesheets.end(); )
		{
			if (!usedSpritesheetIDs.count(it->first))
			{
				it = self->anm2->spritesheets.erase(it);
				texture_free(&self->resources->textures[it->first]);
			}
			else
				it++;
		}
	}

	if (_imgui_item_button(self, IMGUI_SPRITESHEETS_SELECT_ALL))
		for (auto& [id, _] : self->anm2->spritesheets)
			isSelectedIDs[id] = true;

	ImGui::SameLine();

	if (_imgui_item_button(self, IMGUI_SPRITESHEETS_SELECT_NONE))
		for (auto& [id, _] : self->anm2->spritesheets)
			isSelectedIDs[id] = false;

	ImGui::SameLine();
	
	if (_imgui_item_button(self, IMGUI_SPRITESHEETS_SAVE))
	{
		for (auto& [id, isSelected] : isSelectedIDs)
		{
			if (isSelected)
			{
				std::filesystem::path workingPath = std::filesystem::current_path();
				working_directory_from_file_set(self->anm2->path);
				Anm2Spritesheet* spritesheet = &self->anm2->spritesheets[id];
				Texture* texture = &self->resources->textures[id];
				texture_from_gl_write(texture, spritesheet->path);
				imgui_message_queue_push(self, std::format(IMGUI_ACTION_SPRITESHEET_SAVE_FORMAT, id, spritesheet->path));
				std::filesystem::current_path(workingPath);
			}
		}
	}

	if (_imgui_is_no_click_on_item())
		highlightedID = ID_NONE;

	_imgui_item_end_child(); //IMGUI_SPRITESHEETS_OPTIONS_CHILD
	_imgui_item_end();
}

static void _imgui_animation_preview(Imgui* self)
{
	static bool isPreviewHover = false;
	static vec2 mousePos{};
	static vec2 previewPos{};
	static ImVec2 previewScreenPos{};
	static vec2& pan = self->settings->previewPan;
	static f32& zoom = self->settings->previewZoom;
	static vec2& size = self->preview->canvas.size;

	std::string mousePositionString = std::format(IMGUI_POSITION_FORMAT, (s32)mousePos.x, (s32)mousePos.y);
	
	_imgui_item_begin(IMGUI_ANIMATION_PREVIEW);

	_imgui_item_begin_child(IMGUI_ANIMATION_PREVIEW_GRID_SETTINGS);
	_imgui_item_checkbox(self, IMGUI_ANIMATION_PREVIEW_GRID, self->settings->previewIsGrid);
	ImGui::SameLine();
	_imgui_item_coloredit4(self, IMGUI_ANIMATION_PREVIEW_GRID_COLOR, self->settings->previewGridColor);
	_imgui_item_inputint2(self, IMGUI_ANIMATION_PREVIEW_GRID_SIZE, self->settings->previewGridSize);
	_imgui_item_inputint2(self, IMGUI_ANIMATION_PREVIEW_GRID_OFFSET, self->settings->previewGridOffset);
	_imgui_item_end_child();
	
	ImGui::SameLine();
	
	_imgui_item_begin_child(IMGUI_ANIMATION_PREVIEW_VIEW_SETTINGS);
	_imgui_item_dragfloat(self, IMGUI_ANIMATION_PREVIEW_ZOOM, zoom);
	if (_imgui_item_button(self, IMGUI_ANIMATION_PREVIEW_CENTER_VIEW)) pan = vec2();
	ImGui::Text(mousePositionString.c_str());
	_imgui_item_end_child();

	ImGui::SameLine();
	
	_imgui_item_begin_child(IMGUI_ANIMATION_PREVIEW_BACKGROUND_SETTINGS);
	_imgui_item_coloredit4(self, IMGUI_ANIMATION_PREVIEW_BACKGROUND_COLOR, self->settings->previewBackgroundColor);
	
	std::vector<std::string> animationStrings;
	std::vector<const char*> animationLabels;
	std::vector<s32> animationIDs;

	animationStrings.reserve(self->anm2->animations.size() + 1);
	animationIDs.reserve(self->anm2->animations.size() + 1);
	animationLabels.reserve(self->anm2->animations.size() + 1);

	animationIDs.push_back(ID_NONE);
	animationStrings.push_back(IMGUI_EVENT_NONE);
	animationLabels.push_back(animationStrings.back().c_str());
	
	for (auto & [id, animation] : self->anm2->animations) 
	{
		animationIDs.push_back(id);
		animationStrings.push_back(animation.name);
		animationLabels.push_back(animationStrings.back().c_str());
	}

	s32 selectedAnimationID = std::find(animationIDs.begin(), animationIDs.end(), self->preview->animationOverlayID) - animationIDs.begin();

	if (_imgui_item_combo(self, IMGUI_ANIMATION_PREVIEW_OVERLAY, &selectedAnimationID, animationLabels.data(), (s32)animationLabels.size()))
		self->preview->animationOverlayID = animationIDs[selectedAnimationID];

	_imgui_item_dragfloat(self, IMGUI_ANIMATION_PREVIEW_OVERLAY_TRANSPARENCY, self->settings->previewOverlayTransparency);
	_imgui_item_end_child();

	ImGui::SameLine();

	_imgui_item_begin_child(IMGUI_ANIMATION_PREVIEW_HELPER_SETTINGS);
	_imgui_item_checkbox(self, IMGUI_ANIMATION_PREVIEW_AXIS, self->settings->previewIsAxis);
	ImGui::SameLine();
	_imgui_item_coloredit4(self, IMGUI_ANIMATION_PREVIEW_AXIS_COLOR, self->settings->previewAxisColor);
	_imgui_item_checkbox(self, IMGUI_ANIMATION_PREVIEW_ROOT_TRANSFORM, self->settings->previewIsRootTransform);
	_imgui_item_checkbox(self, IMGUI_ANIMATION_PREVIEW_SHOW_PIVOT, self->settings->previewIsShowPivot);
	ImGui::SameLine();
	_imgui_item_checkbox(self, IMGUI_ANIMATION_PREVIEW_BORDER, self->settings->previewIsBorder);
	_imgui_item_end_child();

	previewPos = vec2(ImGui::GetCursorPos());
	
	size = ImGui::GetContentRegionAvail();
	ImGui::Image(self->preview->canvas.texture, size);
	
	if (!ImGui::IsItemHovered())
	{
		if (isPreviewHover)
		{
			SDL_SetCursor(SDL_CreateSystemCursor(MOUSE_CURSOR_DEFAULT));
			_imgui_keyboard_navigation_set(true);
			isPreviewHover = false;
		}
		_imgui_item_end();
		return;
	}

	isPreviewHover = true;

	_imgui_keyboard_navigation_set(false);
	
	mousePos = (vec2((ImGui::GetMousePos()) - (ImGui::GetWindowPos() + previewPos)) - (size * 0.5f) - pan) / PERCENT_TO_UNIT(zoom);

	ToolType tool = self->settings->tool;
	bool isLeft  = ImGui::IsKeyPressed(IMGUI_INPUT_LEFT);
	bool isRight = ImGui::IsKeyPressed(IMGUI_INPUT_RIGHT);
	bool isUp    = ImGui::IsKeyPressed(IMGUI_INPUT_UP);
	bool isDown  = ImGui::IsKeyPressed(IMGUI_INPUT_DOWN);
	bool isMod   = ImGui::IsKeyDown(IMGUI_INPUT_SHIFT);
	bool isMouseClick = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
	bool isMouseDown = ImGui::IsMouseDown(ImGuiMouseButton_Left);
	bool isMouseMiddleDown = ImGui::IsMouseDown(ImGuiMouseButton_Middle);
	ImVec2 mouseDelta = ImGui::GetIO().MouseDelta;
	f32 mouseWheel = ImGui::GetIO().MouseWheel;

	SDL_SetCursor(SDL_CreateSystemCursor(TOOL_MOUSE_CURSORS[tool]));

	if (self->settings->tool == TOOL_MOVE || self->settings->tool == TOOL_SCALE || self->settings->tool == TOOL_ROTATE)
		if (isMouseClick || isLeft || isRight || isUp || isDown)
			imgui_undo_stack_push(self, IMGUI_ACTION_FRAME_TRANSFORM);
	
	if ((self->settings->tool == TOOL_PAN && isMouseDown) || isMouseMiddleDown)
		pan += vec2(mouseDelta.x, mouseDelta.y);

	Anm2Frame* frame = nullptr;
	
	if (self->reference->itemType != ANM2_TRIGGERS) 
		frame = anm2_frame_from_reference(self->anm2, self->reference);

	if (frame)
	{
		f32 step = isMod ? TOOL_STEP_MOD : TOOL_STEP;
		
		switch (tool)
		{
			case TOOL_MOVE:
				if (isMouseDown) 
					frame->position = vec2(mousePos);
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
					frame->scale += vec2(mouseDelta.x, mouseDelta.y);
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
		f32 delta = (mouseWheel > 0 || ImGui::IsKeyPressed(IMGUI_INPUT_ZOOM_IN)) ? CANVAS_ZOOM_STEP : -CANVAS_ZOOM_STEP;
		delta = ImGui::IsKeyDown(IMGUI_INPUT_SHIFT) ? delta * CANVAS_ZOOM_MOD : delta;
		zoom = ROUND_NEAREST_MULTIPLE(zoom + delta, CANVAS_ZOOM_STEP);
		zoom = std::clamp(zoom, CANVAS_ZOOM_MIN, CANVAS_ZOOM_MAX);
	}

	_imgui_item_end();
}

static void _imgui_spritesheet_editor(Imgui* self)
{
	static bool isEditorHover = false;
	static vec2 mousePos = {0, 0};
	vec2& pan = self->settings->editorPan;
	f32& zoom = self->settings->editorZoom;
	vec2& size = self->editor->canvas.size;
	ivec2& gridSize = self->settings->editorGridSize;
	
	std::string mousePositionString = std::format(IMGUI_POSITION_FORMAT, (s32)mousePos.x, (s32)mousePos.y);

	_imgui_item_begin(IMGUI_SPRITESHEET_EDITOR);
	
	_imgui_item_begin_child(IMGUI_SPRITESHEET_EDITOR_GRID_SETTINGS);
	_imgui_item_checkbox(self, IMGUI_SPRITESHEET_EDITOR_GRID, self->settings->editorIsGrid);
	ImGui::SameLine();
	_imgui_item_checkbox(self, IMGUI_SPRITESHEET_EDITOR_GRID_SNAP, self->settings->editorIsGridSnap);
	ImGui::SameLine();
	_imgui_item_coloredit4(self, IMGUI_SPRITESHEET_EDITOR_GRID_COLOR, self->settings->editorGridColor);
	_imgui_item_inputint2(self, IMGUI_SPRITESHEET_EDITOR_GRID_SIZE, self->settings->editorGridSize);
	_imgui_item_inputint2(self, IMGUI_SPRITESHEET_EDITOR_GRID_OFFSET, self->settings->editorGridOffset);
	_imgui_item_end_child();
	
	ImGui::SameLine();
	
	_imgui_item_begin_child(IMGUI_SPRITESHEET_EDITOR_VIEW_SETTINGS);
	_imgui_item_dragfloat(self, IMGUI_SPRITESHEET_EDITOR_ZOOM, self->settings->editorZoom);
	if (_imgui_item_button(self, IMGUI_SPRITESHEET_EDITOR_CENTER_VIEW)) pan = vec2();
	ImGui::Text(mousePositionString.c_str());
	_imgui_item_end_child();
	
	ImGui::SameLine();
	
	_imgui_item_begin_child(IMGUI_SPRITESHEET_EDITOR_BACKGROUND_SETTINGS);
	_imgui_item_coloredit4(self, IMGUI_SPRITESHEET_EDITOR_BACKGROUND_COLOR, self->settings->editorBackgroundColor);
	_imgui_item_checkbox(self, IMGUI_SPRITESHEET_EDITOR_BORDER, self->settings->editorIsBorder);
	_imgui_item_end_child();
	
	ImVec2 editorPos = ImGui::GetCursorPos();
	size = ImGui::GetContentRegionAvail();
	ImGui::Image(self->editor->canvas.texture, size);
	
	if (!ImGui::IsItemHovered())
	{
		if (isEditorHover)
		{
			SDL_SetCursor(SDL_CreateSystemCursor(MOUSE_CURSOR_DEFAULT));
			_imgui_keyboard_navigation_set(true);
			isEditorHover = false;
		}
		_imgui_item_end();
		return;
	}

	isEditorHover = true;

	_imgui_keyboard_navigation_set(false);
	
	mousePos = (vec2((ImGui::GetMousePos()) - (ImGui::GetWindowPos() + editorPos)) - pan) / PERCENT_TO_UNIT(zoom);

	bool isMouseClick = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
	bool isMouseDown = ImGui::IsMouseDown(ImGuiMouseButton_Left);
	bool isMouseMiddleDown = ImGui::IsMouseDown(ImGuiMouseButton_Middle);
	f32 mouseWheel = ImGui::GetIO().MouseWheel;
	ImVec2 mouseDelta = ImGui::GetIO().MouseDelta;
	
	SDL_SetCursor(SDL_CreateSystemCursor(TOOL_MOUSE_CURSORS[self->settings->tool]));

	if ((self->settings->tool == TOOL_PAN && isMouseDown) || isMouseMiddleDown)
		pan += vec2(mouseDelta.x, mouseDelta.y);

	Anm2Frame* frame = nullptr;
	if (self->reference->itemType == ANM2_LAYER) 
		frame = anm2_frame_from_reference(self->anm2, self->reference);

	Texture* texture = map_find(self->resources->textures, self->editor->spritesheetID);

	if (texture)
	{
		vec2 position = mousePos;
		vec4 color = self->settings->tool == TOOL_ERASE ? COLOR_TRANSPARENT : self->settings->toolColor;

		switch (self->settings->tool)
		{
			case TOOL_CROP:
				if (!frame) break;

				if (self->settings->editorIsGridSnap)
					position = {(s32)(position.x / gridSize.x) * gridSize.x, (s32)(position.y / gridSize.y) * gridSize.y};

				if (isMouseClick)
				{
					imgui_undo_stack_push(self, IMGUI_ACTION_FRAME_CROP);
					frame->crop = position;
					frame->size = ivec2(0,0);
				}
				else if (isMouseDown)
					frame->size = position - frame->crop;
				break;
			case TOOL_DRAW:
			case TOOL_ERASE:
				if (isMouseDown)
					texture_pixel_set(texture, mousePos, color);
				break;
			default:
				break;
		}
	}
			
	if (mouseWheel != 0 || ImGui::IsKeyPressed(IMGUI_INPUT_ZOOM_IN) || ImGui::IsKeyPressed(IMGUI_INPUT_ZOOM_OUT))
	{
		f32 delta = (mouseWheel > 0 || ImGui::IsKeyPressed(IMGUI_INPUT_ZOOM_IN)) ? CANVAS_ZOOM_STEP : -CANVAS_ZOOM_STEP;
		delta = ImGui::IsKeyDown(IMGUI_INPUT_SHIFT) ? delta * CANVAS_ZOOM_MOD : delta;
		self->settings->editorZoom = ROUND_NEAREST_MULTIPLE(self->settings->editorZoom + delta, CANVAS_ZOOM_STEP);
		self->settings->editorZoom = std::clamp(self->settings->editorZoom, CANVAS_ZOOM_MIN, CANVAS_ZOOM_MAX);
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
		_imgui_item_dragfloat2(self, IMGUI_FRAME_PROPERTIES_POSITION, frame->position);
		
		if (type == ANM2_LAYER)
		{
			_imgui_item_dragfloat2(self, IMGUI_FRAME_PROPERTIES_CROP, frame->crop);
			_imgui_item_dragfloat2(self, IMGUI_FRAME_PROPERTIES_SIZE, frame->size);
			_imgui_item_dragfloat2(self, IMGUI_FRAME_PROPERTIES_PIVOT, frame->pivot);
		}

		_imgui_item_dragfloat2(self, IMGUI_FRAME_PROPERTIES_SCALE, frame->scale);
		_imgui_item_dragfloat(self, IMGUI_FRAME_PROPERTIES_ROTATION, frame->rotation);
		_imgui_item_inputint(self, IMGUI_FRAME_PROPERTIES_DURATION, frame->delay);
		_imgui_item_coloredit4(self, IMGUI_FRAME_PROPERTIES_TINT, frame->tintRGBA);
		_imgui_item_coloredit3(self, IMGUI_FRAME_PROPERTIES_COLOR_OFFSET, frame->offsetRGB);
		
		if (_imgui_item_button(self, IMGUI_FRAME_PROPERTIES_FLIP_X))
			frame->scale.x = -frame->scale.x;
		ImGui::SameLine();
		if (_imgui_item_button(self, IMGUI_FRAME_PROPERTIES_FLIP_Y))
			frame->scale.y = -frame->scale.y;

		_imgui_item_checkbox(self, IMGUI_FRAME_PROPERTIES_VISIBLE, frame->isVisible);
		ImGui::SameLine();
		_imgui_item_checkbox(self, IMGUI_FRAME_PROPERTIES_INTERPOLATED, frame->isInterpolated);
			
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
			eventStrings.push_back(event.name);
			eventLabels.push_back(eventStrings.back().c_str());
		}

		s32 selectedEventIndex = std::find(eventIDs.begin(), eventIDs.end(), frame->eventID) - eventIDs.begin();

		if (_imgui_item_combo(self, IMGUI_FRAME_PROPERTIES_EVENT, &selectedEventIndex, eventLabels.data(), (s32)eventLabels.size()))
			frame->eventID = eventIDs[selectedEventIndex];
				
		_imgui_item_inputint(self, IMGUI_FRAME_PROPERTIES_AT_FRAME, frame->atFrame);
		frame->atFrame = std::clamp(frame->atFrame, 0, animation->frameNum - 1);
	}

	_imgui_item_end();
}

static void _imgui_messages(Imgui* self)
{
    ImGuiIO& io = ImGui::GetIO();
    ImGuiStyle& style = ImGui::GetStyle();
	ImVec4 borderColor = style.Colors[ImGuiCol_Border];
	ImVec4 textColor   = style.Colors[ImGuiCol_Text];

	ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar 		 |
            				 ImGuiWindowFlags_NoResize 			 |
            				 ImGuiWindowFlags_NoMove 			 |
            				 ImGuiWindowFlags_NoScrollbar 		 |
            				 ImGuiWindowFlags_NoSavedSettings 	 |
            				 ImGuiWindowFlags_AlwaysAutoResize   |
            				 ImGuiWindowFlags_NoFocusOnAppearing |
            				 ImGuiWindowFlags_NoNav 			 |
            				 ImGuiWindowFlags_NoInputs;

    ImVec2 position = {io.DisplaySize.x - IMGUI_MESSAGE_PADDING, io.DisplaySize.y - IMGUI_MESSAGE_PADDING};
 
	for (s32 i = (s32)self->messageQueue.size() - 1; i >= 0; --i)
    {
        ImguiMessage& message = self->messageQueue[i];
		f32 lifetime = message.timeRemaining / IMGUI_MESSAGE_DURATION;
     	borderColor.w = lifetime;
		textColor.w = lifetime;

		message.timeRemaining -= io.DeltaTime;

        if (message.timeRemaining <= 0.0f) 
		{ 
			self->messageQueue.erase(self->messageQueue.begin() + i); 
			continue; 
		}
	    
        ImGui::SetNextWindowPos(position, ImGuiCond_Always, {1.0f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_Border, borderColor);
        ImGui::PushStyleColor(ImGuiCol_Text, textColor);
        ImGui::SetNextWindowBgAlpha(lifetime);

		ImGui::Begin(std::format(IMGUI_MESSAGE_FORMAT, i).c_str(), nullptr, flags);
        ImGui::TextUnformatted(message.text.c_str());
		ImVec2 windowSize = ImGui::GetWindowSize();
        ImGui::End();

        ImGui::PopStyleColor(2);

        position.y -= windowSize.y + IMGUI_MESSAGE_PADDING;
    }

	if (self->dialog->isJustSelected)
	{
		switch (self->dialog->lastType)
		{
			case DIALOG_ANM2_OPEN:
				imgui_message_queue_push(self, std::format(IMGUI_ACTION_FILE_OPEN_FORMAT, self->dialog->lastPath));
				break;
			case DIALOG_ANM2_SAVE:
				imgui_message_queue_push(self, std::format(IMGUI_ACTION_FILE_SAVE_FORMAT, self->dialog->lastPath));
				break;
			default:
				break;
		}

		dialog_reset(self->dialog);
	}
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
		ImGui::OpenPopup(IMGUI_CONTEXT_MENU.popup.c_str());

	if (ImGui::BeginPopup(IMGUI_CONTEXT_MENU.popup.c_str()))
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

void imgui_update(Imgui* self)
{
	ImGui_ImplSDL3_NewFrame();
	ImGui_ImplOpenGL3_NewFrame();
	ImGui::NewFrame();

	_imgui_taskbar(self);
	_imgui_dock(self);
	_imgui_messages(self);
	_imgui_persistent(self);

	self->isHotkeysEnabled = !self->isRename && !self->isChangeValue;

	if (self->isHotkeysEnabled)
	{
		for (const auto& hotkey : imgui_hotkey_registry())
		{
			if (ImGui::IsKeyChordPressed(hotkey.chord))
			{
				if (hotkey.is_focus_window()) continue;
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
				if (ImGui::IsPopupOpen(IMGUI_EXIT_CONFIRMATION.popup.c_str()))
				{
					self->isQuit = true;
					break;
				}
				ImGui::OpenPopup(IMGUI_EXIT_CONFIRMATION.popup.c_str());
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