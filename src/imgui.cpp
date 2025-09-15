#include "imgui.h"

static bool _imgui_chord_pressed(ImGuiKeyChord chord)
{
    if (chord == IMGUI_CHORD_NONE) return false;
    ImGuiKey key = (ImGuiKey)(chord & ~ImGuiMod_Mask_);
    if (key == ImGuiKey_None) return false;
    if (key < ImGuiKey_NamedKey_BEGIN || key >= ImGuiKey_NamedKey_END) return false;

    return ImGui::IsKeyChordPressed(chord);
}

static const char* _imgui_f32_format_get(const ImguiItem& item, f32& value)
{
	if (item.isEmptyFormat) return "";
	return f32_format_get(value);
}

static const char* _imgui_vec2_format_get(const ImguiItem& item, vec2& value)
{
	if (item.isEmptyFormat) return "";
	return vec2_format_get(value);
}

static bool _imgui_window_color_from_position_get(SDL_Window* self, const vec2& position, vec4& color)
{
    ImGuiIO& io = ImGui::GetIO();
	ivec2 fbPosition = {(s32)(position.x * io.DisplayFramebufferScale.x), (s32)(position.y * io.DisplayFramebufferScale.y)};
	ivec2 size{};
    SDL_GetWindowSizeInPixels(self, &size.x, &size.y);
 
	if (fbPosition.x < 0 || fbPosition.y < 0 || fbPosition.x >= size.x || fbPosition.y >= size.y) return false;

    uint8_t rgba[4];

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(fbPosition.x, size.y - 1 - fbPosition.y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, rgba);

    color = vec4(U8_TO_FLOAT(rgba[0]), U8_TO_FLOAT(rgba[1]), U8_TO_FLOAT(rgba[2]), U8_TO_FLOAT(rgba[3]));

    return true;
}

static void _imgui_anm2_open(Imgui* self, const std::string& path)
{
	imgui_anm2_new(self);

	if (anm2_deserialize(self->anm2, path))
	{
		window_title_from_path_set(self->window, path);
		imgui_log_push(self, std::format(IMGUI_LOG_FILE_OPEN_FORMAT, path));
	}
	else
		imgui_log_push(self, std::format(IMGUI_LOG_FILE_OPEN_FORMAT, path));
}

static void _imgui_spritesheet_add(Imgui* self, const std::string& path)
{
	if (self->anm2->path.empty())
	{
		imgui_log_push(self, IMGUI_LOG_NO_ANM2_PATH);
		return;
	}

	imgui_snapshot(self, IMGUI_ACTION_ADD_SPRITESHEET);

	std::filesystem::path workingPath = std::filesystem::current_path();
	std::string spritesheetPath = path;
	std::string anm2WorkingPath = working_directory_from_file_set(self->anm2->path);
	spritesheetPath = std::filesystem::relative(path, anm2WorkingPath).string();
	s32 id = map_next_id_get(self->anm2->spritesheets);
	self->anm2->spritesheets[id] = Anm2Spritesheet{};
	self->anm2->spritesheets[id].path = spritesheetPath;
	texture_from_path_init(&self->anm2->spritesheets[id].texture, spritesheetPath);
	
	std::filesystem::current_path(workingPath);
}

static bool _imgui_is_window_hovered(void)
{
	return ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
}

static bool _imgui_is_window_hovered_and_click(void)
{
	return _imgui_is_window_hovered() && ImGui::IsMouseClicked(0);
}

static bool _imgui_is_window_hovered_and_click_no_anm2_path(Imgui* self)
{
	return _imgui_is_window_hovered_and_click() && self->anm2->path.empty();
}

static bool _imgui_is_no_click_on_item(void)
{
    return ImGui::IsMouseClicked(0) && !ImGui::IsAnyItemHovered();
}

static bool _imgui_is_input_begin(void)
{
	return ImGui::IsItemHovered() && (ImGui::IsKeyPressed(IMGUI_INPUT_RENAME) || ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left));
}

static bool _imgui_is_input_default(void)
{
	return ImGui::IsItemHovered() && (ImGui::IsKeyPressed(IMGUI_INPUT_DEFAULT) || ImGui::IsMouseClicked(IMGUI_MOUSE_DEFAULT));
}

static bool _imgui_is_focus_window(const std::string& focus)
{
    ImGuiContext* ctx = ImGui::GetCurrentContext();
    if (!ctx || !ctx->NavWindow)  return false;

    std::string_view name(ctx->NavWindow->Name);
    return name.find(focus) != std::string_view::npos;
}

static void _imgui_atlas(const AtlasType& self, Imgui* imgui)
{
	ImGui::Image(imgui->resources->atlas.id, ATLAS_SIZE(self), ATLAS_UV_ARGS(self));
}

static ImVec2 _imgui_item_size_get(const ImguiItem& self, ImguiItemType type)
{
	ImVec2 size = self.size;

	switch (type)
	{
		case IMGUI_ATLAS_BUTTON:
			size = self.is_size() ? self.size : ImVec2(ATLAS_SIZE(self.atlas));
			break;
		default:
			if (self.is_row())
				size.x = (ImGui::GetWindowSize().x - (ImGui::GetStyle().ItemSpacing.x * (self.rowCount + 1))) / self.rowCount;
			else if (self.isSizeToText)
				size.x = (ImGui::CalcTextSize(self.label_get().c_str()).x + ImGui::GetStyle().FramePadding.x);
			else if (!self.is_size())
				size.x = ImGui::CalcItemWidth();
			break;
	}

	return size;
}

static void _imgui_item_pre(const ImguiItem& self, ImguiItemType type)
{
	ImVec2 size = _imgui_item_size_get(self, type);

	switch (type)
	{
		case IMGUI_ITEM:
		case IMGUI_TEXT:
		case IMGUI_WINDOW:
		case IMGUI_DOCKSPACE:
		case IMGUI_CHILD:
		case IMGUI_CONFIRM_POPUP:
			break;
		default:
			ImGui::BeginDisabled(self.isDisabled);
	}
	
	switch (type)
	{
		case IMGUI_CHILD:
    		if (self.color.is_normal()) ImGui::PushStyleColor(ImGuiCol_ChildBg, self.color.normal);
			break;
		case IMGUI_INPUT_INT:
		case IMGUI_INPUT_TEXT:
		case IMGUI_INPUT_FLOAT:
		case IMGUI_DRAG_FLOAT:
		case IMGUI_SLIDER_FLOAT:
		case IMGUI_COLOR_EDIT:
				ImGui::SetNextItemWidth(size.x);
				break;
		case IMGUI_BUTTON:
		case IMGUI_ATLAS_BUTTON:
			if (self.is_border()) 
			{
				ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, self.border);
				if (self.color.is_border()) ImGui::PushStyleColor(ImGuiCol_Border, self.color.border);
			}
			if (self.color.is_normal()) ImGui::PushStyleColor(ImGuiCol_Button, self.color.normal);
			if (self.color.is_active()) ImGui::PushStyleColor(ImGuiCol_ButtonActive, self.color.active);
			if (self.color.is_hovered()) ImGui::PushStyleColor(ImGuiCol_ButtonHovered, self.color.hovered);
			if (self.isSelected)
			{ 
				if (self.color.is_active())
					ImGui::PushStyleColor(ImGuiCol_Button, self.color.active);
				else
					ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
			}
			break;
		default:
			break;
	}
}

static void _imgui_item_post(const ImguiItem& self, Imgui* imgui, ImguiItemType type, bool& isActivated)
{
	if (self.is_mnemonic() && !self.isMnemonicDisabled)
	{
		ImVec2 position = ImGui::GetItemRectMin();
		ImFont* font = ImGui::GetFont();
		f32 fontSize = ImGui::GetFontSize();
		const char* start = self.label.c_str();
		const char* charPointer = start + self.mnemonicIndex;

		position.x += ImGui::GetStyle().FramePadding.x;
		
		f32 offset = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, start, charPointer).x;
		f32 charWidth = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, charPointer, charPointer + 1).x;

		ImVec2 lineStart = ImVec2(position.x + offset, position.y + fontSize + 1.0f);
		ImVec2 lineEnd = ImVec2(lineStart.x + charWidth, lineStart.y);

		ImU32 color = ImGui::GetColorU32(ImGuiCol_Text);
		ImGui::GetWindowDrawList()->AddLine(lineStart, lineEnd, color, 1.0f);
		
		if (_imgui_chord_pressed(ImGuiMod_Alt | self.mnemonicKey))
		{
			if (!self.isDisabled) isActivated = true;
			imgui_close_current_popup(imgui);
		}
	}

	if (self.isUseItemActivated && !self.isDisabled) isActivated = ImGui::IsItemActivated();
	
	if 
	(
		imgui->isContextualActionsEnabled && _imgui_chord_pressed(self.chord_get()) &&
		(self.is_focus_window() && _imgui_is_focus_window(self.focusWindow))
	)
		if (!self.isDisabled) isActivated = true;

	if (self.is_tooltip() && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) 
		ImGui::SetTooltip(self.tooltip_get().c_str());

	if (isActivated)
	{
		if (self.is_undoable()) imgui_snapshot(imgui, self.snapshotAction);
		if (self.is_function()) self.function(imgui);

		if (self.is_popup())
		{
			imgui->pendingPopup = self.popup;
			imgui->pendingPopupType = self.popupType;
			imgui->pendingPopupPosition = ImVec2(ImGui::GetItemRectMin().x, ImGui::GetItemRectMin().y + ImGui::GetItemRectSize().y);
		}
	}
	
	switch (type)
	{
		case IMGUI_CHILD:
    		if (self.color.is_normal()) ImGui::PopStyleColor();
			break;
		case IMGUI_BUTTON:
		case IMGUI_ATLAS_BUTTON:
			if (self.is_border()) 
			{
				ImGui::PopStyleVar();
				if (self.color.is_border()) ImGui::PopStyleColor();
			}
    		if (self.color.is_normal()) ImGui::PopStyleColor();
    		if (self.color.is_active()) ImGui::PopStyleColor();
    		if (self.color.is_hovered()) ImGui::PopStyleColor();
    		if (self.isSelected) ImGui::PopStyleColor();
			break;
		default:
			break;
	}

	switch (type)
	{
		case IMGUI_ITEM:
		case IMGUI_TEXT:
		case IMGUI_WINDOW:
		case IMGUI_DOCKSPACE:
		case IMGUI_CHILD:
		case IMGUI_CONFIRM_POPUP:
			break;
		default:
			ImGui::EndDisabled();
	}

	if (self.isSameLine) ImGui::SameLine();
	if (self.isSeparator) ImGui::Separator();
}

#define IMGUI_ITEM_FUNCTION(NAME, TYPE, FUNCTION)                                 \
static bool NAME(ImguiItem self, Imgui* imgui)                                    \
{                                                                                 \
	ImguiItemType type = TYPE;													  \
    _imgui_item_pre(self, type);                                           		  \
    bool isActivated = ([&] { return FUNCTION; })();                              \
    _imgui_item_post(self, imgui, type, isActivated);                             \
    return isActivated;                                                           \
}

#define IMGUI_ITEM_VOID_FUNCTION(NAME, TYPE, FUNCTION)                            \
static void NAME(const ImguiItem& self, Imgui* imgui)                             \
{                             													  \
	ImguiItemType type = TYPE;													  \
    bool isActivated = false;                                                     \
    _imgui_item_pre(self, type);                                                  \
    ([&] { FUNCTION; })();                                                        \
    _imgui_item_post(self, imgui, type, isActivated);                             \
}

#define IMGUI_ITEM_VALUE_FUNCTION(NAME, TYPE, VALUE, FUNCTION)                    			\
static bool NAME(const ImguiItem& self, Imgui* imgui, VALUE& inValue)              			\
{                                                                                 			\
	ImguiItemType type = TYPE;													  			\
    VALUE value = inValue;                                                        			\
    _imgui_item_pre(self, type);                                                  			\
    bool isActivated = ([&](VALUE& value) { return FUNCTION; })(value);            			\
	if (_imgui_is_input_default()) 															\
	{																						\
		value = VALUE(self.value);															\
		isActivated = true;															        \
	}																						\
    _imgui_item_post(self, imgui, type, isActivated);                             			\
    inValue = value;                                                              			\
    return isActivated;                                                           			\
}

#define IMGUI_ITEM_VALUE_CLAMP_FUNCTION(NAME, TYPE, VALUE, FUNCTION)              			\
static bool NAME(const ImguiItem& self, Imgui* imgui, VALUE& inValue)                   	\
{                                                                                 			\
	ImguiItemType type = TYPE;													  			\
    VALUE value = inValue;                                                        			\
    _imgui_item_pre(self, type);                                           		        	\
    bool isActivated = ([&](VALUE& value) { return FUNCTION; })(value);           			\
	if (_imgui_is_input_default()) 															\
	{																						\
		value = VALUE(self.value);															\
		isActivated = true;															        \
	}																						\
	if (self.is_range()) value = glm::clamp(value, VALUE(self.min), VALUE(self.max)); 		\
    _imgui_item_post(self, imgui, type, isActivated);                             			\
    inValue = value;                                                              			\
    return isActivated;                                                           			\
}

#define IMGUI_ITEM_CUSTOM_FUNCTION(NAME, TYPE, BODY)                              \
static bool NAME(const ImguiItem& self, Imgui* imgui)                             \
{                                                                                 \
	ImguiItemType type = TYPE;													  \
    _imgui_item_pre(self, type);                                                  \
    bool isActivated = false;                                                     \
    do { BODY } while (0);                                                        \
    _imgui_item_post(self, imgui, type, isActivated);                             \
    return isActivated;                                                           \
}

#define IMGUI_ITEM_CUSTOM_FUNCTION_WITH_VALUE(NAME, TYPE, VALUE, BODY)            \
static bool NAME(const ImguiItem& self, Imgui* imgui, VALUE& inValue)             \
{                                                                                 \
	ImguiItemType type = TYPE;													  \
    _imgui_item_pre(self, type);                                                  \
    bool isActivated = false;                                                     \
    do { BODY } while (0);                                                        \
    _imgui_item_post(self, imgui, type, isActivated);                             \
    return isActivated;                                                           \
}

#define IMGUI_ITEM_ATLAS_FUNCTION(NAME, FUNCTION)                                 \
static bool NAME(const ImguiItem& self, Imgui* imgui)                             \
{                                                                                 \
	_imgui_atlas(self.atlas, imgui);                                              \
	ImGui::SameLine();                                                            \
    bool isActivated = ([&] { return FUNCTION; })();                              \
    return isActivated;                                                           \
}

#define IMGUI_ITEM_ATLAS_VALUE_FUNCTION(NAME, VALUE, FUNCTION)                    \
static bool NAME(const ImguiItem& self, Imgui* imgui, VALUE& value)               \
{                                                                                 \
	_imgui_atlas(self.atlas, imgui);                                              \
	ImGui::SameLine();                                                            \
    bool isActivated = ([&](VALUE& value) { return FUNCTION; })(value);           \
    return isActivated;                                                           \
}

#define IMGUI_ITEM_CHECKBOX_FUNCTION(NAME, FUNCTION)											    \
static bool NAME(const ImguiItem& self, Imgui* imgui, bool& boolValue)				                \
{																									\
	ImguiItem checkboxItem = self.copy																\
	({.label = std::format(IMGUI_INVISIBLE_FORMAT, self.label), .isMnemonicDisabled = true});		\
	checkboxItem.isDisabled = false;																\
	checkboxItem.isSeparator = false;																\
	checkboxItem.value = 0;																		    \
	bool isCheckboxActivated = _imgui_checkbox(checkboxItem, imgui, boolValue);						\
	ImGui::SameLine();			                                                                    \
    bool isActivated = ([&] { return FUNCTION; })();                                                \
	if (isActivated) boolValue = !boolValue;											            \
	if (isCheckboxActivated) isActivated = true;													\
	return isActivated;																				\
}

#define IMGUI_ITEM_CHECKBOX_VALUE_FUNCTION(NAME, VALUE, FUNCTION)									\
static bool NAME(const ImguiItem& self, Imgui* imgui, VALUE& value, bool& boolValue)				\
{																									\
	ImguiItem checkboxItem = self.copy																\
	({.label = std::format(IMGUI_INVISIBLE_FORMAT, self.label), .isMnemonicDisabled = true});		\
	checkboxItem.isDisabled = false;																\
	checkboxItem.isSeparator = false;																\
	checkboxItem.value = 0;																		    \
	bool isCheckboxActivated = _imgui_checkbox(checkboxItem, imgui, boolValue);						\
	ImGui::SameLine();																				\
    bool isActivated = ([&](VALUE& value) { return FUNCTION; })(value);								\
	if (isCheckboxActivated) isActivated = true;													\
	return isActivated;																				\
}

#define IMGUI_ITEM_AND_CHECKBOX_FUNCTION(NAME, VALUE, FUNCTION)							    		\
static bool NAME(const ImguiItem& self, const ImguiItem& checkboxItem, Imgui* imgui, VALUE& value, bool& boolValue) \
{																									\
	bool isCheckboxActivated = _imgui_checkbox(checkboxItem, imgui, boolValue);						\
	ImGui::SameLine();																				\
    bool isActivated = ([&](VALUE& value) { return FUNCTION; })(value);								\
	if (isCheckboxActivated) isActivated = true;													\
	return isActivated;																				\
}

