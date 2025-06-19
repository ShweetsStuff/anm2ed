#include "imgui.h"

static void _imgui_tooltip(const char* tooltip);
static void _imgui_timeline_element(Imgui* self, void* element, s32* id, s32* index, Anm2AnimationType type, Anm2AnimationType* selectType, s32* selectID);

/* Makes a tooltip! */
static void _imgui_tooltip(const char* tooltip)
{
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
        ImGui::SetTooltip("%s", tooltip);
}

/* Displays each element of the timeline of a selected animation */
static void
_imgui_timeline_element(Imgui* self, void* element, s32* id, s32* index, Anm2AnimationType type, Anm2AnimationType* selectType, s32* selectID)
{
		static s32 selectedElementIndex = -1;
		static s32 selectedSpritesheetIndex = -1;
		Anm2Layer* layer = NULL;
		Anm2LayerAnimation* layerAnimation = NULL;
		Anm2Null* null = NULL;
		Anm2NullAnimation* nullAnimation = NULL;
		Anm2RootAnimation* rootAnimation = NULL;
		Anm2Triggers* triggers = NULL;
		ImTextureID iconTexture = -1;

		bool isArrows = false;
		bool* isShowRect = NULL;
		bool* isVisible = NULL;
		char nameBuffer[ANM2_STRING_MAX] = STRING_EMPTY;
		char nameVisible[ANM2_STRING_FORMATTED_MAX] = STRING_EMPTY;
		char* namePointer = NULL;
		s32* spritesheetID = NULL;
		bool isSelected = selectedElementIndex == *index;
		bool isChangeable = type != ANM2_ROOT_ANIMATION && type != ANM2_TRIGGERS;
		
		switch (type)
		{
			case ANM2_ROOT_ANIMATION:
				rootAnimation = (Anm2RootAnimation*)element;
				iconTexture = self->packed->textures[PACKED_TEXTURE_ROOT].handle;
				strncpy(nameVisible, STRING_IMGUI_TIMELINE_ROOT, ANM2_STRING_FORMATTED_MAX);
				isVisible = &rootAnimation->isVisible;
				break;
			case ANM2_LAYER_ANIMATION:
				layerAnimation = (Anm2LayerAnimation*)element;
				layer = &self->anm2->layers[*id];
				iconTexture = self->packed->textures[PACKED_TEXTURE_LAYER].handle;
				isVisible = &layerAnimation->isVisible;
				spritesheetID = &layer->spritesheetID;
				namePointer = layer->name;
				snprintf(nameBuffer, ANM2_STRING_MAX, "%s", namePointer);
				snprintf(nameVisible, ANM2_STRING_FORMATTED_MAX, STRING_IMGUI_TIMELINE_ELEMENT_FORMAT, *id, namePointer); 
				break;
			case ANM2_NULL_ANIMATION:
				nullAnimation = (Anm2NullAnimation*)element;
				null = &self->anm2->nulls[*id];
				iconTexture = self->packed->textures[PACKED_TEXTURE_NULL].handle;
				isVisible = &nullAnimation->isVisible;
				isShowRect = &null->isShowRect;
				namePointer = null->name;
				snprintf(nameBuffer, ANM2_STRING_MAX, "%s", namePointer);
				snprintf(nameVisible, ANM2_STRING_FORMATTED_MAX, STRING_IMGUI_TIMELINE_ELEMENT_FORMAT, *id, namePointer); 
				break;
			case ANM2_TRIGGERS:
				triggers = (Anm2Triggers*)element;
				iconTexture = self->packed->textures[PACKED_TEXTURE_TRIGGER].handle;
				strncpy(nameVisible, STRING_IMGUI_TIMELINE_TRIGGERS, ANM2_STRING_FORMATTED_MAX);
				isVisible = &triggers->isVisible;
				break;
			default:
				break;
		}

		ImGui::BeginChild(nameVisible, IMGUI_TIMELINE_ELEMENT_SIZE, true, ImGuiWindowFlags_NoScrollbar);

		ImGui::PushID(*index);
		
		/* Shift arrows */
		if (isChangeable)
		{
			bool isSwap = false;
			bool isReversed = (type == ANM2_LAYER_ANIMATION);

			auto arrows_draw = [&](auto it, auto begin, auto end, auto& map, bool* didSwap, bool isReversed)
			{
				bool canMoveUp   = isReversed ? (std::next(it) != end) : (it != begin);
				bool canMoveDown = isReversed ? (it != begin)          : (std::next(it) != end);

				isArrows = canMoveUp || canMoveDown;

				if (isArrows)
					ImGui::BeginChild(STRING_IMGUI_TIMELINE_ELEMENT_SHIFT_ARROWS_LABEL, IMGUI_TIMELINE_SHIFT_ARROWS_SIZE);

				if (canMoveUp)
				{
					auto target = isReversed ? std::next(it) : std::prev(it);
					if (target != map.end() && ImGui::ImageButton(STRING_IMGUI_TIMELINE_ELEMENT_SHIFT_ABOVE, self->packed->textures[PACKED_TEXTURE_ARROW_UP].handle, IMGUI_ICON_SIZE))
					{
						map_swap(map, it->first, target->first);
						*didSwap = true;
					}

					_imgui_tooltip(STRING_IMGUI_TOOLTIP_TIMELINE_ELEMENT_SHIFT_UP);
					
				}

				if (canMoveDown)
				{
					if (!canMoveUp)
						ImGui::Dummy(IMGUI_ICON_BUTTON_SIZE);

					auto target = isReversed ? std::prev(it) : std::next(it);
					if (target != map.end() && ImGui::ImageButton(STRING_IMGUI_TIMELINE_ELEMENT_SHIFT_BELOW, self->packed->textures[PACKED_TEXTURE_ARROW_DOWN].handle, IMGUI_ICON_SIZE))
					{
						map_swap(map, it->first, target->first);
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
					arrows_draw(it, self->anm2->layers.begin(), self->anm2->layers.end(), self->anm2->layers, &isSwap, isReversed);
			}

			if (type == ANM2_NULL_ANIMATION)
			{
				auto it = std::find_if(self->anm2->nulls.begin(), self->anm2->nulls.end(),
					[&](const auto& pair) { return &pair.second == null; });

				if (it != self->anm2->nulls.end())
					arrows_draw(it, self->anm2->nulls.begin(), self->anm2->nulls.end(), self->anm2->nulls, &isSwap, isReversed);
			}
		}

		ImGui::BeginGroup();

		ImGui::Image(iconTexture, IMGUI_ICON_SIZE);
		ImGui::SameLine();

		ImGui::BeginChild(STRING_IMGUI_TIMELINE_ELEMENT_NAME_LABEL, IMGUI_TIMELINE_ELEMENT_NAME_SIZE);

		if (isSelected && isChangeable)
		{
			if (ImGui::InputText(STRING_IMGUI_TIMELINE_ANIMATION_LABEL, nameBuffer, ANM2_STRING_MAX, ImGuiInputTextFlags_EnterReturnsTrue))
			{
				strncpy(namePointer, nameBuffer, ANM2_STRING_MAX);
				selectedElementIndex = -1;
			}
		}
		else
		{
			if (ImGui::Selectable(nameVisible, isSelected))
			{
				selectedElementIndex = *index;

				if (selectType && selectID)
				{
					*selectType = type;
					*selectID = *id;
				}
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
			case ANM2_TRIGGERS:
				_imgui_tooltip(STRING_IMGUI_TOOLTIP_TIMELINE_ELEMENT_TRIGGERS);
				break;
			default:
				break;
		}

		ImGui::EndChild();

		/* Visiblity */
		if (isVisible)
		{
			ImTextureID visibilityIcon = *isVisible
				? self->packed->textures[PACKED_TEXTURE_EYE_OPEN].handle
				: self->packed->textures[PACKED_TEXTURE_EYE_CLOSED].handle;

			ImGui::SameLine();
				
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - IMGUI_ICON_BUTTON_SIZE.x - ImGui::GetStyle().FramePadding.x * 2);
			
			if (ImGui::ImageButton(STRING_IMGUI_TIMELINE_VISIBLE, visibilityIcon, IMGUI_ICON_SIZE))
				*isVisible = !*isVisible;

			_imgui_tooltip(STRING_IMGUI_TOOLTIP_TIMELINE_ELEMENT_VISIBLE);
		}
		
		/* Spritesheet IDs */
		if (spritesheetID)
		{
			char spritesheetIDName[ANM2_STRING_FORMATTED_MAX];

			if (*spritesheetID == -1)
				snprintf(spritesheetIDName, ANM2_STRING_FORMATTED_MAX, STRING_IMGUI_TIMELINE_SPRITESHEET_UNKNOWN);
			else
				snprintf(spritesheetIDName, ANM2_STRING_FORMATTED_MAX, STRING_IMGUI_TIMELINE_SPRITESHEET_FORMAT, *spritesheetID);

			ImGui::Image(self->packed->textures[PACKED_TEXTURE_SPRITESHEET].handle, IMGUI_ICON_SIZE);
			ImGui::SameLine();

			ImGui::BeginChild(STRING_IMGUI_TIMELINE_ELEMENT_SPRITESHEET_ID_LABEL, IMGUI_TIMELINE_SPRITESHEET_ID_SIZE);

			if (selectedSpritesheetIndex == *index)
			{
				if (ImGui::DragInt(STRING_IMGUI_TIMELINE_ELEMENT_SPRITESHEET_ID_LABEL, spritesheetID, 0, 1, self->anm2->spritesheets.size() - 1))
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
				? self->packed->textures[PACKED_TEXTURE_RECT_HIDE].handle
				: self->packed->textures[PACKED_TEXTURE_RECT_SHOW].handle;

			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - IMGUI_ICON_BUTTON_SIZE.x - ImGui::GetStyle().FramePadding.x * 2);
			
			if (ImGui::ImageButton(STRING_IMGUI_TIMELINE_RECT, rectIcon, IMGUI_ICON_SIZE))
				*isShowRect = !*isShowRect;

			_imgui_tooltip(STRING_IMGUI_TOOLTIP_TIMELINE_ELEMENT_RECT);
		}

		if (ImGui::IsMouseClicked(0) && !ImGui::IsAnyItemHovered() && ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
    	{
			selectedElementIndex = -1;
			selectedSpritesheetIndex = -1;
		}

		ImGui::PopID();
		
		ImGui::EndGroup();
	
		ImGui::EndChild();

		*index = *index + 1;
}

void
imgui_init(SDL_Window* window, SDL_GLContext glContext)
{
	IMGUI_CHECKVERSION();

	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	ImGui_ImplSDL3_InitForOpenGL(window, glContext);
	ImGui_ImplOpenGL3_Init(STRING_OPENGL_VERSION);

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	printf(STRING_INFO_IMGUI_INIT);
}

void
imgui_tick(Imgui* self)
{
	static s32 selectedSpritesheetID = -1;
	static s32 selectedEventID = -1;
	static s32 selectedAnimationID = -1;
	static s32 selectedTimelineElementID = -1;
	static Anm2AnimationType selectedTimelineElementType = ANM2_NONE;
	static bool isInterpolated = true;
	static bool isVisible = true;

	static f32 rotation = 0;
	static s32 duration = 0;
	static vec2 cropPosition = {0, 0};
	static vec2 cropSize = {0, 0};
	static vec2 pivot = {0, 0};
	static vec2 position = {0, 0};
	static vec2 xyScale = {0, 0};
	static vec3 offset = {0.0f, 0.0f, 0.0f};
	static vec4 tint = {0.0f, 0.0f, 0.0f, 1.0f};

	ImVec2 previewWindowSize;
	ImVec2 previewSize;
	f32 previewAspect = 0.0f;
	f32 previewWindowAspect = 0.0f;

	ImGuiWindowFlags taskbarWindowFlags;
	ImGuiWindowFlags dockspaceWindowFlags;
	ImGuiDockNodeFlags dockNodeFlags;
	
	ImGui_ImplSDL3_NewFrame();
	ImGui_ImplOpenGL3_NewFrame();
	ImGui::NewFrame();

	taskbarWindowFlags = 0 |
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoSavedSettings;

	dockspaceWindowFlags = 0 |
		ImGuiWindowFlags_NoTitleBar | 
		ImGuiWindowFlags_NoCollapse | 
		ImGuiWindowFlags_NoResize | 
		ImGuiWindowFlags_NoMove | 
		ImGuiWindowFlags_NoBringToFrontOnFocus | 
		ImGuiWindowFlags_NoNavFocus;

	dockNodeFlags = 0 |
		ImGuiDockNodeFlags_PassthruCentralNode; 
		
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	
	/* Taskbar */
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
			selectedAnimationID = -1;
			selectedSpritesheetID = -1;
			selectedEventID = -1;
			anm2_new(self->anm2);
			window_title_from_anm2_set(self->window, self->anm2);

		}

		if (ImGui::Selectable(STRING_IMGUI_FILE_OPEN))
		{
			selectedAnimationID = -1;
			selectedEventID = -1;
			selectedSpritesheetID = -1;
			dialog_anm2_open(self->dialog);
			window_title_from_anm2_set(self->window, self->anm2);
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
			window_title_from_anm2_set(self->window, self->anm2);
		}
		
		ImGui::EndPopup();
	}

	ImGui::End();
	
	/* Dockspace */
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + IMGUI_TASKBAR_HEIGHT));
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, viewport->Size.y - IMGUI_TASKBAR_HEIGHT));
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGui::Begin(STRING_IMGUI_WINDOW, NULL, dockspaceWindowFlags);

    ImGuiID dockspace_id = ImGui::GetID(STRING_IMGUI_DOCKSPACE);
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockNodeFlags);

    ImGui::End();
	
	/* -- Properties -- */
	ImGui::Begin(STRING_IMGUI_PROPERTIES);

	/* FPS */
	ImGui::AlignTextToFramePadding();
	ImGui::Text(STRING_IMGUI_PROPERTIES_FPS);
	ImGui::SameLine();
	ImGui::SliderInt(STRING_IMGUI_PROPERTIES_FPS_LABEL, &self->anm2->fps, ANM2_FPS_MIN, ANM2_FPS_MAX);
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_PROPERTIES_FPS);
	
	/* CreatedBy */
	ImGui::AlignTextToFramePadding();
	ImGui::Text(STRING_IMGUI_PROPERTIES_CREATED_BY);
	ImGui::SameLine();
	ImGui::InputText(STRING_IMGUI_PROPERTIES_CREATED_BY_LABEL, self->anm2->createdBy, ANM2_STRING_MAX);
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_PROPERTIES_CREATED_BY);
	
	/* CreatedOn */
	ImGui::Text(STRING_IMGUI_PROPERTIES_CREATED_ON, self->anm2->createdOn);

	/* Version */
	ImGui::Text(STRING_IMGUI_PROPERTIES_VERSION, self->anm2->version);

	ImGui::End();
	
	/* -- Animations --  */
	ImGui::Begin(STRING_IMGUI_ANIMATIONS);

	/* Iterate through all animations, can be selected and names can be edited */
	for (auto & [id, animation] : self->anm2->animations)
	{
		char name[ANM2_STRING_FORMATTED_MAX];
		char oldName[ANM2_STRING_MAX];
		bool isSelected = selectedAnimationID == id;
		static s32 defaultAnimationID = -1;

		/* Distinguish default animation; if animation has the same name, only mark the first one as default */
		if (strcmp(animation.name, self->anm2->defaultAnimation) == 0)
		{
			snprintf(name, ANM2_STRING_FORMATTED_MAX, STRING_IMGUI_ANIMATIONS_DEFAULT_ANIMATION_FORMAT, animation.name);
			defaultAnimationID = id;
		}
		else
			strncpy(name, animation.name, ANM2_STRING_FORMATTED_MAX - 1);

		ImGui::PushID(id);

		ImGui::Image(self->packed->textures[PACKED_TEXTURE_ANIMATION].handle, IMGUI_ICON_SIZE);
		ImGui::SameLine();

		if (isSelected)
		{
			if (ImGui::InputText(STRING_IMGUI_ANIMATIONS_ANIMATION_LABEL, animation.name, ANM2_STRING_MAX, ImGuiInputTextFlags_EnterReturnsTrue))
			{
				if (id == defaultAnimationID)
					strncpy(self->anm2->defaultAnimation, animation.name, ANM2_STRING_MAX);

				selectedAnimationID = -1;
			}
		}
		else
		{
			if (ImGui::Selectable(name, isSelected))
				selectedAnimationID = id;
		}

		_imgui_tooltip(STRING_IMGUI_TOOLTIP_ANIMATIONS_SELECT);

		ImGui::PopID();
	}

	if (ImGui::Button(STRING_IMGUI_ANIMATIONS_ADD))
	{
		s32 id;
		bool isDefault = self->anm2->animations.size() == 0; /* first animation is default automatically */

		id = anm2_animation_add(self->anm2);
		
		selectedAnimationID = id;

		if (isDefault)
			strncpy(self->anm2->defaultAnimation, self->anm2->animations[id].name, ANM2_STRING_MAX);
	}
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_ANIMATIONS_ADD);
		
	ImGui::SameLine();
	
	if (selectedAnimationID != -1)
	{
		/* Remove */
		if (ImGui::Button(STRING_IMGUI_ANIMATIONS_REMOVE))
		{
			anm2_animation_remove(self->anm2, selectedAnimationID);
			selectedAnimationID = -1;
		}
		_imgui_tooltip(STRING_IMGUI_TOOLTIP_ANIMATIONS_REMOVE);
		
		ImGui::SameLine();

		/* Duplicate */
		if (ImGui::Button(STRING_IMGUI_ANIMATIONS_DUPLICATE))
		{
			s32 id = map_next_id_get(self->anm2->animations);
			self->anm2->animations.insert({id, self->anm2->animations[selectedAnimationID]});
			selectedAnimationID = id;
		}
		_imgui_tooltip(STRING_IMGUI_TOOLTIP_ANIMATIONS_DUPLICATE);

		if (ImGui::Button(STRING_IMGUI_ANIMATIONS_SET_AS_DEFAULT))
		{
			strncpy(self->anm2->defaultAnimation, self->anm2->animations[selectedAnimationID].name, ANM2_STRING_MAX);
			selectedAnimationID = -1;
		}

		_imgui_tooltip(STRING_IMGUI_TOOLTIP_ANIMATIONS_SET_AS_DEFAULT);
	}

	if (ImGui::IsMouseClicked(0) && !ImGui::IsAnyItemHovered() && ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
    	selectedAnimationID = -1;

	ImGui::End();

	/* -- Events --  */
	ImGui::Begin(STRING_IMGUI_EVENTS);

	/* Iterate through all events, can be selected and names can be edited */
	for (auto & [id, event] : self->anm2->events)
	{
		char eventString[ANM2_STRING_FORMATTED_MAX];
		bool isSelected;
		
		snprintf(eventString, ANM2_STRING_FORMATTED_MAX, STRING_IMGUI_EVENT_FORMAT, (s32)id, event.name); 

		ImGui::PushID(id);
		
		ImGui::Image(self->packed->textures[PACKED_TEXTURE_EVENT].handle, IMGUI_ICON_SIZE);
		ImGui::SameLine();

		isSelected = selectedEventID == id;
		
		if (isSelected)
		{
			if (ImGui::InputText(STRING_IMGUI_EVENTS_EVENT_LABEL, event.name, ANM2_STRING_MAX, ImGuiInputTextFlags_EnterReturnsTrue))
				selectedEventID = -1;
		}
		else
		{
			if (ImGui::Selectable(eventString, isSelected))
				selectedEventID = id;
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
	
	if (selectedEventID != -1)
	{
		if (ImGui::Button(STRING_IMGUI_EVENTS_REMOVE))
		{
			self->anm2->events.erase(selectedEventID);
			selectedEventID = -1;
		}
		_imgui_tooltip(STRING_IMGUI_TOOLTIP_EVENTS_REMOVE);
	}

	if (ImGui::IsMouseClicked(0) && !ImGui::IsAnyItemHovered() && ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
    	selectedEventID = -1;

	ImGui::End();

	/* -- Spritesheets --  */
	ImGui::Begin(STRING_IMGUI_SPRITESHEETS);

	for (auto [id, spritesheet] : self->anm2->spritesheets)
	{
		char spritesheetString[ANM2_STRING_FORMATTED_MAX];
		bool isSelected = false;

		snprintf(spritesheetString, ANM2_STRING_FORMATTED_MAX, STRING_IMGUI_SPRITESHEET_FORMAT, (s32)id, spritesheet.path);
	
		ImGui::BeginChild(spritesheetString, IMGUI_SPRITESHEET_SIZE, true, ImGuiWindowFlags_None);
		
		ImGui::PushID(id);

		ImGui::Image(self->packed->textures[PACKED_TEXTURE_SPRITESHEET].handle, IMGUI_ICON_SIZE);
		ImGui::SameLine();

		isSelected = selectedSpritesheetID == id;
		if (ImGui::Selectable(spritesheetString, isSelected))
			selectedSpritesheetID = id;

		ImGui::PopID();
			
		ImGui::EndChild();
	}

	ImGui::Button(STRING_IMGUI_SPRITESHEETS_ADD);
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_SPRITESHEETS_ADD);
	
	ImGui::SameLine();

	if (selectedSpritesheetID != -1)
	{
		if (ImGui::Button(STRING_IMGUI_SPRITESHEETS_REMOVE))
		{
			self->anm2->spritesheets.erase(selectedSpritesheetID);
			selectedSpritesheetID = -1;
		}
		_imgui_tooltip(STRING_IMGUI_TOOLTIP_SPRITESHEETS_REMOVE);

		ImGui::SameLine();
		ImGui::Button(STRING_IMGUI_SPRITESHEETS_RELOAD);
		_imgui_tooltip(STRING_IMGUI_TOOLTIP_SPRITESHEETS_RELOAD);
		ImGui::SameLine();
		ImGui::Button(STRING_IMGUI_SPRITESHEETS_REPLACE);
		_imgui_tooltip(STRING_IMGUI_TOOLTIP_SPRITESHEETS_REPLACE);
	}

	if (ImGui::IsMouseClicked(0) && !ImGui::IsAnyItemHovered() && ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
    	selectedSpritesheetID = -1;
		
	ImGui::End();

	/* -- Frame Properties --  */

	ImGui::Begin(STRING_IMGUI_FRAME_PROPERTIES);

	ImGui::DragFloat2(STRING_IMGUI_FRAME_PROPERTIES_POSITION, value_ptr(position), 1, 0, 0, "%.0f");
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_POSITION);

	ImGui::DragFloat2(STRING_IMGUI_FRAME_PROPERTIES_CROP_POSITION, value_ptr(cropPosition), 1, 0, 0, "%.0f");
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_CROP_POSITION);
	
	ImGui::DragFloat2(STRING_IMGUI_FRAME_PROPERTIES_CROP_SIZE, value_ptr(cropSize), 1, 0, 0, "%.0f");
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_CROP_SIZE);
	
	ImGui::DragFloat2(STRING_IMGUI_FRAME_PROPERTIES_PIVOT, value_ptr(pivot), 1, 0, 0, "%.0f");
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_PIVOT);

	ImGui::DragFloat2(STRING_IMGUI_FRAME_PROPERTIES_SCALE, value_ptr(xyScale), 1.0, 0, 0, "%.1f");
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_SCALE);
	
	ImGui::DragFloat(STRING_IMGUI_FRAME_PROPERTIES_ROTATION, &rotation, 1, 0, 0, "%.1f");
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_ROTATION);
	
	ImGui::DragInt(STRING_IMGUI_FRAME_PROPERTIES_DURATION, &duration, 1, 0, 255);
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_DURATION);
	
	ImGui::ColorEdit4(STRING_IMGUI_FRAME_PROPERTIES_TINT, value_ptr(tint));
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_TINT);
	
	ImGui::ColorEdit3(STRING_IMGUI_FRAME_PROPERTIES_COLOR_OFFSET, value_ptr(offset));
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_COLOR_OFFSET);
	
	ImGui::Checkbox(STRING_IMGUI_FRAME_PROPERTIES_VISIBLE, &isVisible);
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_VISIBLE);
	
	ImGui::SameLine();
	
	ImGui::Checkbox(STRING_IMGUI_FRAME_PROPERTIES_INTERPOLATED, &isInterpolated);
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_INTERPOLATED);
	
	ImGui::End();

	/* -- Animation Preview --  */

	ImGui::Begin(STRING_IMGUI_ANIMATION_PREVIEW);
	
	/* Settings */
	ImGui::BeginChild(STRING_IMGUI_ANIMATION_PREVIEW_SETTINGS, IMGUI_ANIMATION_PREVIEW_SETTINGS_SIZE, true);

	/* Grid settings */
	ImGui::BeginChild(STRING_IMGUI_ANIMATION_PREVIEW_GRID_SETTINGS, IMGUI_ANIMATION_PREVIEW_SETTINGS_GRID_SIZE, true);

	/* Grid toggle */
	ImGui::Checkbox(STRING_IMGUI_ANIMATION_PREVIEW_GRID, &self->preview->isGrid);
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_ANIMATION_PREVIEW_GRID);

	ImGui::SameLine();
	
	/* Grid Color */
	ImGui::ColorEdit4(STRING_IMGUI_ANIMATION_PREVIEW_GRID_COLOR, value_ptr(self->preview->gridColor), ImGuiColorEditFlags_NoInputs);
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_ANIMATION_PREVIEW_GRID_COLOR);
	
	/* Grid Size */
	ImGui::InputInt2(STRING_IMGUI_ANIMATION_PREVIEW_GRID_SIZE, value_ptr(self->preview->gridSize));
	self->preview->gridSize.x = CLAMP(self->preview->gridSize.x, PREVIEW_GRID_MIN, PREVIEW_GRID_MAX);
	self->preview->gridSize.y = CLAMP(self->preview->gridSize.y, PREVIEW_GRID_MIN, PREVIEW_GRID_MAX);
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_ANIMATION_PREVIEW_GRID_SIZE);
	
	ImGui::EndChild();
	
	ImGui::SameLine();
	
	/* Zoom settings */
	ImGui::BeginChild(STRING_IMGUI_ANIMATION_PREVIEW_ZOOM_SETTINGS, IMGUI_ANIMATION_PREVIEW_SETTINGS_ZOOM_SIZE, true);

	/* Zoom */
	ImGui::DragFloat(STRING_IMGUI_ANIMATION_PREVIEW_ZOOM, &self->preview->zoom, 1, PREVIEW_ZOOM_MIN, PREVIEW_ZOOM_MAX, "%.0f");
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_ANIMATION_PREVIEW_ZOOM);
	
	ImGui::EndChild();
	
	ImGui::SameLine();
	
	/* Background settings */
	ImGui::BeginChild(STRING_IMGUI_ANIMATION_PREVIEW_BACKGROUND_SETTINGS, IMGUI_ANIMATION_PREVIEW_SETTINGS_BACKGROUND_SIZE, true);

	/* Background color */
	ImGui::ColorEdit4(STRING_IMGUI_ANIMATION_PREVIEW_BACKGROUND_COLOR, value_ptr(self->preview->color), ImGuiColorEditFlags_NoInputs);
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_ANIMATION_PREVIEW_BACKGROUND_COLOR);
	
	ImGui::EndChild();
	
	ImGui::SameLine();

	ImGui::EndChild();

	ImGui::Image(self->preview->texture, ImVec2(PREVIEW_SIZE.x, PREVIEW_SIZE.y));
	
	ImGui::End();
	
	/* -- Spritesheet Editor -- */

	ImGui::Begin(STRING_IMGUI_SPRITESHEET_EDITOR);
	
	ImGui::End();
	
	/* -- Timeline -- */
	ImGui::Begin(STRING_IMGUI_TIMELINE);

	if (selectedAnimationID != -1)
	{
		s32 index = 0;
		Anm2Animation* animation = &self->anm2->animations[selectedAnimationID];

		ImGui::BeginChild(STRING_IMGUI_TIMELINE_ANIMATIONS, IMGUI_TIMELINE_ELEMENT_LIST_SIZE, true);
		
		/* Root */
		_imgui_timeline_element(self, &animation->rootAnimation, NULL, &index, ANM2_ROOT_ANIMATION, NULL, NULL); 

		/* reverse order */
		for (auto it = animation->layerAnimations.rbegin(); it != animation->layerAnimations.rend(); it++)
		{
			s32 id = it->first;
			Anm2LayerAnimation& layer = it->second;

			_imgui_timeline_element(self, &layer, &id, &index, ANM2_LAYER_ANIMATION, &selectedTimelineElementType, &selectedTimelineElementID); 
		}

		for (auto & [id, null] : animation->nullAnimations)
			_imgui_timeline_element(self, &null, (s32*)&id, &index, ANM2_NULL_ANIMATION, &selectedTimelineElementType, &selectedTimelineElementID); 

		/* Triggers */
		_imgui_timeline_element(self, &animation->triggers, NULL, &index, ANM2_TRIGGERS, NULL, NULL); 
		
		/* Element configuration */
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
			selectedTimelineElementID != -1 && 
			((selectedTimelineElementType == ANM2_LAYER_ANIMATION) || (selectedTimelineElementType == ANM2_NULL_ANIMATION))
		)
		{
			if (ImGui::Button(STRING_IMGUI_TIMELINE_ELEMENT_REMOVE))
			{
				switch (selectedTimelineElementType)
				{
					case ANM2_LAYER_ANIMATION:
						anm2_layer_remove(self->anm2, selectedTimelineElementID);
						break;
					case ANM2_NULL_ANIMATION:
						anm2_null_remove(self->anm2, selectedTimelineElementID);
						break;
					default:
						break;
				}

				selectedTimelineElementID = -1;
				selectedTimelineElementType = ANM2_NONE;
			}

			_imgui_tooltip(STRING_IMGUI_TOOLTIP_TIMELINE_ELEMENT_REMOVE);
		}
		else
		{
			selectedTimelineElementID = -1;
			selectedTimelineElementType = ANM2_NONE;
		}

		ImGui::EndChild();

		ImGui::SameLine();

		/* Animation playback and frames */
		static bool isPlaying = false;

		if (isPlaying)
		{
			if (ImGui::Button(STRING_IMGUI_TIMELINE_PAUSE))
			{
				isPlaying = !isPlaying;
			}
			_imgui_tooltip(STRING_IMGUI_TOOLTIP_TIMELINE_PLAY);
			
		}
		else
		{
			if (ImGui::Button(STRING_IMGUI_TIMELINE_PLAY))
			{
				isPlaying = !isPlaying;
			}
			_imgui_tooltip(STRING_IMGUI_TOOLTIP_TIMELINE_PAUSE);
		}

		/*
		ImGui::SameLine();

		if (ImGui::Button(STRING_IMGUI_TIMELINE_ADD_FRAME))
		{

		}
		_imgui_tooltip(STRING_IMGUI_TOOLTIP_TIMELINE_FRAME_ADD);

		ImGui::SameLine();

		if (ImGui::Button(STRING_IMGUI_TIMELINE_REMOVE_FRAME))
		{

		}
		*/
	}

	ImGui::End();

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
	ImGui::DestroyContext();

	printf(STRING_INFO_IMGUI_FREE);
}