#define IMGUI_ITEM_DISABLED_GET(VALUE, ITEM, CONDITION) ImguiItem VALUE = ITEM; VALUE.isDisabled = CONDITION;

IMGUI_ITEM_FUNCTION(_imgui_begin, IMGUI_WINDOW, ImGui::Begin(self.label_get().c_str(), nullptr, self.flags));
#define IMGUI_BEGIN_OR_RETURN(item, imgui) if (!_imgui_begin(item, imgui)) { _imgui_end(); return; }
static void _imgui_end(void){ImGui::End();}
IMGUI_ITEM_VOID_FUNCTION(_imgui_dockspace, IMGUI_DOCKSPACE, ImGui::DockSpace(ImGui::GetID(self.label_get().c_str()), self.size, self.flags));
IMGUI_ITEM_FUNCTION(_imgui_begin_child, IMGUI_CHILD, ImGui::BeginChild(self.label_get().c_str(), self.size, self.flags, self.windowFlags));
static void _imgui_end_child(void) {ImGui::EndChild(); }
IMGUI_ITEM_VOID_FUNCTION(_imgui_text, IMGUI_TEXT, ImGui::Text(self.label_get().c_str()));
IMGUI_ITEM_FUNCTION(_imgui_button, IMGUI_BUTTON, ImGui::Button(self.label_get().c_str(), _imgui_item_size_get(self, type)));
IMGUI_ITEM_FUNCTION(_imgui_begin_table, IMGUI_TABLE, ImGui::BeginTable(self.label_get().c_str(), self.value, self.flags));
static void _imgui_end_table(void) {ImGui::EndTable(); }
static void _imgui_table_setup_column(const char* text) {ImGui::TableSetupColumn(text); }
static void _imgui_table_headers_row(void) {ImGui::TableHeadersRow(); }
static void _imgui_table_next_row(void) {ImGui::TableNextRow(); }
static void _imgui_table_set_column_index(s32 index) {ImGui::TableSetColumnIndex(index); }
IMGUI_ITEM_FUNCTION(_imgui_selectable, IMGUI_SELECTABLE, ImGui::Selectable(self.label_get().c_str(), self.isSelected, self.flags, _imgui_item_size_get(self, type)));
IMGUI_ITEM_VALUE_FUNCTION(_imgui_radio_button, IMGUI_RADIO_BUTTON, s32, ImGui::RadioButton(self.label_get().c_str(), &value, self.value));
IMGUI_ITEM_VALUE_FUNCTION(_imgui_color_button, IMGUI_COLOR_BUTTON, vec4, ImGui::ColorButton(self.label_get().c_str(), ImVec4(value), self.flags));
IMGUI_ITEM_VALUE_FUNCTION(_imgui_checkbox, IMGUI_CHECKBOX, bool, ImGui::Checkbox(self.label_get().c_str(), &value));
IMGUI_ITEM_VALUE_CLAMP_FUNCTION(_imgui_input_int, IMGUI_INPUT_INT, s32, ImGui::InputInt(self.label_get().c_str(), &value, self.step, self.stepFast, self.flags));
IMGUI_ITEM_VALUE_CLAMP_FUNCTION(_imgui_input_int2, IMGUI_INPUT_INT, ivec2, ImGui::InputInt2(self.label_get().c_str(), value_ptr(value), self.flags));
IMGUI_ITEM_VALUE_CLAMP_FUNCTION(_imgui_input_float, IMGUI_INPUT_FLOAT, f32, ImGui::InputFloat(self.label_get().c_str(), &value, self.step, self.stepFast, _imgui_f32_format_get(self, value), self.flags));
IMGUI_ITEM_VALUE_FUNCTION(_imgui_slider_float, IMGUI_SLIDER_FLOAT, f32, ImGui::SliderFloat(self.label_get().c_str(), &value, self.min, self.max, _imgui_f32_format_get(self, value), self.flags));
IMGUI_ITEM_VALUE_FUNCTION(_imgui_drag_float, IMGUI_DRAG_FLOAT, f32, ImGui::DragFloat(self.label_get().c_str(), &value, self.speed, self.min, self.max, _imgui_f32_format_get(self, value)));
IMGUI_ITEM_VALUE_FUNCTION(_imgui_drag_float2, IMGUI_DRAG_FLOAT, vec2, ImGui::DragFloat2(self.label_get().c_str(), value_ptr(value), self.speed, self.min, self.max, _imgui_vec2_format_get(self, value)));
IMGUI_ITEM_VALUE_FUNCTION(_imgui_color_edit3, IMGUI_COLOR_EDIT, vec3, ImGui::ColorEdit3(self.label_get().c_str(), value_ptr(value), self.flags));
IMGUI_ITEM_VALUE_FUNCTION(_imgui_color_edit4, IMGUI_COLOR_EDIT, vec4, ImGui::ColorEdit4(self.label_get().c_str(), value_ptr(value), self.flags));
IMGUI_ITEM_CHECKBOX_FUNCTION(_imgui_checkbox_selectable, _imgui_selectable(self, imgui));
IMGUI_ITEM_CHECKBOX_VALUE_FUNCTION(_imgui_checkbox_checkbox, bool, _imgui_checkbox(self, imgui, value));
IMGUI_ITEM_CHECKBOX_VALUE_FUNCTION(_imgui_checkbox_input_int, s32, _imgui_input_int(self, imgui, value));
IMGUI_ITEM_CHECKBOX_VALUE_FUNCTION(_imgui_checkbox_drag_float, f32, _imgui_drag_float(self, imgui, value));
IMGUI_ITEM_CHECKBOX_VALUE_FUNCTION(_imgui_checkbox_drag_float2, vec2, _imgui_drag_float2(self, imgui, value));
IMGUI_ITEM_CHECKBOX_VALUE_FUNCTION(_imgui_checkbox_color_edit3, vec3, _imgui_color_edit3(self, imgui, value));
IMGUI_ITEM_CHECKBOX_VALUE_FUNCTION(_imgui_checkbox_color_edit4, vec4, _imgui_color_edit4(self, imgui, value));

static bool _imgui_input_text(const ImguiItem& self, Imgui* imgui, std::string& value)
{
	value.resize(self.max);
    _imgui_item_pre(self, IMGUI_INPUT_TEXT);
    bool isActivated = ImGui::InputText(self.label_get().c_str(), value.data(), self.max, self.flags);
    _imgui_item_post(self, imgui, IMGUI_INPUT_TEXT, isActivated);
    return isActivated;
}

static bool _imgui_combo(ImguiItem self, Imgui* imgui, s32* value)
{
	std::vector<const char*> cStrings;
	cStrings.reserve(self.items.size());
	for (auto& string : self.items)
		cStrings.emplace_back(string.c_str());

	_imgui_item_pre(self, IMGUI_COMBO);

	bool isActivated = ImGui::Combo(self.label_get().c_str(), value, cStrings.data(), (s32)self.items.size());
	if (_imgui_is_input_default()) 
	{
		*value = self.value; 
		isActivated = true;
	}

	_imgui_item_post(self, imgui, IMGUI_COMBO, isActivated);
	
	return isActivated;
};

IMGUI_ITEM_CUSTOM_FUNCTION(_imgui_atlas_button, IMGUI_ATLAS_BUTTON,
{
	ImVec2 size = _imgui_item_size_get(self, type);

	if (self.is_size())
	{
		isActivated = ImGui::Button(self.label_get().c_str(), size);
		
		ImVec2 start = ImGui::GetItemRectMin() + self.atlasOffset;
		ImVec2 end = start + ImVec2(ATLAS_SIZE(self.atlas));

		ImGui::GetWindowDrawList()->AddImage(imgui->resources->atlas.id, start, end, ATLAS_UV_ARGS(self.atlas));
	}
	else
		isActivated = ImGui::ImageButton(self.label_get().c_str(), imgui->resources->atlas.id, size, ATLAS_UV_ARGS(self.atlas));
});

static bool _imgui_selectable_input_int(const ImguiItem& self, Imgui* imgui, s32& value)
{
	static s32 temp;
    static s32 id = ID_NONE;
	ImguiItem selectable = self.copy({.label = std::format(IMGUI_SELECTABLE_INPUT_INT_FORMAT, value)});
	ImVec2 size = _imgui_item_size_get(selectable, IMGUI_SELECTABLE);
	bool isActivated = false;
	
    if (id == self.id)
    {
		isActivated = _imgui_input_int(IMGUI_CHANGE_INPUT_INT.copy({.size = size}), imgui, temp);
		imgui_contextual_actions_disable(imgui);
		
		if (isActivated || _imgui_is_no_click_on_item())
        {
			value = temp;
			id = ID_NONE;
			imgui_contextual_actions_enable(imgui);
        }
    }
    else
    {
        isActivated = _imgui_selectable(selectable, imgui);
        
		if (_imgui_is_input_begin())
        {
			temp = value;
            id = self.id;
            ImGui::SetKeyboardFocusHere(-1);
        }
    }

	return isActivated;
};

static bool _imgui_selectable_input_text(const ImguiItem& self, Imgui* imgui, std::string& value)
{
	static std::string buffer{};
    static s32 id = ID_NONE;
	ImguiItem selectable = self.copy({});
	ImVec2 size = _imgui_item_size_get(selectable, IMGUI_SELECTABLE);
	bool isActivated = false;
	
    if (id == self.id)
    {
		isActivated = _imgui_input_text(IMGUI_CHANGE_INPUT_TEXT.copy({.size = size}), imgui, buffer);
		imgui_contextual_actions_disable(imgui);
		
		if (isActivated || _imgui_is_no_click_on_item())
        {
			value = buffer;
			buffer.clear();
			id = ID_NONE;
			imgui_contextual_actions_enable(imgui);
        }
    }
    else
    {
        isActivated = _imgui_selectable(selectable, imgui);
        
		if (_imgui_is_input_begin())
        {
			buffer = value;
			buffer.resize(IMGUI_CHANGE_INPUT_TEXT.max);
            id = self.id;
            ImGui::SetKeyboardFocusHere(-1);
        }
    }

	return isActivated;
};

IMGUI_ITEM_ATLAS_FUNCTION(_imgui_atlas_selectable, _imgui_selectable(self, imgui));
IMGUI_ITEM_ATLAS_VALUE_FUNCTION(_imgui_atlas_selectable_input_int, s32, _imgui_selectable_input_int(self, imgui, value));
IMGUI_ITEM_ATLAS_VALUE_FUNCTION(_imgui_atlas_selectable_input_text, std::string, _imgui_selectable_input_text(self, imgui, value));

static bool _imgui_confirm_popup(ImguiItem self, Imgui* imgui, ImguiPopupState* state = nullptr, bool isOnlyConfirm = false)
{
	ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	
	if (state) *state = IMGUI_POPUP_STATE_CLOSED;

	if (imgui_begin_popup_modal(self.label_get().c_str(), imgui))
	{
		if (state) *state = IMGUI_POPUP_STATE_OPEN;
		
		ImGui::Text(self.text_get());
		ImGui::Separator();

		if (_imgui_button(IMGUI_POPUP_OK.copy({.rowCount = isOnlyConfirm ? 1 : IMGUI_CONFIRM_POPUP_ROW_COUNT}), imgui))
		{
			imgui_close_current_popup(imgui);
			imgui_end_popup(imgui);
			if (state) *state = isOnlyConfirm ? IMGUI_POPUP_STATE_CANCEL : IMGUI_POPUP_STATE_CONFIRM;
			return true;
		}

		ImGui::SameLine();
		
		if (!isOnlyConfirm)
		{
			if (_imgui_button(IMGUI_POPUP_CANCEL, imgui))
			{
				imgui_close_current_popup(imgui);
				if (state) *state = IMGUI_POPUP_STATE_CANCEL;
			}
		}

		imgui_end_popup(imgui);
	}

	return false;
}

static void _imgui_no_anm2_path_check(Imgui* self)
{
	if (_imgui_is_window_hovered_and_click_no_anm2_path(self) && !imgui_is_any_popup_open())
		imgui_open_popup(IMGUI_NO_ANM2_PATH_CONFIRMATION.label);
	_imgui_confirm_popup(IMGUI_NO_ANM2_PATH_CONFIRMATION, self, nullptr, true);
}

static void _imgui_context_menu(Imgui* self)
{
	if (!self->isContextualActionsEnabled) return;
	
	if (_imgui_is_window_hovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
		imgui_open_popup(IMGUI_CONTEXT_MENU.label_get());

	if (imgui_begin_popup(IMGUI_CONTEXT_MENU.label_get(), self))
	{
		_imgui_selectable(IMGUI_CUT, self);			
		_imgui_selectable(IMGUI_COPY, self);			
		_imgui_selectable(IMGUI_PASTE.copy({!clipboard_is_value()}), self);

		imgui_end_popup(self);
	}
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
	static Anm2Reference hoverReference;
	static Anm2Reference swapItemReference;
	static Anm2Type& itemType = self->reference->itemType;
	static ImVec2 itemMin{};
	static ImVec2 localMousePos{};
	static ImVec2 mousePos{};
	static ImVec2 playheadPos{};
	static ImVec2 scroll{};
	static bool isItemSwap = false; 
	static const ImVec2& frameSize = IMGUI_TIMELINE_FRAME_SIZE;
	static f32& time = self->preview->time;
	static s32 frameTime{};
	static s32& itemID = self->reference->itemID;
	
	IMGUI_BEGIN_OR_RETURN(IMGUI_TIMELINE, self);
	_imgui_no_anm2_path_check(self);

	Anm2Animation* animation = anm2_animation_from_reference(self->anm2, self->reference);

	if (!animation)
	{
		ImGui::Text(IMGUI_TIMELINE_ANIMATION_NONE);
		_imgui_end(); // IMGUI_TIMELINE
		return;
	}

	s32 length = animation->frameNum;
	s32 actualLength = anm2_animation_length_get(animation);
	
	ImVec2 actualFramesSize = {actualLength * frameSize.x, frameSize.y};
	ImVec2 framesSize = {length * frameSize.x, frameSize.y};

	ImVec2 defaultItemSpacing = ImGui::GetStyle().ItemSpacing;
	ImVec2 defaultWindowPadding = ImGui::GetStyle().WindowPadding;
	ImVec2 defaultFramePadding = ImGui::GetStyle().FramePadding;

	vec2 windowSize = ImGui::GetContentRegionAvail();
	vec2 childSize = {windowSize.x, windowSize.y - IMGUI_FOOTER_CHILD.size.y}; 

	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

	_imgui_begin_child(IMGUI_TIMELINE_CHILD.copy({.size = childSize}), self);
	ImVec2 clipRectMin = ImGui::GetWindowDrawList()->GetClipRectMin();
	ImVec2 clipRectMax = ImGui::GetWindowDrawList()->GetClipRectMax();
	clipRectMin.x += IMGUI_TIMELINE_ITEM_SIZE.x;

	ImVec2 scrollDelta{};
	
	if (_imgui_is_window_hovered() && !imgui_is_any_popup_open())
	{
		ImGuiIO& io = ImGui::GetIO();
		f32 lineHeight = ImGui::GetTextLineHeight();

		scrollDelta.x -= io.MouseWheelH * lineHeight;
		scrollDelta.y -= io.MouseWheel * lineHeight;
	}

	std::function<void()> timeline_header = [&]()
	{
		static bool isPlayheadDrag = false;
		_imgui_begin_child(IMGUI_TIMELINE_HEADER_CHILD, self);

		ImGui::SetScrollX(scroll.x);
		
		itemMin = ImGui::GetItemRectMin();
		mousePos = ImGui::GetMousePos();
		localMousePos = ImVec2(mousePos.x - itemMin.x + scroll.x, mousePos.y - itemMin.y);
		frameTime = (s32)(localMousePos.x / frameSize.x);

		if (ImGui::IsMouseDown(0) && _imgui_is_window_hovered()) isPlayheadDrag = true;

		if (isPlayheadDrag)
		{
			if (self->settings->playbackIsClampPlayhead)
				time = std::clamp(frameTime, 0, std::max(0, length - 1));
			else
				time = std::max(frameTime, 0);
		
			if (ImGui::IsMouseReleased(0)) 
				isPlayheadDrag = false;
		}

		ImVec2 cursorPos = ImGui::GetCursorScreenPos();
		playheadPos = {cursorPos.x + (time * frameSize.x), cursorPos.y};

		ImDrawList* drawList = ImGui::GetWindowDrawList();

		ImVec2 dummySize = actualFramesSize.x > framesSize.x ? actualFramesSize : framesSize;
		ImGui::Dummy(dummySize);

		f32 viewWidth = ImGui::GetWindowContentRegionMax().x - ImGui::GetWindowContentRegionMin().x;
		s32 start = (s32)std::floor(scroll.x / frameSize.x) - 1;
		start = (start < 0) ? 0 : start;

		s32 end   = (s32)std::ceil((scroll.x + viewWidth) / frameSize.x) + 1;
		end = (end > ANM2_FRAME_NUM_MAX) ? ANM2_FRAME_NUM_MAX : end;

		playheadPos = ImVec2(cursorPos.x + time * frameSize.x, cursorPos.y);

		for (s32 i = start; i < end; i++)
		{
			bool isMultiple = (i % IMGUI_TIMELINE_FRAME_MULTIPLE) == 0;
			bool isInactive = i >= length;

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

			drawList->AddImage(self->resources->atlas.id, positionStart, positionEnd, ATLAS_UV_ARGS(ATLAS_FRAME_ALT));
		}

		_imgui_end_child(); // IMGUI_TIMELINE_HEADER_CHILD
	
		ImGui::SetNextWindowPos(ImGui::GetWindowPos());
		ImGui::SetNextWindowSize(ImGui::GetWindowSize());
		_imgui_begin(IMGUI_PLAYHEAD, self);

		ImVec2& pos = playheadPos;
		
		ImVec2 lineStart = {pos.x + (frameSize.x * 0.5f) - (IMGUI_PLAYHEAD_LINE_WIDTH * 0.5f), pos.y + frameSize.y};
		ImVec2 lineEnd = {lineStart.x + IMGUI_PLAYHEAD_LINE_WIDTH, lineStart.y + childSize.y - frameSize.y};
		
		drawList = ImGui::GetWindowDrawList();

		drawList->PushClipRect(clipRectMin, clipRectMax, true);
		drawList->AddImage(self->resources->atlas.id, pos, ImVec2(pos.x + frameSize.x, pos.y + frameSize.y), ATLAS_UV_ARGS(ATLAS_PICKER));
		drawList->AddRectFilled(lineStart, lineEnd, IMGUI_PLAYHEAD_LINE_COLOR);
		drawList->PopClipRect();

		_imgui_end(); // IMGUI_PLAYHEAD
	};

	std::function<void(Anm2Reference, s32&)> timeline_item_child = [&](Anm2Reference reference, s32& index)
	{
		Anm2Item* item = anm2_item_from_reference(self->anm2, &reference);
		Anm2Type& type = reference.itemType;
		if (!item) return;
		if (!self->settings->timelineIsShowUnused && item->frames.empty() && (type == ANM2_LAYER || type == ANM2_NULL)) return;

		ImVec2 buttonSize = ImVec2(ATLAS_SIZE_NORMAL) + (defaultFramePadding * ImVec2(2, 2));
		
		Anm2Layer* layer = nullptr;
		Anm2Null* null = nullptr;
		s32 buttonCount = type == ANM2_NULL ? 2 : 1;
		f32 buttonAreaWidth = buttonCount * buttonSize.x + (buttonCount - 1) * defaultItemSpacing.x;
		bool isSelected = (self->reference->itemID == reference.itemID && self->reference->itemType == type);

		ImGui::PushID(reference.itemID);
				
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, defaultItemSpacing);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, defaultWindowPadding);
			
		ImVec2 childPos = ImGui::GetCursorScreenPos();
		_imgui_begin_child(*IMGUI_TIMELINE_ITEM_CHILDS[type], self);
		ImVec2 childSize = ImGui::GetContentRegionAvail();

		std::string dragDrop = IMGUI_TIMELINE_ITEM_SELECTABLES[type]->dragDrop;

		switch (type)
		{
			case ANM2_ROOT:
			case ANM2_TRIGGERS:
				if (_imgui_atlas_selectable(IMGUI_TIMELINE_ITEM_SELECTABLES[type]->copy({.isSelected = isSelected}), self))
					*self->reference = reference;
				break;
			case ANM2_LAYER:
				layer = &self->anm2->layers[reference.itemID];
				if 
				(
					_imgui_atlas_selectable(IMGUI_TIMELINE_ITEM_SELECTABLES[type]->copy
					({
						.isSelected = isSelected, 
						.label = std::format(IMGUI_TIMELINE_ITEM_CHILD_FORMAT, reference.itemID, layer->name)
					}),	
					self
				)
				)
					*self->reference = reference;

				break;
			case ANM2_NULL:
				null = &self->anm2->nulls[reference.itemID];
				if 
				(
					_imgui_atlas_selectable(IMGUI_TIMELINE_ITEM_SELECTABLES[type]->copy
					({
						.isSelected = isSelected, 
						.label = std::format(IMGUI_TIMELINE_ITEM_CHILD_FORMAT, reference.itemID, null->name)
					}),	
					self)
				)
					*self->reference = reference;
				break;
			default:
				break;
		}

		if (imgui_begin_popup_modal(IMGUI_POPUP_ITEM_PROPERTIES, self, IMGUI_POPUP_ITEM_PROPERTIES_SIZE))
		{
			static s32 selectedLayerID = ID_NONE;
			static s32 selectedNullID = ID_NONE;
			Anm2Type& type = reference.itemType;

			_imgui_begin_child(IMGUI_TIMELINE_ITEM_PROPERTIES_TYPE_CHILD, self);

			_imgui_radio_button(IMGUI_TIMELINE_ITEM_PROPERTIES_LAYER.copy({true}), self, (s32&)type);
			_imgui_radio_button(IMGUI_TIMELINE_ITEM_PROPERTIES_NULL.copy({true}), self, (s32&)type);

			_imgui_end_child(); // IMGUI_TIMELINE_ITEM_PROPERTIES_TYPE_CHILD
			
			_imgui_begin_child(IMGUI_TIMELINE_ITEM_PROPERTIES_ITEMS_CHILD, self);
		
			switch (type)
			{
				case ANM2_LAYER:
				default:
				{
					for (auto & [id, layer] : self->anm2->layers)
					{
						if (id == reference.itemID) continue;
						
						ImGui::PushID(id);

						ImguiItem layerItem = IMGUI_LAYER.copy
						({
							.isSelected = selectedLayerID == id,
							.label = std::format(IMGUI_LAYER_FORMAT, id, layer.name),
							.id = id
						});
						if (_imgui_atlas_selectable(layerItem, self)) selectedLayerID = id;

						ImGui::PopID();
					};
					break;
				}			
				case ANM2_NULL:
				{
					for (auto & [id, null] : self->anm2->nulls)
					{
						if (id == reference.itemID) continue;
						
						ImGui::PushID(id);

						ImguiItem nullItem = IMGUI_NULL.copy
						({
							.isSelected = selectedNullID == id,
							.label = std::format(IMGUI_NULL_FORMAT, id, null.name),
							.id = id
						});
						if (_imgui_atlas_selectable(nullItem, self)) selectedNullID = id;

						ImGui::PopID();
					};
					break;
				}
			}

			_imgui_end_child(); // IMGUI_TIMELINE_ITEM_PROPERTIES_ITEMS_CHILD

			_imgui_begin_child(IMGUI_TIMELINE_ITEM_PROPERTIES_OPTIONS_CHILD, self);
		
			if (self->anm2->layers.size() == 0) selectedLayerID = ID_NONE;
			if (self->anm2->nulls.size() == 0) selectedNullID = ID_NONE;

			bool isDisabled = 				 type == ANM2_NONE || 
			(type == ANM2_LAYER && selectedLayerID == ID_NONE) || 
			(type == ANM2_NULL && selectedNullID == ID_NONE);
			
			if (_imgui_button(IMGUI_TIMELINE_ITEM_PROPERTIES_CONFIRM.copy({isDisabled}), self))
			{
				switch (type)
				{
					case ANM2_LAYER: 
						if (!map_find(animation->layerAnimations, selectedLayerID))
						{
							anm2_animation_layer_animation_add(animation, selectedLayerID);
							anm2_animation_layer_animation_remove(animation, reference.itemID);
						}
						else
							map_swap(animation->layerAnimations, selectedLayerID, reference.itemID); 

						*self->reference = {self->reference->animationID, ANM2_LAYER, selectedLayerID};
						break;
					case ANM2_NULL: 
						if (!map_find(animation->nullAnimations, selectedNullID))
						{
							anm2_animation_null_animation_add(animation, selectedNullID);
							anm2_animation_null_animation_remove(animation, reference.itemID);

						}
						else
							map_swap(animation->nullAnimations, selectedNullID, reference.itemID); 

						*self->reference = {self->reference->animationID, ANM2_NULL, selectedNullID};
						break;
					default: break;
				}
				 
				imgui_close_current_popup(self);
			}
			
			if (_imgui_button(IMGUI_POPUP_CANCEL, self)) imgui_close_current_popup(self);
			
			_imgui_end_child(); // IMGUI_TIMELINE_ITEM_PROPERTIES_OPTIONS_CHILD
					
			imgui_end_popup(self);
		}

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2());
		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None) && !dragDrop.empty())
		{
			*self->reference = reference;
			
			ImGui::SetDragDropPayload(dragDrop.c_str(), &reference, sizeof(Anm2Reference));
			timeline_item_child(reference, index);
			ImGui::EndDragDropSource();
		}
		ImGui::PopStyleVar();

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(dragDrop.c_str()))
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

		ImGui::SetCursorScreenPos({childPos.x + childSize.x - buttonAreaWidth, childPos.y + defaultWindowPadding.y});

		if (type == ANM2_NULL)
		{
			const ImguiItem& rectItem = null->isShowRect ? IMGUI_TIMELINE_ITEM_SHOW_RECT : IMGUI_TIMELINE_ITEM_HIDE_RECT;
			if (_imgui_atlas_button(rectItem, self)) null->isShowRect = !null->isShowRect;
			ImGui::SameLine(0.0f, defaultItemSpacing.x);
		}
		
		const ImguiItem& visibleItem = item->isVisible ? IMGUI_TIMELINE_ITEM_VISIBLE : IMGUI_TIMELINE_ITEM_INVISIBLE;
		if (_imgui_atlas_button(visibleItem, self)) item->isVisible = !item->isVisible;

		ImGui::PopStyleVar(2);
		
		_imgui_end_child(); // itemChild
		
		ImGui::PopID();

		index++;
	};

	std::function<void()> timeline_items_child = [&]()
	{
		static s32& animationID = self->reference->animationID;
		s32 index = 0;

		_imgui_begin_child(IMGUI_TIMELINE_ITEMS_CHILD, self);
		ImGui::SetScrollY(scroll.y);

		timeline_item_child({animationID, ANM2_ROOT}, index);

		for (auto& id : std::ranges::reverse_view(animation->layerOrder))
			timeline_item_child({animationID, ANM2_LAYER, id}, index);

		for (auto & [id, null] : animation->nullAnimations)
			timeline_item_child({animationID, ANM2_NULL, id}, index);

		timeline_item_child({animationID, ANM2_TRIGGERS}, index);

		_imgui_end_child(); // IMGUI_TIMELINE_ITEMS_CHILD

		if (isItemSwap)
		{
			Anm2Animation* animation = anm2_animation_from_reference(self->anm2, self->reference);

			imgui_snapshot(self, IMGUI_ACTION_ITEM_SWAP);
			
			switch (swapItemReference.itemType)
			{
				case ANM2_LAYER:
					vector_value_swap(animation->layerOrder, self->reference->itemID, swapItemReference.itemID);
					break;
				case ANM2_NULL:
					map_swap(animation->nullAnimations, self->reference->itemID, swapItemReference.itemID);
					break;
				default: break;
			}

			self->reference->itemID = swapItemReference.itemID;
			anm2_reference_clear(&swapItemReference);
			isItemSwap = false;
		}
	};

	std::function<void(Anm2Reference, s32&)> timeline_item_frames = [&](Anm2Reference reference, s32& index)
	{
		Anm2Item* item = anm2_item_from_reference(self->anm2, &reference);
		if (!item) return;
		Anm2Type& type = reference.itemType;
		if (!self->settings->timelineIsShowUnused && item->frames.empty() && (type == ANM2_LAYER || type == ANM2_NULL)) return;

		ImGui::PushID(index);

		ImDrawList* drawList = ImGui::GetWindowDrawList();

		f32 viewWidth = ImGui::GetWindowContentRegionMax().x - ImGui::GetWindowContentRegionMin().x;
		
		_imgui_begin_child(IMGUI_TIMELINE_ITEM_FRAMES_CHILD.copy({.size = actualFramesSize.x > framesSize.x ? actualFramesSize : framesSize}), self);
		
		ImVec2 startPos  = ImGui::GetCursorPos();      
		ImVec2 cursorPos = ImGui::GetCursorScreenPos();

		if (_imgui_is_window_hovered())
		{
			hoverReference = reference;
			hoverReference.time = frameTime;
			hoverReference.frameIndex = anm2_frame_index_from_time(self->anm2, reference, frameTime);
			self->clipboard->location = hoverReference;
			self->clipboard->type = CLIPBOARD_FRAME;
				
			if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
			{
				*self->reference = reference;
				if (reference.itemType == ANM2_LAYER && reference.itemID != ID_NONE)
					_imgui_spritesheet_editor_set(self, self->anm2->layers[self->reference->itemID].spritesheetID);
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
			drawList->AddImage(self->resources->atlas.id, startPosition, endPosition, ATLAS_UV_ARGS(ATLAS_FRAME));
		}

		ImGui::SetCursorPos(startPos);

		std::function<void(s32, Anm2Frame&)> timeline_item_frame = [&](s32 i, Anm2Frame& frame)
		{
			static s32 frameDelayStart{};
			static f32 frameDelayTimeStart{};
			const bool isModCtrl = ImGui::IsKeyDown(IMGUI_INPUT_CTRL);
			static bool isFrameSwap = false;
			static bool isDrag  = false;

			ImGui::PushID(i);
			reference.frameIndex = i;
			ImVec2 framePos = ImGui::GetCursorPos();
			AtlasType atlas = frame.isInterpolated ? ATLAS_CIRCLE : ATLAS_SQUARE;

			if (type == ANM2_TRIGGERS)
			{
				framePos.x = startPos.x + (frameSize.x * frame.atFrame);
				atlas = ATLAS_TRIGGER;
			}

			ImguiItem frameButton = IMGUI_TIMELINE_FRAMES[type]->copy
			({.isSelected = reference == *self->reference, .size = {frameSize.x * frame.delay, frameSize.y}, .id = i, .atlas = atlas});

			ImGui::SetCursorPos(framePos);
			
			if (_imgui_atlas_button(frameButton, self))
			{
				*self->reference = reference;
				frameDelayStart = frame.delay;
				frameDelayTimeStart = frameTime;
			}

			if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceNoDisableHover))
			{
				if (!isDrag)
				{
					*self->reference = reference;
					frameDelayStart = frame.delay;
					frameDelayTimeStart = frameTime;
					isFrameSwap = false;
					isDrag = true;
				}

				ImGui::SetDragDropPayload(frameButton.drag_drop_get(), &reference, sizeof(Anm2Reference));
				if (!isModCtrl) timeline_item_frame(i, frame);
				ImGui::EndDragDropSource();
			}

			if (isDrag)
			{
				if (Anm2Frame* referenceFrame = anm2_frame_from_reference(self->anm2, self->reference))
				{
					switch (self->reference->itemType)
					{
						case ANM2_TRIGGERS:
						{
							referenceFrame->atFrame = std::max(frameTime, 0);
							for (auto& trigger : animation->triggers.frames)
							{
								if (&trigger == referenceFrame) continue;
								if (trigger.atFrame == referenceFrame->atFrame) 
								{ 
									referenceFrame->atFrame++; 
									break; 
								}
							}
							break;
						}
						case ANM2_ROOT:
						case ANM2_LAYER:
						case ANM2_NULL:
						{
							if (isModCtrl)
								referenceFrame->delay = std::max(frameDelayStart + (s32)(frameTime - frameDelayTimeStart), ANM2_FRAME_NUM_MIN);
							break;
						}
						default: break;
					}
				}
			}

			if (ImGui::BeginDragDropTarget())
			{
				ImGui::AcceptDragDropPayload(frameButton.drag_drop_get());
				ImGui::EndDragDropTarget();
			}

			if (isDrag && ImGui::IsMouseReleased(0))
			{
				if 
				(	!isFrameSwap &&
					*self->reference != hoverReference &&
					self->reference->itemType == hoverReference.itemType
				)
				{
					imgui_snapshot(self, IMGUI_ACTION_FRAME_MOVE);
					if (Anm2Frame* referenceFrame = anm2_frame_from_reference(self->anm2, self->reference))
					{
						Anm2Reference addReference = hoverReference;
						addReference.frameIndex = hoverReference.frameIndex < self->reference->frameIndex ? 
							std::min(0, addReference.frameIndex - 2) : 
							std::min(0, addReference.frameIndex - 1);
						Anm2Frame addFrame = *referenceFrame;
						anm2_frame_remove(self->anm2, self->reference);
						anm2_frame_add(self->anm2, &addFrame, &hoverReference);
						*self->reference = hoverReference;
						hoverReference = Anm2Reference();
					}
				}

				isDrag = false;
			}


			if (i < (s32)item->frames.size() - 1) ImGui::SameLine();

			ImGui::PopID();
		};

		for (auto [i, frame] : std::views::enumerate(item->frames))
			timeline_item_frame(i, frame);

		_imgui_end_child(); // itemFramesChild

		ImGui::PopID();
		
		index++;
	};

	std::function<void()> timeline_frames_child = [&]()
	{
		s32& animationID = self->reference->animationID;
		s32 index = 0;

		_imgui_begin_child(IMGUI_TIMELINE_FRAMES_CHILD, self);
		scroll.x = ImGui::GetScrollX() + scrollDelta.x;
		scroll.y = ImGui::GetScrollY() + scrollDelta.y;
		ImGui::SetScrollX(scroll.x);
		ImGui::SetScrollY(scroll.y);

		timeline_item_frames(Anm2Reference(animationID, ANM2_ROOT), index);

		for (auto& id : std::ranges::reverse_view(animation->layerOrder))
			timeline_item_frames(Anm2Reference(animationID, ANM2_LAYER, id), index);

		for (auto & [id, null] : animation->nullAnimations)
			timeline_item_frames(Anm2Reference(animationID, ANM2_NULL, id), index);

		timeline_item_frames(Anm2Reference(animationID, ANM2_TRIGGERS), index);

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, defaultItemSpacing);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, defaultWindowPadding);
		_imgui_context_menu(self);
		ImGui::PopStyleVar(2);
		
		_imgui_end_child(); // IMGUI_TIMELINE_FRAMES_CHILD
	};

	// In order to set scroll properly timeline_frames_child must be called first
	ImGui::SetCursorPos(ImVec2(IMGUI_TIMELINE_ITEM_SIZE));
	timeline_frames_child();
	ImGui::SetCursorPos(ImVec2());
	
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, defaultItemSpacing);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, defaultWindowPadding);

	_imgui_begin_child(IMGUI_TIMELINE_ITEM_CHILD, self);

	const ImguiItem& unusedItem = self->settings->timelineIsShowUnused ? IMGUI_TIMELINE_SHOW_UNUSED : IMGUI_TIMELINE_HIDE_UNUSED;
	if (_imgui_atlas_button(unusedItem, self)) self->settings->timelineIsShowUnused = !self->settings->timelineIsShowUnused;
	ImGui::PopStyleVar(2);
	
	_imgui_end_child(); // IMGUI_TIMELINE_ITEM_CHILD
	ImGui::SameLine();
	timeline_header();
	
	timeline_items_child();
	
	ImGui::PopStyleVar(2);

	_imgui_end_child(); // IMGUI_TIMELINE_CHILD

	Anm2Frame* frame = anm2_frame_from_reference(self->anm2, self->reference);
	Anm2Item* item = anm2_item_from_reference(self->anm2, self->reference);
	_imgui_begin_child(IMGUI_TIMELINE_ITEM_FOOTER_CHILD, self);
	
	_imgui_button(IMGUI_TIMELINE_ADD_ITEM, self);

	if (imgui_begin_popup_modal(IMGUI_TIMELINE_ADD_ITEM.popup, self, IMGUI_TIMELINE_ADD_ITEM.popupSize))
	{
		static s32 selectedLayerID = ID_NONE;
		static s32 selectedNullID = ID_NONE;
		s32& type = self->settings->timelineAddItemType;

		_imgui_begin_child(IMGUI_TIMELINE_ITEM_PROPERTIES_TYPE_CHILD, self);

		_imgui_radio_button(IMGUI_TIMELINE_ITEM_PROPERTIES_LAYER, self, type);
		_imgui_radio_button(IMGUI_TIMELINE_ITEM_PROPERTIES_NULL, self, type);

		_imgui_end_child(); // IMGUI_TIMELINE_ITEM_PROPERTIES_TYPE_CHILD
		
		_imgui_begin_child(IMGUI_TIMELINE_ITEM_PROPERTIES_ITEMS_CHILD, self);
		
		switch (type)
		{
			case ANM2_LAYER:
			default:
			{
				for (auto & [id, layer] : self->anm2->layers)
				{
					ImGui::PushID(id);

					ImguiItem layerItem = IMGUI_LAYER.copy
					({
						.isSelected = selectedLayerID == id,
						.label = std::format(IMGUI_LAYER_FORMAT, id, layer.name),
						.id = id
					});
					if (_imgui_atlas_selectable(layerItem, self)) selectedLayerID = id;

					ImGui::PopID();
				};
				break;
			}			
			case ANM2_NULL:
			{
				for (auto & [id, null] : self->anm2->nulls)
				{
					ImGui::PushID(id);

					ImguiItem nullItem = IMGUI_NULL.copy
					({
						.isSelected = selectedNullID == id,
						.label = std::format(IMGUI_NULL_FORMAT, id, null.name),
						.id = id
					});
					if (_imgui_atlas_selectable(nullItem, self)) selectedNullID = id;

					ImGui::PopID();
				};
				break;
			}
		}

		_imgui_end_child(); // IMGUI_TIMELINE_ITEM_PROPERTIES_ITEMS_CHILD

		_imgui_begin_child(IMGUI_TIMELINE_ITEM_PROPERTIES_OPTIONS_CHILD, self);
	
		if (self->anm2->layers.size() == 0) selectedLayerID = ID_NONE;
		if (self->anm2->nulls.size() == 0) selectedNullID = ID_NONE;

		bool isDisabled = 				 type == ANM2_NONE || 
		(type == ANM2_LAYER && selectedLayerID == ID_NONE) || 
		(type == ANM2_NULL && selectedNullID == ID_NONE);
		
		if (_imgui_button(IMGUI_TIMELINE_ITEM_PROPERTIES_CONFIRM.copy({isDisabled}), self))
		{
			switch (type)
			{
				case ANM2_LAYER: 
					anm2_animation_layer_animation_add(animation, selectedLayerID);
					*self->reference = {self->reference->animationID, ANM2_LAYER, selectedLayerID};
					break;
				case ANM2_NULL: 
					anm2_animation_null_animation_add(animation, selectedNullID); 
					*self->reference = {self->reference->animationID, ANM2_NULL, selectedNullID};
					break;
				default: break;
			}
			 
			imgui_close_current_popup(self);
		}

		if (_imgui_button(IMGUI_POPUP_CANCEL, self)) imgui_close_current_popup(self);
		
		_imgui_end_child(); // IMGUI_TIMELINE_ITEM_PROPERTIES_OPTIONS_CHILD
				
		imgui_end_popup(self);
	}

	if (_imgui_button(IMGUI_TIMELINE_REMOVE_ITEM.copy({!item || itemType == ANM2_ROOT || itemType == ANM2_TRIGGERS}), self))
	{
		switch (itemType)
		{
			case ANM2_LAYER: anm2_animation_layer_animation_remove(animation, itemID); break;
			case ANM2_NULL:  anm2_animation_null_animation_remove(animation, itemID);  break;
			default: break;
		}

		anm2_reference_item_clear(self->reference);
	}
	
	_imgui_end_child(); //IMGUI_TIMELINE_FOOTER_ITEM_CHILD
	
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
	ImGui::SameLine();
	ImGui::PopStyleVar();
	
	_imgui_begin_child(IMGUI_TIMELINE_OPTIONS_FOOTER_CHILD, self);
	
	if (_imgui_button(self->preview->isPlaying ? IMGUI_PAUSE : IMGUI_PLAY, self))
	{
		if (!self->preview->isPlaying && time >= animation->frameNum - 1) 
			time = 0.0f;
		self->preview->isPlaying = !self->preview->isPlaying;
	}

	if (_imgui_button(IMGUI_ADD_FRAME.copy({!item}), self))
	{
		Anm2Reference frameReference = *self->reference;
		frameReference.frameIndex = item->frames.empty() ? 0 : std::clamp(frameReference.frameIndex, 0, (s32)(item->frames.size() - 1));
		Anm2Frame* addFrame = anm2_frame_from_reference(self->anm2, &frameReference);
		anm2_frame_add(self->anm2, addFrame, &frameReference);
	}

	if(_imgui_button(IMGUI_REMOVE_FRAME.copy({!frame}), self))
	{
		anm2_frame_remove(self->anm2, self->reference);
		anm2_reference_frame_clear(self->reference);
	}

	_imgui_button(IMGUI_BAKE.copy({!frame || itemType == ANM2_TRIGGERS}), self);

	if (imgui_begin_popup_modal(IMGUI_BAKE.popup, self, IMGUI_BAKE.popupSize))
	{
		static s32& interval = self->settings->bakeInterval;
		static bool& isRoundScale = self->settings->bakeIsRoundScale;
		static bool& isRoundRotation = self->settings->bakeIsRoundRotation;
		
		_imgui_begin_child(IMGUI_BAKE_CHILD, self);

		_imgui_input_int(IMGUI_BAKE_INTERVAL.copy({.max = frame->delay}), self, interval);
		_imgui_checkbox(IMGUI_BAKE_ROUND_SCALE, self, isRoundScale);
		_imgui_checkbox(IMGUI_BAKE_ROUND_ROTATION, self, isRoundRotation);

		if (_imgui_button(IMGUI_BAKE_CONFIRM, self))
		{
			anm2_frame_bake(self->anm2, self->reference, interval, isRoundScale, isRoundRotation);
			imgui_close_current_popup(self);
		}

		if (_imgui_button(IMGUI_POPUP_CANCEL, self)) imgui_close_current_popup(self);

		_imgui_end_child(); //IMGUI_BAKE_CHILD)
			
		imgui_end_popup(self);
	}
	
	if (_imgui_button(IMGUI_FIT_ANIMATION_LENGTH, self)) anm2_animation_length_set(animation);

	_imgui_input_int(IMGUI_ANIMATION_LENGTH, self, animation->frameNum);
	_imgui_input_int(IMGUI_FPS, self, self->anm2->fps);
	_imgui_checkbox(IMGUI_LOOP, self, animation->isLoop);
	_imgui_selectable_input_text(IMGUI_CREATED_BY.copy({.label = self->anm2->createdBy}), self, self->anm2->createdBy);

	_imgui_end_child(); // IMGUI_TIMELINE_FOOTER_OPTIONS_CHILD
	
	_imgui_end(); // IMGUI_TIMELINE
}

static void _imgui_onionskin(Imgui* self)
{
    IMGUI_BEGIN_OR_RETURN(IMGUI_ONIONSKIN, self);

    static auto& isEnabled         = self->settings->onionskinIsEnabled;
    static auto& beforeCount       = self->settings->onionskinBeforeCount;
    static auto& afterCount        = self->settings->onionskinAfterCount;
    static auto& beforeColorOffset = self->settings->onionskinBeforeColorOffset;
    static auto& afterColorOffset  = self->settings->onionskinAfterColorOffset;
    static auto& drawOrder = self->settings->onionskinDrawOrder;

    _imgui_checkbox(IMGUI_ONIONSKIN_ENABLED, self, isEnabled);

    auto onionskin_section = [&](auto& text, auto& count, auto& colorOffset)
    {
        ImGui::PushID(text.label.c_str());
        _imgui_text(text, self);
        _imgui_input_int(IMGUI_ONIONSKIN_COUNT.copy({!isEnabled}), self, count);
		_imgui_color_edit3(IMGUI_ONIONSKIN_COLOR_OFFSET.copy({!isEnabled}), self, colorOffset);
        ImGui::PopID();
    };

    onionskin_section(IMGUI_ONIONSKIN_BEFORE, beforeCount, beforeColorOffset);
	onionskin_section(IMGUI_ONIONSKIN_AFTER, afterCount,  afterColorOffset);
	
	ImGui::Separator();

	_imgui_text(IMGUI_ONIONSKIN_DRAW_ORDER, self);
	_imgui_radio_button(IMGUI_ONIONSKIN_BELOW.copy({!isEnabled}), self, drawOrder);
	_imgui_radio_button(IMGUI_ONIONSKIN_ABOVE.copy({!isEnabled}), self, drawOrder);

    _imgui_end(); // IMGUI_ONIONSKIN
}

static void _imgui_taskbar(Imgui* self)
{
	static ImguiPopupState exitConfirmState = IMGUI_POPUP_STATE_CLOSED;

	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImguiItem taskbar = IMGUI_TASKBAR;
	ImGui::SetNextWindowSize({viewport->Size.x, IMGUI_TASKBAR.size.y});
	ImGui::SetNextWindowPos(viewport->Pos);
	_imgui_begin(taskbar, self);

	Anm2Animation* animation = anm2_animation_from_reference(self->anm2, self->reference);
	Anm2Item* item = anm2_item_from_reference(self->anm2, self->reference);

	_imgui_selectable(IMGUI_FILE, self);
	
	if (imgui_begin_popup(IMGUI_FILE.popup, self))
	{
		_imgui_selectable(IMGUI_NEW, self);
		_imgui_selectable(IMGUI_OPEN, self);
		_imgui_selectable(IMGUI_SAVE.copy({self->anm2->path.empty()}), self);
		_imgui_selectable(IMGUI_SAVE_AS.copy({self->anm2->path.empty()}), self);
		_imgui_selectable(IMGUI_EXPLORE_ANM2_LOCATION.copy({self->anm2->path.empty()}), self);
		_imgui_selectable(IMGUI_EXIT, self);
		imgui_end_popup(self);
	}

	if (self->dialog->isSelected && self->dialog->type == DIALOG_ANM2_OPEN)
	{
		_imgui_anm2_open(self, self->dialog->path);
		dialog_reset(self->dialog);
	}			

	if (self->dialog->isSelected && self->dialog->type == DIALOG_ANM2_SAVE)
	{
		anm2_serialize(self->anm2, self->dialog->path);
		window_title_from_path_set(self->window, self->dialog->path);
		imgui_log_push(self, std::format(IMGUI_LOG_FILE_SAVE_FORMAT, self->dialog->path));
		dialog_reset(self->dialog);
	}

	if (self->isTryQuit) imgui_open_popup(IMGUI_EXIT_CONFIRMATION.label);

	_imgui_confirm_popup(IMGUI_EXIT_CONFIRMATION, self, &exitConfirmState);

	switch (exitConfirmState)
	{
		case IMGUI_POPUP_STATE_CONFIRM: self->isQuit = true; break;
		case IMGUI_POPUP_STATE_CANCEL: self->isTryQuit = false; break;
		default: break;
	}

	_imgui_selectable(IMGUI_WIZARD.copy({}), self);
	
	if (imgui_begin_popup(IMGUI_WIZARD.popup, self))
	{
		_imgui_selectable(IMGUI_GENERATE_ANIMATION_FROM_GRID.copy({!item || (self->reference->itemType != ANM2_LAYER)}), self);
		_imgui_selectable(IMGUI_CHANGE_ALL_FRAME_PROPERTIES.copy({!item}), self);
		_imgui_selectable(IMGUI_SCALE_ANM2.copy({self->anm2->animations.empty()}), self);
		_imgui_selectable(IMGUI_RENDER_ANIMATION.copy({!animation}), self);
	
		imgui_end_popup(self);
	}

	if (imgui_begin_popup_modal(IMGUI_GENERATE_ANIMATION_FROM_GRID.popup, self, IMGUI_GENERATE_ANIMATION_FROM_GRID.popupSize))
	{
		static auto& startPosition = self->settings->generateStartPosition;
		static auto& size = self->settings->generateSize;
		static auto& pivot = self->settings->generatePivot;
		static auto& rows = self->settings->generateRows;
		static auto& columns = self->settings->generateColumns;
		static auto& count = self->settings->generateCount;
		static auto& delay = self->settings->generateDelay;
		static f32& time = self->generatePreview->time;

		_imgui_begin_child(IMGUI_GENERATE_ANIMATION_FROM_GRID_OPTIONS_CHILD, self);
		_imgui_input_int2(IMGUI_GENERATE_ANIMATION_FROM_GRID_START_POSITION, self, startPosition);
		_imgui_input_int2(IMGUI_GENERATE_ANIMATION_FROM_GRID_SIZE, self, size);
		_imgui_input_int2(IMGUI_GENERATE_ANIMATION_FROM_GRID_PIVOT, self, pivot);
		_imgui_input_int(IMGUI_GENERATE_ANIMATION_FROM_GRID_ROWS, self, rows);
		_imgui_input_int(IMGUI_GENERATE_ANIMATION_FROM_GRID_COLUMNS, self, columns);
		_imgui_input_int(IMGUI_GENERATE_ANIMATION_FROM_GRID_COUNT.copy({.max = rows * columns}), self, count);
		_imgui_input_int(IMGUI_GENERATE_ANIMATION_FROM_GRID_DELAY, self, delay);
		_imgui_end_child(); //IMGUI_GENERATE_ANIMATION_FROM_GRID_OPTIONS_CHILD 
		
		ImGui::SameLine();

		_imgui_begin_child(IMGUI_GENERATE_ANIMATION_FROM_GRID_PREVIEW_CHILD, self);
		
		generate_preview_draw(self->generatePreview);
		ImGui::Image(self->generatePreview->canvas.framebuffer, GENERATE_PREVIEW_SIZE);

		_imgui_begin_child(IMGUI_GENERATE_ANIMATION_FROM_GRID_SLIDER_CHILD, self);
		_imgui_slider_float(IMGUI_GENERATE_ANIMATION_FROM_GRID_SLIDER, self, time);
		_imgui_end_child(); // IMGUI_GENERATE_ANIMATION_FROM_GRID_SLIDER_CHILD

		_imgui_end_child(); //IMGUI_GENERATE_ANIMATION_FROM_GRID_PREVIEW_CHILD 
	
		_imgui_begin_child(IMGUI_FOOTER_CHILD, self);
		if (_imgui_button(IMGUI_GENERATE_ANIMATION_FROM_GRID_GENERATE, self))
		{
			anm2_generate_from_grid(self->anm2, self->reference, startPosition, size, pivot, columns, count, delay);
			imgui_close_current_popup(self);
		}
		if (_imgui_button(IMGUI_POPUP_CANCEL, self)) imgui_close_current_popup(self);
		_imgui_end_child(); // IMGUI_FOOTER_CHILD

		imgui_end_popup(self);
	}

	if (imgui_begin_popup_modal(IMGUI_CHANGE_ALL_FRAME_PROPERTIES.popup, self, IMGUI_CHANGE_ALL_FRAME_PROPERTIES.popupSize))
	{
		static auto& isCrop         = self->settings->changeIsCrop;
		static auto& isSize         = self->settings->changeIsSize;
		static auto& isPosition     = self->settings->changeIsPosition;
		static auto& isPivot        = self->settings->changeIsPivot;
		static auto& isScale        = self->settings->changeIsScale;
		static auto& isRotation     = self->settings->changeIsRotation;
		static auto& isColorOffset  = self->settings->changeIsColorOffset;
		static auto& isTint         = self->settings->changeIsTint;
		static auto& isVisibleSet   = self->settings->changeIsVisibleSet;
		static auto& isInterpolatedSet = self->settings->changeIsInterpolatedSet;
		static auto& crop           = self->settings->changeCrop;
		static auto& size           = self->settings->changeSize;
		static auto& position       = self->settings->changePosition;
		static auto& pivot          = self->settings->changePivot;
		static auto& scale          = self->settings->changeScale;
		static auto& rotation       = self->settings->changeRotation;
		static auto& isDelay        = self->settings->changeIsDelay;
		static auto& delay          = self->settings->changeDelay;
		static auto& tint           = self->settings->changeTint;
		static auto& colorOffset    = self->settings->changeColorOffset;
		static auto& isVisible      = self->settings->changeIsVisible;
		static auto& isInterpolated = self->settings->changeIsInterpolated;
		static auto& isFromSelectedFrame = self->settings->changeIsFromSelectedFrame;
		static auto& numberFrames = self->settings->changeNumberFrames;
		s32 start = std::max(self->reference->frameIndex, 0);
		s32 max = isFromSelectedFrame ? 
		std::max(ANM2_FRAME_NUM_MIN, (s32)item->frames.size() - start) : (s32)item->frames.size();

		auto change_frames = [&](Anm2ChangeType type)
		{
			anm2_item_frame_set
			(
				self->anm2, self->reference,
				Anm2FrameChange
				{
					isVisibleSet      ? std::optional{isVisible}      : std::nullopt,
					isInterpolatedSet ? std::optional{isInterpolated} : std::nullopt,
					isRotation 		  ? std::optional{rotation}       : std::nullopt,
					isDelay        	  ? std::optional{delay}          : std::nullopt,
					isCrop         	  ? std::optional{crop}           : std::nullopt,
					isPivot        	  ? std::optional{pivot}          : std::nullopt,
					isPosition     	  ? std::optional{position}       : std::nullopt,
					isSize         	  ? std::optional{size}           : std::nullopt,
					isScale        	  ? std::optional{scale}          : std::nullopt,
					isColorOffset  	  ? std::optional{colorOffset}    : std::nullopt,
					isTint         	  ? std::optional{tint}           : std::nullopt
				},
				type, start, numberFrames
			);

			imgui_close_current_popup(self);
		};

		_imgui_begin_child(IMGUI_CHANGE_ALL_FRAME_PROPERTIES_CHILD, self);
		_imgui_checkbox_drag_float2(IMGUI_FRAME_PROPERTIES_CROP.copy({!isCrop}), self, crop, isCrop);
		_imgui_checkbox_drag_float2(IMGUI_FRAME_PROPERTIES_SIZE.copy({!isSize}), self, size, isSize);
		_imgui_checkbox_drag_float2(IMGUI_FRAME_PROPERTIES_POSITION.copy({!isPosition}), self, position, isPosition);
		_imgui_checkbox_drag_float2(IMGUI_FRAME_PROPERTIES_PIVOT.copy({!isPivot}), self, pivot, isPivot);
		_imgui_checkbox_drag_float2(IMGUI_FRAME_PROPERTIES_SCALE.copy({!isScale}), self, scale, isScale);
		_imgui_checkbox_drag_float(IMGUI_FRAME_PROPERTIES_ROTATION.copy({!isRotation}), self, rotation, isRotation);
		_imgui_checkbox_input_int(IMGUI_FRAME_PROPERTIES_DELAY.copy({!isDelay}), self, delay, isDelay);
		_imgui_checkbox_color_edit4(IMGUI_FRAME_PROPERTIES_TINT.copy({!isTint}), self, tint, isTint);
		_imgui_checkbox_color_edit3(IMGUI_FRAME_PROPERTIES_COLOR_OFFSET.copy({!isColorOffset}), self, colorOffset, isColorOffset);
		_imgui_checkbox_checkbox(IMGUI_FRAME_PROPERTIES_VISIBLE.copy({!isVisibleSet}), self, isVisible, isVisibleSet);
		ImGui::NewLine();
		_imgui_checkbox_checkbox(IMGUI_FRAME_PROPERTIES_INTERPOLATED.copy({!isInterpolatedSet}), self, isInterpolated, isInterpolatedSet);
		_imgui_end_child(); // IMGUI_FOOTER_CHILD

		_imgui_begin_child(IMGUI_CHANGE_ALL_FRAME_PROPERTIES_SETTINGS_CHILD, self);
		_imgui_text(IMGUI_CHANGE_ALL_FRAME_PROPERTIES_SETTINGS, self);
		_imgui_checkbox(IMGUI_CHANGE_ALL_FRAME_PROPERTIES_FROM_SELECTED_FRAME, self, isFromSelectedFrame);
		_imgui_input_int(IMGUI_CHANGE_ALL_FRAME_PROPERTIES_NUMBER_FRAMES.copy({.max = max, .value = max}), self, numberFrames);
		_imgui_end_child(); // IMGUI_CHANGE_ALL_FRAME_PROPERTIES_SETTINGS_CHILD

		_imgui_begin_child(IMGUI_FOOTER_CHILD, self);
		if (_imgui_button(IMGUI_CHANGE_ALL_FRAME_PROPERTIES_ADD, self)) change_frames(ANM2_CHANGE_ADD);
		if (_imgui_button(IMGUI_CHANGE_ALL_FRAME_PROPERTIES_SUBTRACT, self)) change_frames(ANM2_CHANGE_SUBTRACT);
		if (_imgui_button(IMGUI_CHANGE_ALL_FRAME_PROPERTIES_SET, self)) change_frames(ANM2_CHANGE_SET);
		if (_imgui_button(IMGUI_CHANGE_ALL_FRAME_PROPERTIES_CANCEL, self)) imgui_close_current_popup(self);
		_imgui_end_child(); // IMGUI_FOOTER_CHILD
		
		imgui_end_popup(self);
	}

	if (imgui_begin_popup_modal(IMGUI_SCALE_ANM2.popup, self, IMGUI_SCALE_ANM2.popupSize))
	{
		_imgui_begin_child(IMGUI_SCALE_ANM2_OPTIONS_CHILD, self);
		_imgui_input_float(IMGUI_SCALE_ANM2_VALUE, self, self->settings->scaleValue);	
		_imgui_end_child(); // IMGUI_SCALE_ANM2_OPTIONS_CHILD
		
		_imgui_begin_child(IMGUI_FOOTER_CHILD, self);
		if (_imgui_button(IMGUI_SCALE_ANM2_SCALE, self)) 
		{
			anm2_scale(self->anm2, self->settings->scaleValue);
			imgui_close_current_popup(self);
		}
		if (_imgui_button(IMGUI_POPUP_CANCEL, self)) imgui_close_current_popup(self);
		_imgui_end_child(); // IMGUI_FOOTER_CHILD

		imgui_end_popup(self);
	}

	if (imgui_begin_popup_modal(IMGUI_RENDER_ANIMATION.popup, self, IMGUI_RENDER_ANIMATION.popupSize))
	{
		static DialogType& dialogType = self->dialog->type;
		static bool& dialogIsSelected = self->dialog->isSelected;
		static s32& type = self->settings->renderType;
		static f32& scale = self->settings->renderScale;
		static bool& isUseAnimationBounds = self->settings->renderIsUseAnimationBounds;
		static std::string& dialogPath = self->dialog->path;
		static std::string& ffmpegPath = self->settings->renderFFmpegPath;
		static std::string& format = self->settings->renderFormat;
		static std::string& path = self->settings->renderPath;

		_imgui_begin_child(IMGUI_RENDER_ANIMATION_CHILD, self);

		if (_imgui_atlas_button(IMGUI_RENDER_ANIMATION_LOCATION_BROWSE, self))
			dialog_render_path_set(self->dialog, (RenderType)type);

		if (dialogIsSelected && (dialogType == DIALOG_RENDER_PATH_SET))
		{
			path = path_extension_change(dialogPath, RENDER_EXTENSIONS[type]);
			dialog_reset(self->dialog);
		}

		_imgui_input_text(IMGUI_RENDER_ANIMATION_LOCATION, self, path);
		
		if (_imgui_atlas_button(IMGUI_RENDER_ANIMATION_FFMPEG_BROWSE, self)) 
			dialog_ffmpeg_path_set(self->dialog);

		if (dialogIsSelected && dialogType == DIALOG_FFMPEG_PATH_SET)
		{
			ffmpegPath = self->dialog->path;
			dialog_reset(self->dialog);
		}
		
		_imgui_input_text(IMGUI_RENDER_ANIMATION_FFMPEG_PATH, self, ffmpegPath);
		_imgui_combo(IMGUI_RENDER_ANIMATION_OUTPUT, self, &type);
		_imgui_input_text(IMGUI_RENDER_ANIMATION_FORMAT.copy({type != RENDER_PNG}), self, format);
		_imgui_checkbox(IMGUI_RENDER_ANIMATION_IS_USE_ANIMATION_BOUNDS, self, isUseAnimationBounds);
		_imgui_input_float(IMGUI_RENDER_ANIMATION_SCALE.copy({!isUseAnimationBounds}), self, scale);

		_imgui_end_child(); // IMGUI_RENDER_ANIMATION_CHILD
		
		_imgui_begin_child(IMGUI_RENDER_ANIMATION_FOOTER_CHILD, self);

		if (_imgui_button(IMGUI_RENDER_ANIMATION_CONFIRM, self))
		{
			bool isRenderStart = true;

			if (!std::filesystem::exists(ffmpegPath))
			{
				imgui_log_push(self, IMGUI_LOG_RENDER_ANIMATION_FFMPEG_PATH_ERROR);
				isRenderStart = false;
			}

			if (isRenderStart)
			{
				switch (self->settings->renderType)
				{
					case RENDER_PNG:
						if (!std::filesystem::is_directory(path))
						{
							imgui_log_push(self, IMGUI_LOG_RENDER_ANIMATION_DIRECTORY_ERROR);
							isRenderStart = false;
						}
						break;
					case RENDER_GIF:
					case RENDER_WEBM:
					case RENDER_MP4:
						if (!path_is_valid(path))
						{
							imgui_log_push(self, IMGUI_LOG_RENDER_ANIMATION_PATH_ERROR);
							isRenderStart = false;
						}
					default:
						break;
				}
			}

			if (isRenderStart)
				preview_render_start(self->preview);
			else
				self->preview->isRenderCancelled = true;
			
			imgui_close_current_popup(self);
		}
		
		if (_imgui_button(IMGUI_POPUP_CANCEL, self))
			imgui_close_current_popup(self);

		_imgui_end_child(); // IMGUI_RENDER_ANIMATION_FOOTER_CHILD
			
		imgui_end_popup(self);
	}

	if (imgui_begin_popup_modal(IMGUI_RENDER_ANIMATION_CONFIRM.popup, self, IMGUI_RENDER_ANIMATION_CONFIRM.popupSize))
	{
		static s32& type = self->settings->renderType;
		static std::string& format = self->settings->renderFormat;

		auto rendering_end = [&]()
		{
			preview_render_end(self->preview);
			imgui_close_current_popup(self);
		};
		
		std::vector<Texture>& frames = self->preview->renderFrames;
		std::string path = std::string(self->settings->renderPath.c_str());
		
		if (self->preview->isRenderCancelled)
		{
			rendering_end();
			self->preview->isRenderCancelled = false;
		}

		if (!animation)
		{
			imgui_log_push(self, IMGUI_LOG_RENDER_ANIMATION_NO_ANIMATION_ERROR);
			rendering_end();
		}

		_imgui_begin_child(IMGUI_RENDERING_ANIMATION_CHILD, self);

		f32 progress = self->preview->time / (animation->frameNum - 1);
		ImGui::ProgressBar(progress);

		_imgui_text(IMGUI_RENDERING_ANIMATION_INFO, self);

		_imgui_end_child(); //IMGUI_RENDERING_ANIMATION_CHILD

		if (_imgui_button(IMGUI_RENDERING_ANIMATION_CANCEL, self))
			self->preview->isRenderCancelled = true;

		if (self->preview->isRenderFinished && frames.empty())
		{
			imgui_log_push(self, IMGUI_LOG_RENDER_ANIMATION_NO_FRAMES_ERROR);
			rendering_end();
		}
		
		if (self->preview->isRenderFinished)
		{
			switch (type)
			{
				case RENDER_PNG:
				{
					std::filesystem::path workingPath = std::filesystem::current_path();
					std::filesystem::current_path(path);

					for (auto [i, frame] : std::views::enumerate(frames))
					{
						std::string framePath = std::vformat(format, std::make_format_args(i));
						framePath = path_extension_change(framePath, RENDER_EXTENSIONS[type]);
						if (!frame.isInvalid) texture_from_gl_write(&frame, framePath);
					}

					std::filesystem::current_path(workingPath);
					imgui_log_push(self, std::format(IMGUI_LOG_RENDER_ANIMATION_FRAMES_SAVE_FORMAT, path));
					break;
				}
				case RENDER_GIF:
				case RENDER_WEBM:
				case RENDER_MP4:
				{
					std::string ffmpegPath = std::string(self->settings->renderFFmpegPath.c_str());
					path = path_extension_change(path, RENDER_EXTENSIONS[self->settings->renderType]);
					
					if (ffmpeg_render(ffmpegPath, path, frames, self->preview->canvas.size, self->anm2->fps, (RenderType)type))
						imgui_log_push(self, std::format(IMGUI_LOG_RENDER_ANIMATION_SAVE_FORMAT, path));
					else
						imgui_log_push(self, std::format(IMGUI_LOG_RENDER_ANIMATION_FFMPEG_ERROR, path));
					break;
				}
				default:
					break;
			}
			
			rendering_end();
		}

		imgui_end_popup(self);
	}

	_imgui_selectable(IMGUI_PLAYBACK.copy({}), self);

	if (imgui_begin_popup(IMGUI_PLAYBACK.popup, self, IMGUI_PLAYBACK.popupSize))
	{
		_imgui_checkbox_selectable(IMGUI_ALWAYS_LOOP, self, self->settings->playbackIsLoop);
		_imgui_checkbox_selectable(IMGUI_CLAMP_PLAYHEAD, self, self->settings->playbackIsClampPlayhead);
		imgui_end_popup(self);
	}

	_imgui_selectable(IMGUI_SETTINGS.copy({}), self);

	if (imgui_begin_popup(IMGUI_SETTINGS.popup, self, IMGUI_SETTINGS.popupSize))
	{
		if (_imgui_checkbox_selectable(IMGUI_VSYNC, self, self->settings->isVsync)) window_vsync_set(self->settings->isVsync);
		_imgui_selectable(IMGUI_HOTKEYS, self);
		if (_imgui_selectable(IMGUI_DEFAULT_SETTINGS, self)) *self->settings = Settings();
		imgui_end_popup(self);
	}

	if (imgui_begin_popup_modal(IMGUI_HOTKEYS.popup, self, IMGUI_HOTKEYS.popupSize))
	{
		_imgui_begin_child(IMGUI_HOTKEYS_CHILD, self);

		if (_imgui_begin_table(IMGUI_HOTKEYS_TABLE, self))
		{
			static s32 selectedIndex = INDEX_NONE;

			_imgui_table_setup_column(IMGUI_HOTKEYS_FUNCTION);
			_imgui_table_setup_column(IMGUI_HOTKEYS_HOTKEY);
			_imgui_table_headers_row();

			for (s32 i = 0; i < HOTKEY_COUNT; i++)
			{
				if (!SETTINGS_HOTKEY_MEMBERS[i]) continue;

				bool isSelected = selectedIndex == i;
    			const char* string = HOTKEY_STRINGS[i];
    			std::string* settingString = &(self->settings->*SETTINGS_HOTKEY_MEMBERS[i]);
    			std::string chordString = isSelected ? IMGUI_HOTKEY_CHANGE : *settingString;
	
				ImGui::PushID(i);
				_imgui_table_next_row();
				_imgui_table_set_column_index(0);
				ImGui::TextUnformatted(string);
				_imgui_table_set_column_index(1);

				if (ImGui::Selectable(chordString.c_str(), isSelected)) selectedIndex = i;
				ImGui::PopID();

				if (isSelected)
				{
					ImGuiKeyChord chord = IMGUI_CHORD_NONE;

					if (ImGui::IsKeyDown(ImGuiMod_Ctrl))  chord |= ImGuiMod_Ctrl;
					if (ImGui::IsKeyDown(ImGuiMod_Shift)) chord |= ImGuiMod_Shift;
					if (ImGui::IsKeyDown(ImGuiMod_Alt))   chord |= ImGuiMod_Alt;
					if (ImGui::IsKeyDown(ImGuiMod_Super)) chord |= ImGuiMod_Super;

					for (auto& [_, key] : IMGUI_KEY_MAP)
					{
						if (ImGui::IsKeyPressed(key))
						{
							chord |= key;
							imgui_hotkey_chord_registry()[i] = chord;
							*settingString = imgui_string_from_chord_get(chord);
							selectedIndex = INDEX_NONE;
							break;
						}
					}
				}
			}

			_imgui_end_table();
		}

		_imgui_end_child(); // IMGUI_HOTKEYS_CHILD;

		_imgui_begin_child(IMGUI_HOTKEYS_OPTIONS_CHILD, self);
		if (_imgui_button(IMGUI_HOTKEYS_CONFIRM, self)) imgui_close_current_popup(self);
		_imgui_end_child(); // IMGUI_HOTKEYS_OPTIONS_CHILD
		
		imgui_end_popup(self);
	}
	
	_imgui_end();
}

static void _imgui_tools(Imgui* self)
{
	ImGuiStyle style = ImGui::GetStyle();

	IMGUI_BEGIN_OR_RETURN(IMGUI_TOOLS, self);

	f32 availableWidth = ImGui::GetContentRegionAvail().x;
	f32 usedWidth = style.FramePadding.x;

	for (s32 i = 0; i < TOOL_COUNT; i++)
	{
		ImguiItem item = *IMGUI_TOOL_ITEMS[i];

		if (i > 0 && usedWidth + ImGui::GetItemRectSize().x < availableWidth)
			ImGui::SameLine();
		else
			usedWidth = 0;

		item.isSelected = self->settings->tool == (ToolType)i;
			
		switch ((ToolType)i)
		{
			case TOOL_UNDO: item.isDisabled = self->snapshots->undoStack.is_empty(); break;
			case TOOL_REDO: item.isDisabled = self->snapshots->redoStack.is_empty(); break;
			default: break;
		}

		if (i != TOOL_COLOR)
			_imgui_atlas_button(item, self);
		else
			_imgui_color_edit4(item, self, self->settings->toolColor);
	
		usedWidth += ImGui::GetItemRectSize().x + style.ItemSpacing.x;
	}

	_imgui_end(); // IMGUI_TOOLS
}

static void _imgui_layers(Imgui* self)
{
	static s32 selectedLayerID = ID_NONE;

	IMGUI_BEGIN_OR_RETURN(IMGUI_LAYERS, self);
	_imgui_no_anm2_path_check(self);
	
	ImVec2 size = ImGui::GetContentRegionAvail();

	_imgui_begin_child(IMGUI_LAYERS_CHILD.copy({.size = {size.x, size.y - IMGUI_FOOTER_CHILD.size.y}}), self);
	ImGui::SetScrollX(0.0f);

	for (auto & [id, layer] : self->anm2->layers)
	{
		ImGui::PushID(id);

		ImguiItem layerItem = IMGUI_LAYER.copy
		({
			.isSelected = selectedLayerID == id,
			.label = std::format(IMGUI_LAYER_FORMAT, id, layer.name),
			.size = {ImGui::GetContentRegionAvail().x - (IMGUI_LAYER_SPRITESHEET_ID.size.x * 2.0f), 0},
			.id = id
		});
		if (_imgui_atlas_selectable_input_text(layerItem, self, layer.name)) selectedLayerID = id;

		ImGui::SameLine();

		ImguiItem spritesheetItem = IMGUI_LAYER_SPRITESHEET_ID.copy
		({
			.isSelected = selectedLayerID == id,
			.label = std::format(IMGUI_LAYER_FORMAT, id, layer.name),
			.id = id
		});	
		_imgui_atlas_selectable_input_int(spritesheetItem, self, layer.spritesheetID);
	
		ImGui::PopID();
	};

	_imgui_end_child(); // layersChild
	
	_imgui_begin_child(IMGUI_FOOTER_CHILD, self);
	
	if (_imgui_button(IMGUI_LAYER_ADD.copy({self->anm2->path.empty()}), self))
		selectedLayerID = anm2_layer_add(self->anm2);

	if (_imgui_button(IMGUI_LAYER_REMOVE.copy({selectedLayerID == ID_NONE}), self))
	{
		anm2_layer_remove(self->anm2, selectedLayerID);
		selectedLayerID = ID_NONE;
	}
	
	_imgui_end_child(); // IMGUI_FOOTER_CHILD
	_imgui_end(); // IMGUI_LAYERS
}

static void _imgui_nulls(Imgui* self)
{
	static s32 selectedNullID = ID_NONE;

	IMGUI_BEGIN_OR_RETURN(IMGUI_NULLS, self);
	_imgui_no_anm2_path_check(self);
	
	ImVec2 size = ImGui::GetContentRegionAvail();

	_imgui_begin_child(IMGUI_NULLS_CHILD.copy({.size = {size.x, size.y - IMGUI_FOOTER_CHILD.size.y}}), self);

	for (auto & [id, null] : self->anm2->nulls)
	{
		ImGui::PushID(id);

		ImguiItem nullItem = IMGUI_NULL.copy
		({
			.isSelected = selectedNullID == id,
			.label = std::format(IMGUI_NULL_FORMAT, id, null.name),
			.id = id
		});

		if (_imgui_atlas_selectable_input_text(nullItem, self, null.name)) selectedNullID = id;
		
		ImGui::PopID();
	};

	_imgui_end_child(); // nullsChild
	
	_imgui_begin_child(IMGUI_FOOTER_CHILD, self);
	
	if (_imgui_button(IMGUI_NULL_ADD.copy({self->anm2->path.empty()}), self))
		selectedNullID = anm2_null_add(self->anm2);

	if (_imgui_button(IMGUI_NULL_REMOVE.copy({selectedNullID == ID_NONE}), self))
	{
		anm2_null_remove(self->anm2, selectedNullID);
		selectedNullID = ID_NONE;
	}
	
	_imgui_end_child(); // IMGUI_FOOTER_CHILD
	_imgui_end(); // IMGUI_NULLS
}

static void _imgui_animations(Imgui* self)
{
	IMGUI_BEGIN_OR_RETURN(IMGUI_ANIMATIONS, self);
	_imgui_no_anm2_path_check(self);
	
	ImVec2 size = ImGui::GetContentRegionAvail();

	_imgui_begin_child(IMGUI_ANIMATIONS_CHILD.copy({.size = {size.x, size.y - IMGUI_FOOTER_CHILD.size.y}}), self);

	for (auto & [id, animation] : self->anm2->animations)
	{
		ImGui::PushID(id);

		ImguiItem animationItem = IMGUI_ANIMATION.copy
		({
			.isSelected = self->reference->animationID == id, 
			.label = self->anm2->defaultAnimationID == id ? std::format(IMGUI_ANIMATION_DEFAULT_FORMAT, animation.name) : animation.name, 
			.id = id
		});
		
		if (_imgui_atlas_selectable_input_text(animationItem, self, animation.name))
		{
			self->reference->animationID = id;
			anm2_reference_item_clear(self->reference);
		}

		if (ImGui::IsItemHovered())
		{
			self->clipboard->type = CLIPBOARD_ANIMATION;
			self->clipboard->location = (s32)id;
		}
		
		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
		{
			if (ImGui::IsDragDropActive()) ImGui::SetNextItemWidth(_imgui_item_size_get(animationItem, IMGUI_SELECTABLE).x);
			ImGui::SetDragDropPayload(animationItem.drag_drop_get(), &id, sizeof(s32));
			_imgui_atlas_selectable(animationItem, self);
			ImGui::EndDragDropSource();	
		}

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(animationItem.drag_drop_get()))
			{
				s32 sourceID = *(s32*)payload->Data;
				if (sourceID != id)
				{
					imgui_snapshot(self, IMGUI_ACTION_ANIMATION_SWAP);
					map_swap(self->anm2->animations, sourceID, id);
				}
			}

			ImGui::EndDragDropTarget();
		}
		
		ImGui::PopID();
	};

	_imgui_context_menu(self);

	_imgui_end_child(); // animationsChild
	
	Anm2Animation* animation = anm2_animation_from_reference(self->anm2, self->reference);
	
	_imgui_begin_child(IMGUI_FOOTER_CHILD, self);
	
	if (_imgui_button(IMGUI_ANIMATION_ADD.copy({self->anm2->path.empty()}), self))
	{
		s32 id = anm2_animation_add(self->anm2);
		self->reference->animationID = id;

		if (self->anm2->animations.size() == 1)
			self->anm2->defaultAnimationID = id;
	}

	if (_imgui_button(IMGUI_ANIMATION_DUPLICATE.copy({!animation}), self))
		self->reference->animationID = anm2_animation_add(self->anm2, animation, self->reference->animationID);

	_imgui_button(IMGUI_ANIMATION_MERGE.copy({!animation}), self);
	
	if (imgui_begin_popup_modal(IMGUI_ANIMATION_MERGE.popup, self, IMGUI_ANIMATION_MERGE.popupSize))
	{
		const bool isModCtrl  = ImGui::IsKeyDown(IMGUI_INPUT_CTRL);
		const bool isModShift = ImGui::IsKeyDown(IMGUI_INPUT_SHIFT);
		static bool& isDeleteAnimationsAfter = self->settings->mergeIsDeleteAnimationsAfter;
		static s32 lastClickedID = ID_NONE;
		static s32& mergeType = self->settings->mergeType;
		static size_t lastAnimationCount = 0;
		static std::vector<s32> animationIDs;
		static std::vector<s32> sortedIDs;
		
		if (self->anm2->animations.size() != lastAnimationCount)
		{
			sortedIDs.clear();
			for (const auto& [id, _] : self->anm2->animations)
				sortedIDs.push_back(id);
			std::sort(sortedIDs.begin(), sortedIDs.end());
			lastAnimationCount = self->anm2->animations.size();
		}

		_imgui_begin_child(IMGUI_MERGE_ANIMATIONS_CHILD, self);

		for (const auto& [id, animation] : self->anm2->animations)
		{
			ImGui::PushID(id);

			if 
			(
				_imgui_atlas_selectable
				(
					IMGUI_ANIMATION.copy
					({
						.isSelected = std::find(animationIDs.begin(), animationIDs.end(), id) != animationIDs.end(), 
						.label = animation.name
					}),
					self
				)
			)
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

		_imgui_end_child(); //IMGUI_MERGE_ANIMATIONS_CHILD
			
		_imgui_begin_child(IMGUI_MERGE_ON_CONFLICT_CHILD, self);
		_imgui_text(IMGUI_MERGE_ON_CONFLICT, self);
		_imgui_radio_button(IMGUI_MERGE_APPEND_FRAMES, self, mergeType);
		_imgui_radio_button(IMGUI_MERGE_REPLACE_FRAMES, self, mergeType);
		_imgui_radio_button(IMGUI_MERGE_PREPEND_FRAMES, self, mergeType);
		_imgui_radio_button(IMGUI_MERGE_IGNORE, self, mergeType);
		_imgui_end_child(); //IMGUI_MERGE_ON_CONFLICT_CHILD
		
		_imgui_begin_child(IMGUI_MERGE_OPTIONS_CHILD, self);

		_imgui_checkbox(IMGUI_MERGE_DELETE_ANIMATIONS_AFTER, self, isDeleteAnimationsAfter);

		_imgui_end_child(); //IMGUI_MERGE_OPTIONS_CHILD
		
		if (_imgui_button(IMGUI_MERGE_CONFIRM.copy({animationIDs.empty()}), self))
		{
			anm2_animation_merge(self->anm2, self->reference->animationID, animationIDs, (Anm2MergeType)mergeType);

			if (isDeleteAnimationsAfter)
				for (s32 id : animationIDs)
					if (id != self->reference->animationID)
						self->anm2->animations.erase(id);

			animationIDs.clear();
			imgui_close_current_popup(self);
		}

		if (_imgui_button(IMGUI_POPUP_CANCEL, self))
		{
			animationIDs.clear();
			imgui_close_current_popup(self);
		}
		
		imgui_end_popup(self);
	}

	if (_imgui_button(IMGUI_ANIMATION_REMOVE.copy({!animation}), self))
	{
		anm2_animation_remove(self->anm2, self->reference->animationID);
		anm2_reference_clear(self->reference);
	}
	
	if (_imgui_button(IMGUI_ANIMATION_DEFAULT.copy({!animation}), self))
		self->anm2->defaultAnimationID = self->reference->animationID; 

	_imgui_end_child(); // IMGUI_FOOTER_CHILD
	_imgui_end(); // IMGUI_ANIMATIONS
}

static void _imgui_events(Imgui* self)
{
	static s32 selectedID = ID_NONE;
	
	IMGUI_BEGIN_OR_RETURN(IMGUI_EVENTS, self);
	_imgui_no_anm2_path_check(self);
	
	ImVec2 windowSize = ImGui::GetContentRegionAvail();

	_imgui_begin_child(IMGUI_EVENTS_CHILD.copy({.size = {windowSize.x, windowSize.y - IMGUI_FOOTER_CHILD.size.y}}), self);

	std::function<void(s32, Anm2Event&)> event_item = [&](s32 id, Anm2Event& event)
	{
		ImGui::PushID(id);

		if (_imgui_atlas_selectable_input_text(IMGUI_EVENT.copy({.isSelected = id == selectedID, .label = event.name, .id = id}), self, event.name))
			selectedID = id;

		ImGui::PopID();
	};

	for (auto& [id, event] : self->anm2->events)
		event_item(id, event);

	_imgui_end_child(); // eventsChild
	
	_imgui_begin_child(IMGUI_FOOTER_CHILD, self);
	
	if (_imgui_button(IMGUI_EVENTS_ADD.copy({self->anm2->path.empty()}), self))
	{
		s32 id = map_next_id_get(self->anm2->events);
		self->anm2->events[id] = Anm2Event{}; 
		selectedID = id;
	}

	if (_imgui_button(IMGUI_EVENTS_REMOVE_UNUSED.copy({self->anm2->events.empty()}), self))
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
	
	_imgui_end_child(); // IMGUI_ANIMATIONS_OPTIONS_CHILD)
	_imgui_end(); // IMGUI_EVENTS
}

static void _imgui_spritesheets(Imgui* self)
{
	static std::unordered_set<s32> selectedIDs;
	static s32 highlightedID = ID_NONE;

	IMGUI_BEGIN_OR_RETURN(IMGUI_SPRITESHEETS, self);
	_imgui_no_anm2_path_check(self);

	ImVec2 windowSize = ImGui::GetContentRegionAvail();

	_imgui_begin_child(IMGUI_SPRITESHEETS_CHILD.copy({.size = {windowSize.x, windowSize.y - IMGUI_SPRITESHEETS_FOOTER_CHILD.size.y}}), self);
	
	std::function<void(s32, Anm2Spritesheet&)> spritesheet_item = [&](s32 id, Anm2Spritesheet& spritesheet)
	{
		ImGui::PushID(id);
		
		Texture& texture = spritesheet.texture;
		bool isContains = selectedIDs.contains(id);
		
		_imgui_begin_child(IMGUI_SPRITESHEET_CHILD, self);

		if (_imgui_checkbox(IMGUI_SPRITESHEET_SELECTED, self, isContains))
		{
			if (isContains)
				selectedIDs.insert(id);
			else
				selectedIDs.erase(id);
		}

		if (_imgui_atlas_selectable(IMGUI_SPRITESHEET.copy({.isSelected = id == highlightedID, .label = std::format(IMGUI_SPRITESHEET_FORMAT, id, spritesheet.path)}), self))
			highlightedID = id;

		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
		{
			ImGui::SetDragDropPayload(IMGUI_SPRITESHEET.drag_drop_get(), &id, sizeof(s32));
			spritesheet_item(id, spritesheet);
			ImGui::EndDragDropSource();
		}

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(IMGUI_SPRITESHEET.drag_drop_get()))
			{
				s32 sourceID = *(s32*)payload->Data;
				if (sourceID != id)
					map_swap(self->anm2->spritesheets, sourceID, id);
			}
			ImGui::EndDragDropTarget();
		}

		ImVec2 spritesheetPreviewSize = IMGUI_SPRITESHEET_PREVIEW_SIZE;
		f32 spritesheetAspect = (f32)texture.size.x / texture.size.y;

		if ((IMGUI_SPRITESHEET_PREVIEW_SIZE.x / IMGUI_SPRITESHEET_PREVIEW_SIZE.y) > spritesheetAspect)
			spritesheetPreviewSize.x = IMGUI_SPRITESHEET_PREVIEW_SIZE.y * spritesheetAspect;
		else
			spritesheetPreviewSize.y = IMGUI_SPRITESHEET_PREVIEW_SIZE.x / spritesheetAspect;

		if (texture.isInvalid)
			_imgui_atlas(ATLAS_NONE, self);
		else
			ImGui::Image(texture.id, spritesheetPreviewSize);
			
		_imgui_end_child(); // IMGUI_SPRITESHEET_CHILD

		ImGui::PopID();
	};
	
	for (auto& [id, spritesheet] : self->anm2->spritesheets)
		spritesheet_item(id, spritesheet);

	_imgui_end_child(); // spritesheetsChild
		
	_imgui_begin_child(IMGUI_SPRITESHEETS_FOOTER_CHILD, self);
	
	if (_imgui_button(IMGUI_SPRITESHEET_ADD.copy({self->anm2->path.empty()}), self))
		dialog_spritesheet_add(self->dialog);

	if (self->dialog->isSelected && self->dialog->type == DIALOG_SPRITESHEET_ADD)
	{
		_imgui_spritesheet_add(self, self->dialog->path);
		dialog_reset(self->dialog);
	}
	
	if (_imgui_button(IMGUI_SPRITESHEETS_RELOAD.copy({selectedIDs.empty()}), self))
	{
		for (auto& id : selectedIDs)
		{
			std::filesystem::path workingPath = std::filesystem::current_path();
			working_directory_from_file_set(self->anm2->path);
			Texture texture;
			texture_from_path_init(&texture, self->anm2->spritesheets[id].path);
			self->anm2->spritesheets[id].texture = texture;
			std::filesystem::current_path(workingPath);
		}

		imgui_log_push(self, IMGUI_LOG_RELOAD_SPRITESHEET);
	}

	if (_imgui_button(IMGUI_SPRITESHEETS_REPLACE.copy({highlightedID == ID_NONE}), self))
		dialog_spritesheet_replace(self->dialog, highlightedID);

	if (self->dialog->isSelected && self->dialog->type == DIALOG_SPRITESHEET_REPLACE)
	{
		imgui_snapshot(self, IMGUI_ACTION_REPLACE_SPRITESHEET);
		
		std::filesystem::path workingPath = std::filesystem::current_path();
		std::string anm2WorkingPath = working_directory_from_file_set(self->anm2->path);
		std::string spritesheetPath = std::filesystem::relative(self->dialog->path, anm2WorkingPath).string();
	
		self->anm2->spritesheets[self->dialog->replaceID].path = spritesheetPath;
		Texture texture;
		texture_from_path_init(&texture, spritesheetPath);
		self->anm2->spritesheets[self->dialog->replaceID].texture = texture; 
		dialog_reset(self->dialog);
		std::filesystem::current_path(workingPath);
	}

	if (_imgui_button(IMGUI_SPRITESHEETS_REMOVE_UNUSED.copy({self->anm2->spritesheets.empty()}), self))
	{
		std::unordered_set<s32> usedSpritesheetIDs;

		for (auto& [layerID, layer] : self->anm2->layers)
			if (layer.spritesheetID != ID_NONE)
				usedSpritesheetIDs.insert(layer.spritesheetID);

		for (auto it = self->anm2->spritesheets.begin(); it != self->anm2->spritesheets.end(); )
		{
			if (!usedSpritesheetIDs.count(it->first))
			{
				texture_free(&self->anm2->spritesheets[it->first].texture);
				it = self->anm2->spritesheets.erase(it);
			}
			else
				it++;
		}
	}

	if (_imgui_button(IMGUI_SPRITESHEETS_SELECT_ALL.copy({selectedIDs.size() == self->anm2->spritesheets.size()}), self))
		for (auto [id, _] : self->anm2->spritesheets)
			selectedIDs.insert(id);

	if (_imgui_button(IMGUI_SPRITESHEETS_SELECT_NONE.copy({selectedIDs.empty()}), self))
			selectedIDs.clear();

	if (_imgui_button(IMGUI_SPRITESHEET_SAVE.copy({selectedIDs.empty()}), self))
	{
		for (auto& id : selectedIDs)
		{
			Anm2Spritesheet& spritesheet = self->anm2->spritesheets[id];
			std::filesystem::path workingPath = std::filesystem::current_path();
			working_directory_from_file_set(self->anm2->path);
			texture_from_gl_write(&spritesheet.texture, spritesheet.path);
			imgui_log_push(self, std::format(IMGUI_LOG_SPRITESHEET_SAVE_FORMAT, id, spritesheet.path));
			std::filesystem::current_path(workingPath);
		}
	}

	if (_imgui_is_no_click_on_item()) highlightedID = ID_NONE;

	_imgui_end_child(); //IMGUI_SPRITESHEETS_FOOTER_CHILD
	_imgui_end(); // IMGUI_SPRITESHEETS
}

static void _imgui_animation_preview(Imgui* self)
{
	static s32& tool = self->settings->tool;
	static f32& zoom = self->settings->previewZoom;
	static vec2& pan = self->settings->previewPan;
	static ivec2& size = self->preview->canvas.size;
	static vec2 mousePos{};
	static vec2 previewPos{};

	std::string mousePositionString = std::format(IMGUI_POSITION_FORMAT, (s32)mousePos.x, (s32)mousePos.y);
	
	IMGUI_BEGIN_OR_RETURN(IMGUI_ANIMATION_PREVIEW, self);

	_imgui_begin_child(IMGUI_CANVAS_GRID_CHILD, self);
	_imgui_checkbox(IMGUI_CANVAS_GRID, self, self->settings->previewIsGrid);
	ImGui::SameLine();
	_imgui_color_edit4(IMGUI_CANVAS_GRID_COLOR, self, self->settings->previewGridColor);
	_imgui_input_int2(IMGUI_CANVAS_GRID_SIZE, self, self->settings->previewGridSize);
	_imgui_input_int2(IMGUI_CANVAS_GRID_OFFSET, self, self->settings->previewGridOffset);
	_imgui_end_child(); // IMGUI_CANVAS_GRID_CHILD
	
	ImGui::SameLine();
	
	_imgui_begin_child(IMGUI_CANVAS_VIEW_CHILD, self);
	_imgui_drag_float(IMGUI_CANVAS_ZOOM, self, zoom);
	if (_imgui_button(IMGUI_ANIMATION_PREVIEW_CENTER_VIEW.copy({pan == vec2()}), self)) pan = vec2();
	if (_imgui_button(IMGUI_ANIMATION_PREVIEW_FIT.copy({self->reference->animationID == ID_NONE}), self))
	{
    	vec4 rect = anm2_animation_rect_get(self->anm2, self->reference, self->settings->previewIsRootTransform);
		
		if (rect != vec4(-1.0f) && (rect.z > 0 && rect.w > 0))
		{
			f32 scaleX = self->preview->canvas.size.x / rect.z;
			f32 scaleY = self->preview->canvas.size.y / rect.w;
			f32 fitScale = std::min(scaleX, scaleY);

			zoom = UNIT_TO_PERCENT(fitScale);

        	vec2 rectCenter = { rect.x + rect.z * 0.5f, rect.y + rect.w * 0.5f };
        	pan = -rectCenter * fitScale;
		}
	}
	ImGui::Text(mousePositionString.c_str());
	_imgui_end_child(); //IMGUI_CANVAS_VIEW_CHILD

	ImGui::SameLine();
	
	_imgui_begin_child(IMGUI_CANVAS_VISUAL_CHILD, self);
	_imgui_color_edit4(IMGUI_CANVAS_BACKGROUND_COLOR, self, self->settings->previewBackgroundColor);
	
	std::vector<s32> animationIDs;
	ImguiItem animationOverlayItem = IMGUI_CANVAS_ANIMATION_OVERLAY;

	animationIDs.emplace_back(ID_NONE);
	animationOverlayItem.items.emplace_back(IMGUI_NONE);

	for (auto& [id, animation] : self->anm2->animations) 
	{
		animationIDs.emplace_back(id);
		animationOverlayItem.items.emplace_back(animation.name);
	}

	s32 animationIndex = 0;

	if (self->preview->animationOverlayID != ID_NONE)
		animationIndex = std::find(animationIDs.begin(), animationIDs.end(), self->preview->animationOverlayID) - animationIDs.begin();
		
	if (_imgui_combo(animationOverlayItem, self, &animationIndex))
		self->preview->animationOverlayID = animationIDs[animationIndex];

	_imgui_drag_float(IMGUI_CANVAS_ANIMATION_OVERLAY_TRANSPARENCY, self, self->settings->previewOverlayTransparency);
	_imgui_end_child(); //IMGUI_CANVAS_VISUAL_CHILD

	ImGui::SameLine();

	_imgui_begin_child(IMGUI_CANVAS_HELPER_CHILD, self);
	_imgui_checkbox(IMGUI_CANVAS_AXES, self, self->settings->previewIsAxes);
	ImGui::SameLine();
	_imgui_color_edit4(IMGUI_CANVAS_AXES_COLOR, self, self->settings->previewAxesColor);
	ImGui::SameLine();
	_imgui_checkbox(IMGUI_CANVAS_ALT_ICONS, self, self->settings->previewIsAltIcons);
	_imgui_checkbox(IMGUI_CANVAS_ROOT_TRANSFORM, self, self->settings->previewIsRootTransform);
	ImGui::SameLine();
	_imgui_checkbox(IMGUI_CANVAS_TRIGGERS, self, self->settings->previewIsTriggers);
	_imgui_checkbox(IMGUI_CANVAS_PIVOTS, self, self->settings->previewIsPivots);
	ImGui::SameLine();
	_imgui_checkbox(IMGUI_CANVAS_ICONS, self, self->settings->previewIsIcons);
	ImGui::SameLine();
	_imgui_checkbox(IMGUI_CANVAS_BORDER, self, self->settings->previewIsBorder);
	_imgui_end_child(); // IMGUI_CANVAS_HELPER_CHILD

	ImVec2 previewCursorScreenPos = ImGui::GetCursorScreenPos();
	
	if (!self->preview->isRender) size = ivec2(vec2(ImGui::GetContentRegionAvail()));
		   
	preview_draw(self->preview);
	ImGui::Image(self->preview->canvas.framebuffer, vec2(size));
	
	if (self->settings->previewIsTriggers)
	{
		Anm2Frame trigger; 
		anm2_frame_from_time(self->anm2, &trigger, {self->reference->animationID, ANM2_TRIGGERS}, self->preview->time);

		if (trigger.eventID != ID_NONE)
		{
			f32 textScale = ImGui::GetCurrentWindow()->FontWindowScale;
			ImVec2 textPos = previewCursorScreenPos + ImGui::GetStyle().ItemSpacing;
			ImGui::SetWindowFontScale(IMGUI_TRIGGERS_FONT_SCALE);
			ImGui::GetWindowDrawList()->AddText(textPos, IMGUI_TRIGGERS_EVENT_COLOR, self->anm2->events[trigger.eventID].name.c_str());
			ImGui::SetWindowFontScale(textScale);
		}
	}

	if (ImGui::IsItemHovered()) 
	{
		self->pendingCursor = TOOL_CURSORS[tool];
		imgui_keyboard_nav_disable();
	}
	else
	{
		_imgui_end(); // IMGUI_ANIMATION_EDITOR
		imgui_keyboard_nav_enable();
		return;
	}

	_imgui_end(); // IMGUI_ANIMATION_PREVIEW

	mousePos = vec2(ImGui::GetMousePos() - previewCursorScreenPos - pan - (vec2(size) * 0.5f)) / PERCENT_TO_UNIT(zoom);
	
	const bool isLeft = ImGui::IsKeyPressed(IMGUI_INPUT_LEFT);
	const bool isRight = ImGui::IsKeyPressed(IMGUI_INPUT_RIGHT);
	const bool isUp = ImGui::IsKeyPressed(IMGUI_INPUT_UP);
	const bool isDown = ImGui::IsKeyPressed(IMGUI_INPUT_DOWN);
	const bool isMod = ImGui::IsKeyDown(IMGUI_INPUT_SHIFT);
	const bool isZoomIn = _imgui_chord_pressed(imgui_hotkey_chord_registry()[HOTKEY_ZOOM_IN]);
	const bool isZoomOut = _imgui_chord_pressed(imgui_hotkey_chord_registry()[HOTKEY_ZOOM_OUT]);
	const bool isMouseClick = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
	const bool isMouseDown = ImGui::IsMouseDown(ImGuiMouseButton_Left);
	const bool isMouseMiddleDown = ImGui::IsMouseDown(ImGuiMouseButton_Middle);
	const ImVec2 mouseDelta = ImGui::GetIO().MouseDelta;
	const f32 mouseWheel = ImGui::GetIO().MouseWheel;
	
	Anm2Frame* frame = nullptr;
	
	if (self->reference->itemType != ANM2_TRIGGERS) 
		frame = anm2_frame_from_reference(self->anm2, self->reference);

	f32 step = isMod ? TOOL_STEP_MOD : TOOL_STEP;
	
	if ((tool == TOOL_PAN && isMouseDown) || isMouseMiddleDown)
		pan += vec2(mouseDelta.x, mouseDelta.y);

	switch (tool)
	{
		case TOOL_MOVE:
			if (!frame) break;
		
			if (isMouseClick || isLeft || isRight || isUp || isDown)
				imgui_snapshot(self, IMGUI_ACTION_MOVE);
		
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
			if (!frame) break;

			if (isMouseClick || isLeft || isRight || isUp || isDown)
				imgui_snapshot(self, IMGUI_ACTION_ROTATE);
		
			if (isMouseDown)
				frame->rotation += mouseDelta.x;
			else
			{
				if (isLeft || isUp) frame->rotation -= step;
				if (isRight || isDown) frame->rotation += step;
			}
			break;
		case TOOL_SCALE:
			if (!frame) break;

			if (isMouseClick || isLeft || isRight || isUp || isDown)
				imgui_snapshot(self, IMGUI_ACTION_SCALE);
		
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

	if (mouseWheel != 0 || isZoomIn || isZoomOut)
	{
		f32 delta = (mouseWheel > 0 || isZoomIn) ? CANVAS_ZOOM_STEP : -CANVAS_ZOOM_STEP;
		zoom = std::clamp(ROUND_NEAREST_MULTIPLE(zoom + delta, CANVAS_ZOOM_STEP), CANVAS_ZOOM_MIN, CANVAS_ZOOM_MAX);
	}
}

static void _imgui_spritesheet_editor(Imgui* self)
{
	static vec2 mousePos = {0, 0};
	static s32& tool = self->settings->tool;
	static vec4& toolColor = self->settings->toolColor;
	static ivec2& gridSize = self->settings->editorGridSize;
	static ivec2& gridOffset = self->settings->editorGridOffset;
	static vec2& pan = self->settings->editorPan;
	static f32& zoom = self->settings->editorZoom;
	static ivec2& size = self->editor->canvas.size;
	
	std::string mousePositionString = std::format(IMGUI_POSITION_FORMAT, (s32)mousePos.x, (s32)mousePos.y);

	IMGUI_BEGIN_OR_RETURN(IMGUI_SPRITESHEET_EDITOR, self);
	
	_imgui_begin_child(IMGUI_CANVAS_GRID_CHILD, self);
	_imgui_checkbox(IMGUI_CANVAS_GRID, self, self->settings->editorIsGrid);
	ImGui::SameLine();
	_imgui_checkbox(IMGUI_CANVAS_GRID_SNAP, self, self->settings->editorIsGridSnap);
	ImGui::SameLine();
	_imgui_color_edit4(IMGUI_CANVAS_GRID_COLOR, self, self->settings->editorGridColor);
	_imgui_input_int2(IMGUI_CANVAS_GRID_SIZE, self, gridSize);
	_imgui_input_int2(IMGUI_CANVAS_GRID_OFFSET, self, gridOffset);
	_imgui_end_child();
	
	ImGui::SameLine();
	
	_imgui_begin_child(IMGUI_CANVAS_VIEW_CHILD, self);
	_imgui_drag_float(IMGUI_CANVAS_ZOOM, self, zoom);
	if (_imgui_button(IMGUI_SPRITESHEET_EDITOR_CENTER_VIEW.copy({pan == vec2()}), self)) pan = vec2();
	if (_imgui_button(IMGUI_SPRITESHEET_EDITOR_FIT.copy({self->editor->spritesheetID == ID_NONE}), self))
	{
        vec4 rect = {0, 0, self->anm2->spritesheets[self->editor->spritesheetID].texture.size.x,
        				   self->anm2->spritesheets[self->editor->spritesheetID].texture.size.y};

		if ((rect.z > 0 && rect.w > 0))
		{
			f32 scaleX = self->editor->canvas.size.x / rect.z;
			f32 scaleY = self->editor->canvas.size.y / rect.w;
			f32 fitScale = std::min(scaleX, scaleY);

			zoom = UNIT_TO_PERCENT(fitScale);
			pan = {};
		}
	}
	ImGui::Text(mousePositionString.c_str());
	_imgui_end_child(); // IMGUI_CANVAS_VIEW_CHILD
	
	ImGui::SameLine();
	
	_imgui_begin_child(IMGUI_CANVAS_VISUAL_CHILD, self);
	_imgui_color_edit4(IMGUI_CANVAS_BACKGROUND_COLOR, self, self->settings->editorBackgroundColor);
	_imgui_checkbox(IMGUI_CANVAS_BORDER, self, self->settings->editorIsBorder);
	_imgui_end_child(); // IMGUI_CANVAS_VISUAL_CHILD
	
	ImVec2 editorCursorScreenPos = ImGui::GetCursorScreenPos();
	size = ivec2(vec2(ImGui::GetContentRegionAvail()));
	editor_draw(self->editor);
	ImGui::Image(self->editor->canvas.framebuffer, vec2(size));
	
	if (ImGui::IsItemHovered()) 
	{
		self->pendingCursor = TOOL_CURSORS[tool];
		imgui_keyboard_nav_disable();
	}
	else
	{
		_imgui_end(); // IMGUI_SPRITESHEET_EDITOR
		imgui_keyboard_nav_enable();
		return;
	}

	_imgui_end(); // IMGUI_SPRITESHEET_EDITOR

	mousePos = vec2(ImGui::GetMousePos() - editorCursorScreenPos - pan) / PERCENT_TO_UNIT(zoom);

	const bool isLeft = ImGui::IsKeyPressed(IMGUI_INPUT_LEFT);
	const bool isRight = ImGui::IsKeyPressed(IMGUI_INPUT_RIGHT);
	const bool isUp = ImGui::IsKeyPressed(IMGUI_INPUT_UP);
	const bool isDown = ImGui::IsKeyPressed(IMGUI_INPUT_DOWN);
	const bool isShift = ImGui::IsKeyDown(IMGUI_INPUT_SHIFT);
	const bool isCtrl = ImGui::IsKeyDown(IMGUI_INPUT_CTRL);
	const bool isMouseClick = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
	const bool isMouseDown = ImGui::IsMouseDown(ImGuiMouseButton_Left);
	const bool isMouseMiddleDown = ImGui::IsMouseDown(ImGuiMouseButton_Middle);
	const bool isZoomIn = _imgui_chord_pressed(imgui_hotkey_chord_registry()[HOTKEY_ZOOM_IN]);
	const bool isZoomOut = _imgui_chord_pressed(imgui_hotkey_chord_registry()[HOTKEY_ZOOM_OUT]);
	const f32 mouseWheel = ImGui::GetIO().MouseWheel;
	const ImVec2 mouseDelta = ImGui::GetIO().MouseDelta;
	
	Anm2Frame* frame = nullptr;
	if (self->reference->itemType == ANM2_LAYER) 
		frame = anm2_frame_from_reference(self->anm2, self->reference);

	Anm2Spritesheet* spritesheet = map_find(self->anm2->spritesheets, self->editor->spritesheetID);
	Texture* texture = spritesheet ? &spritesheet->texture : nullptr;

	vec2 position = mousePos;
	f32 step = isShift ? TOOL_STEP_MOD : TOOL_STEP;
	
	if ((tool == TOOL_PAN && isMouseDown) || isMouseMiddleDown)
		pan += vec2(mouseDelta.x, mouseDelta.y);
		
	switch (tool)
	{
		case TOOL_MOVE:
			if (!texture || !frame) break;

			if (isMouseClick || isLeft || isRight || isUp || isDown)
				imgui_snapshot(self, IMGUI_ACTION_MOVE);

			if (isMouseDown) 
			{
				if (self->settings->editorIsGridSnap)
				{
					position.x = roundf((position.x - gridSize.x) / gridSize.x) * gridSize.x + gridOffset.x - (gridSize.x * 0.5f);
					position.y = roundf((position.y - gridSize.y) / gridSize.y) * gridSize.y + gridOffset.y - (gridSize.y * 0.5f);
				}	

				frame->pivot = position - frame->crop;
			}
			else 
			{
				if (isLeft)  frame->pivot.x -= step;
				if (isRight) frame->pivot.x += step;
				if (isUp)    frame->pivot.y -= step;
				if (isDown)  frame->pivot.y += step;
			}
			break;
		case TOOL_CROP:
			if (!texture || !frame) break;

			if (isMouseClick || isLeft || isRight || isUp || isDown)
				imgui_snapshot(self, IMGUI_ACTION_MOVE);

			if (self->settings->editorIsGridSnap)
			{
				position.x = roundf(position.x / gridSize.x) * gridSize.x + gridOffset.x - (gridSize.x * 0.5f);
				position.y = roundf(position.y / gridSize.y) * gridSize.y + gridOffset.y - (gridSize.y * 0.5f);
			}
			
			if (isMouseClick)
			{
				frame->crop = position;
				frame->size = ivec2(0,0);
			}
			else if (isMouseDown)
				frame->size = position - frame->crop;
			else
			{
				if (isCtrl)
				{
					if (isLeft)  frame->crop.x -= step;
					if (isRight) frame->crop.x += step;
					if (isUp)    frame->crop.y -= step;
					if (isDown)  frame->crop.y += step;
				}
				else
				{
					if (isLeft)  frame->size.x -= step;
					if (isRight) frame->size.x += step;
					if (isUp)    frame->size.y -= step;
					if (isDown)  frame->size.y += step;
				}

				frame->size.x = std::max({}, frame->size.x);
				frame->size.y = std::max({}, frame->size.y);
			}
			break;
		case TOOL_DRAW:
		case TOOL_ERASE:
		{
			if (!texture) break;
			
			vec4 color = tool == TOOL_ERASE ? COLOR_TRANSPARENT : toolColor;

			if (isMouseClick)
				imgui_snapshot(self, tool == TOOL_DRAW ? IMGUI_ACTION_DRAW : IMGUI_ACTION_ERASE);

			if (isMouseDown)
				texture_pixel_set(texture, position, color);
			break;
		}
		case TOOL_COLOR_PICKER:
			if (isMouseDown)
			{
				SDL_GetMouseState(&mousePos.x, &mousePos.y);
				_imgui_window_color_from_position_get(self->window, mousePos, toolColor);
				ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2());
				ImGui::BeginTooltip();
				_imgui_color_button(IMGUI_COLOR_PICKER_BUTTON, self, toolColor);
				ImGui::EndTooltip();
				ImGui::PopStyleVar();
			}
			break;
		default:
			break;
	}
			
	if (mouseWheel != 0 || isZoomIn || isZoomOut)
	{
		f32 delta = (mouseWheel > 0 || isZoomIn) ? CANVAS_ZOOM_STEP : -CANVAS_ZOOM_STEP;
		zoom = std::clamp(ROUND_NEAREST_MULTIPLE(zoom + delta, CANVAS_ZOOM_STEP), CANVAS_ZOOM_MIN, CANVAS_ZOOM_MAX);
	}
}

static void _imgui_frame_properties(Imgui* self)
{
	static Anm2Type& type = self->reference->itemType;
	
	IMGUI_BEGIN_OR_RETURN(IMGUI_FRAME_PROPERTIES, self);

	Anm2Frame* frame = anm2_frame_from_reference(self->anm2, self->reference);
	
	bool isLayerFrame = frame && type == ANM2_LAYER;
	
	if (!frame || type != ANM2_TRIGGERS)
	{
		_imgui_drag_float2(IMGUI_FRAME_PROPERTIES_CROP.copy({!isLayerFrame}), self, !isLayerFrame ? dummy_value<vec2>() : frame->crop);
		_imgui_drag_float2(IMGUI_FRAME_PROPERTIES_SIZE.copy({!isLayerFrame}), self, !isLayerFrame ? dummy_value<vec2>() : frame->size);
		_imgui_drag_float2(IMGUI_FRAME_PROPERTIES_POSITION.copy({!frame}), self, !frame ? dummy_value<vec2>() : frame->position);
		_imgui_drag_float2(IMGUI_FRAME_PROPERTIES_PIVOT.copy({!isLayerFrame}), self, !isLayerFrame ? dummy_value<vec2>() : frame->pivot);
		_imgui_drag_float2(IMGUI_FRAME_PROPERTIES_SCALE.copy({!frame}), self, !frame ? dummy_value<vec2>() : frame->scale);
		_imgui_drag_float(IMGUI_FRAME_PROPERTIES_ROTATION.copy({!frame}), self, !frame ? dummy_value<f32>() : frame->rotation);
		_imgui_input_int(IMGUI_FRAME_PROPERTIES_DELAY.copy({!frame}), self, !frame ? dummy_value<s32>() : frame->delay);
		_imgui_color_edit4(IMGUI_FRAME_PROPERTIES_TINT.copy({!frame}), self, !frame ? dummy_value<vec4>() : frame->tintRGBA);
		_imgui_color_edit3(IMGUI_FRAME_PROPERTIES_COLOR_OFFSET.copy({!frame}), self, !frame ? dummy_value<vec3>() : frame->offsetRGB);
		_imgui_checkbox(IMGUI_FRAME_PROPERTIES_VISIBLE.copy({!frame}), self, !frame ? dummy_value<bool>() : frame->isVisible);
		_imgui_checkbox(IMGUI_FRAME_PROPERTIES_INTERPOLATED.copy({!frame}), self, !frame ? dummy_value<bool>() : frame->isInterpolated);
		_imgui_checkbox(IMGUI_FRAME_PROPERTIES_ROUND.copy({!frame}), self, self->settings->propertiesIsRound);
		if (_imgui_button(IMGUI_FRAME_PROPERTIES_FLIP_X.copy({!frame}), self)) frame->scale.x = -frame->scale.x;
		if (_imgui_button(IMGUI_FRAME_PROPERTIES_FLIP_Y.copy({!frame}), self)) frame->scale.y = -frame->scale.y;
	
		if (self->settings->propertiesIsRound && frame)
		{
			frame->position = glm::trunc(frame->position);
			frame->pivot = glm::trunc(frame->pivot);
			frame->crop = glm::trunc(frame->crop);
			frame->scale = glm::trunc(frame->scale);
			frame->rotation = glm::trunc(frame->rotation);
			frame->tintRGBA = glm::trunc(frame->tintRGBA);
			frame->offsetRGB = glm::trunc(frame->offsetRGB);
		}
	}
	else
	{
		std::vector<s32> eventIDs;
		ImguiItem eventItem = IMGUI_FRAME_PROPERTIES_EVENT.copy({!frame});
	
		eventIDs.emplace_back(ID_NONE);
		eventItem.items.emplace_back(IMGUI_NONE);
		
		for (auto & [id, event] : self->anm2->events) 
		{
			eventIDs.emplace_back(id);
			eventItem.items.emplace_back(event.name);
		}
		
		s32 eventIndex = std::find(eventIDs.begin(), eventIDs.end(), frame->eventID) - eventIDs.begin();
		
		if (_imgui_combo(eventItem, self, &eventIndex))
			frame->eventID = eventIDs[eventIndex];

		_imgui_input_int(IMGUI_FRAME_PROPERTIES_AT_FRAME.copy({!frame}), self, frame->atFrame);
	}

	_imgui_end(); // IMGUI_FRAME_PROPERTIES
}

static void _imgui_log(Imgui* self)
{
    ImGuiIO& io = ImGui::GetIO();
    ImGuiStyle& style = ImGui::GetStyle();
	ImVec4 borderColor = style.Colors[ImGuiCol_Border];
	ImVec4 textColor   = style.Colors[ImGuiCol_Text];

    ImVec2 position = {io.DisplaySize.x - IMGUI_LOG_PADDING, io.DisplaySize.y - IMGUI_LOG_PADDING};
 
	for (s32 i = (s32)self->log.size() - 1; i >= 0; --i)
    {
        ImguiLogItem& item = self->log[i];
		f32 lifetime = item.timeRemaining / IMGUI_LOG_DURATION;
     	borderColor.w = lifetime;
		textColor.w = lifetime;

		item.timeRemaining -= io.DeltaTime;

        if (item.timeRemaining <= 0.0f) 
		{ 
			self->log.erase(self->log.begin() + i); 
			continue; 
		}
	
        ImGui::SetNextWindowPos(position, ImGuiCond_Always, {1.0f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_Border, borderColor);
        ImGui::PushStyleColor(ImGuiCol_Text, textColor);
        ImGui::SetNextWindowBgAlpha(lifetime);

		_imgui_begin(IMGUI_LOG_WINDOW.copy({.label = std::format(IMGUI_LOG_FORMAT, i)}), self);
        ImGui::TextUnformatted(item.text.c_str());
		ImVec2 windowSize = ImGui::GetWindowSize();
        _imgui_end(); // IMGUI_LOG_WINDOW

        ImGui::PopStyleColor(2);

        position.y -= windowSize.y + IMGUI_LOG_PADDING;
  	}
}

static void _imgui_dock(Imgui* self)
{
	ImguiItem window = IMGUI_WINDOW_MAIN;
	ImGuiViewport* viewport = ImGui::GetMainViewport();

    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + IMGUI_TASKBAR.size.y));
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, viewport->Size.y - IMGUI_TASKBAR.size.y));
    ImGui::SetNextWindowViewport(viewport->ID);
	
	_imgui_begin(window, self);
	_imgui_dockspace(IMGUI_DOCKSPACE_MAIN, self);

	_imgui_tools(self);
	_imgui_animations(self);
	_imgui_events(self);
	_imgui_spritesheets(self);
	_imgui_animation_preview(self);
	_imgui_spritesheet_editor(self);
	_imgui_layers(self);
	_imgui_nulls(self);
	_imgui_timeline(self);
	_imgui_onionskin(self);
	_imgui_frame_properties(self);

	_imgui_end(); // IMGUI_WINDOW_MAIN 
}

void imgui_init
(
    Imgui* self,
    Dialog* dialog,
    Resources* resources,
    Anm2* anm2,
    Anm2Reference* reference,
    Editor* editor,
    Preview* preview,
    GeneratePreview* generatePreview,
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
	self->generatePreview = generatePreview;
	self->settings = settings;
	self->snapshots = snapshots;
	self->clipboard = clipboard;
	self->window = window;
	self->glContext = glContext;

	self->saveAnm2 = *anm2;
	
	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	ImGui_ImplSDL3_InitForOpenGL(self->window, *self->glContext);
	ImGui_ImplOpenGL3_Init(IMGUI_OPENGL_VERSION);

	ImGuiIO& io = ImGui::GetIO();
	io.IniFilename = nullptr;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigWindowsMoveFromTitleBarOnly = true;

	imgui_keyboard_nav_enable();
	
	ImGui::LoadIniSettingsFromDisk(settings_path_get().c_str());

	for (s32 i = 0; i < HOTKEY_COUNT; i++)
	{
		if (!SETTINGS_HOTKEY_MEMBERS[i]) continue;
		imgui_hotkey_chord_registry()[i] = imgui_chord_from_string_get(*&(self->settings->*SETTINGS_HOTKEY_MEMBERS[i]));
	}

}

void imgui_update(Imgui* self)
{
	ImGui_ImplSDL3_NewFrame();
	ImGui_ImplOpenGL3_NewFrame();
	ImGui::NewFrame();

	_imgui_taskbar(self);
	_imgui_dock(self);
	_imgui_log(self);

	if (self->isContextualActionsEnabled)
	{
		for (const auto& item : imgui_item_registry())
		{
			if (item->is_chord() && _imgui_chord_pressed(item->chord_get()) && !item->is_focus_window())
			{
				if (item->is_undoable()) imgui_snapshot(self, item->snapshotAction);
				if (item->is_function()) item->function(self);
			}
		}
	}

	imgui_contextual_actions_enable(self);

	if (self->pendingCursor != self->cursor)
	{ 
		SDL_SetCursor(SDL_CreateSystemCursor(self->pendingCursor));
		self->cursor = self->pendingCursor;
	}

	self->pendingCursor = CURSOR_DEFAULT;

	SDL_Event event;

	while(SDL_PollEvent(&event))
	{
    	ImGui_ImplSDL3_ProcessEvent(&event);
		
		switch (event.type)
		{
			case SDL_EVENT_DROP_FILE:
        	{
            	const char* droppedFile = event.drop.data;
            	
				if (path_is_extension(droppedFile, ANM2_EXTENSION)) _imgui_anm2_open(self, droppedFile);
				else if (path_is_extension(droppedFile, ANM2_SPRITESHEET_EXTENSION)) _imgui_spritesheet_add(self, droppedFile);
				else imgui_log_push(self, IMGUI_LOG_DRAG_DROP_ERROR);
            	
				break;
			}
			case SDL_EVENT_QUIT:
				if (self->isTryQuit) self->isQuit = true;
				else imgui_quit(self);
				break;
			default:
				break;
		}
	}
}

void imgui_draw(void)
{
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void imgui_free(void)
{
    if (!ImGui::GetCurrentContext()) return;
	
	ImGui_ImplSDL3_Shutdown();
	ImGui_ImplOpenGL3_Shutdown();
	ImGui::SaveIniSettingsToDisk(settings_path_get().c_str());
	ImGui::DestroyContext();
}