#include "anm2.h"

using namespace tinyxml2;

static void _anm2_created_on_set(Anm2* self)
{
	auto now = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::tm localTime = *std::localtime(&time);

	std::ostringstream timeString;
	timeString << std::put_time(&localTime, ANM2_CREATED_ON_FORMAT);
	self->createdOn = timeString.str();
}

bool anm2_serialize(Anm2* self, const std::string& path)
{
	XMLDocument document;
	XMLError error;

	XMLElement* animatedActorElement;
	XMLElement* infoElement;
	XMLElement* contentElement;
	XMLElement* spritesheetsElement;
	XMLElement* layersElement;
	XMLElement* nullsElement;
	XMLElement* eventsElement;
	XMLElement* animationsElement;

	if (!self || path.empty()) return false;

	if (self->version == 0) _anm2_created_on_set(self);

	self->path = path;
	self->version++;

	// AnimatedActor
	animatedActorElement = document.NewElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_ANIMATED_ACTOR]);
	document.InsertFirstChild(animatedActorElement);

	// Info 
	infoElement = document.NewElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_INFO]);
	infoElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_CREATED_BY], self->createdBy.c_str()); // CreatedBy 
	infoElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_CREATED_ON], self->createdOn.c_str()); // CreatedOn 
	infoElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_VERSION], self->version); // Version
	infoElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_FPS], self->fps); // FPS 
	animatedActorElement->InsertEndChild(infoElement);

	// Content 
	contentElement = document.NewElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_CONTENT]);
	
	// Spritesheets 
	spritesheetsElement = document.NewElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_SPRITESHEETS]);

	for (auto& [id, spritesheet] : self->spritesheets)
	{
		if (id == ID_NONE) continue;
		
		XMLElement* spritesheetElement;
		
		// Spritesheet 
		spritesheetElement = document.NewElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_SPRITESHEET]);
		spritesheetElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_PATH], spritesheet.path.c_str()); // Path 
		spritesheetElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_ID], id); // ID 
		spritesheetsElement->InsertEndChild(spritesheetElement);
	}
	
	contentElement->InsertEndChild(spritesheetsElement);
	
	// Layers 
	layersElement = document.NewElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_LAYERS]);

	for (auto& [id, layer] : self->layers)
	{
		if (id == ID_NONE) continue;

		XMLElement* layerElement;
		
		// Layer 
		layerElement = document.NewElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_LAYER]);
		layerElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_NAME], layer.name.c_str()); // Path 
		layerElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_ID], id); // ID 
		layerElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_SPRITESHEET_ID], layer.spritesheetID); // SpritesheetId 
		
		layersElement->InsertEndChild(layerElement);
	}
	
	contentElement->InsertEndChild(layersElement);
	
	// Nulls 
	nullsElement = document.NewElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_NULLS]);

	for (auto& [id, null] : self->nulls)
	{
		if (id == ID_NONE) continue;
		
		XMLElement* nullElement;
		
		// Null 
		nullElement = document.NewElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_NULL]);
		nullElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_NAME], null.name.c_str()); // Name 
		nullElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_ID], id); // ID 

		// special case; only serialize if this is true 
		if (null.isShowRect)
			nullElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_SHOW_RECT], null.isShowRect); // ShowRect 
		nullsElement->InsertEndChild(nullElement);
	}
	
	contentElement->InsertEndChild(nullsElement);

	// Events 
	eventsElement = document.NewElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_EVENTS]);

	for (auto& [id, event] : self->events)
	{
		if (id == ID_NONE) continue;
		
		XMLElement* eventElement;
		
		// Event 
		eventElement = document.NewElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_EVENT]);
		eventElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_NAME], event.name.c_str()); // Name 
		eventElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_ID], id); // ID 
		eventsElement->InsertEndChild(eventElement);
	}
	
	contentElement->InsertEndChild(eventsElement);

	animatedActorElement->InsertEndChild(contentElement);
		
	// Animations
	animationsElement = document.NewElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_ANIMATIONS]);
	
	if (self->defaultAnimationID != ID_NONE)
		animationsElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_DEFAULT_ANIMATION], self->animations[self->defaultAnimationID].name.c_str()); // DefaultAnimation

	for (auto& [id, animation] : self->animations)
	{
		if (id == ID_NONE) continue;
		
		XMLElement* animationElement;
		XMLElement* rootAnimationElement;
		XMLElement* layerAnimationsElement;
		XMLElement* nullAnimationsElement;
		XMLElement* triggersElement;
		
		// Animation
		animationElement = document.NewElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_ANIMATION]);

		animationElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_NAME], animation.name.c_str()); // Name
		animationElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_FRAME_NUM], animation.frameNum); // FrameNum
		animationElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_LOOP], animation.isLoop); // Loop

		// RootAnimation
		rootAnimationElement = document.NewElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_ROOT_ANIMATION]);

		for (auto& frame : animation.rootAnimation.frames)
		{
			XMLElement* frameElement;

			// Frame
			frameElement = document.NewElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_FRAME]);
			
			frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_X_POSITION], frame.position.x); // XPosition
			frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_Y_POSITION], frame.position.y); //  YPosition
			frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_X_PIVOT], frame.pivot.x); // XPivot
			frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_Y_PIVOT], frame.pivot.y); // YPivot
			frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_X_SCALE], frame.scale.x); // XScale
			frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_Y_SCALE], frame.scale.y); // YScale
			frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_DELAY], frame.delay); // Delay
			frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_VISIBLE], frame.isVisible); // Visible
			frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_RED_TINT], FLOAT_TO_U8(frame.tintRGBA.r)); // RedTint
			frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_GREEN_TINT], FLOAT_TO_U8(frame.tintRGBA.g)); // GreenTint
			frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_BLUE_TINT], FLOAT_TO_U8(frame.tintRGBA.b)); // BlueTint
			frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_ALPHA_TINT], FLOAT_TO_U8(frame.tintRGBA.a)); // AlphaTint
			frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_RED_OFFSET], FLOAT_TO_U8(frame.offsetRGB.r)); // RedOffset
			frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_GREEN_OFFSET], FLOAT_TO_U8(frame.offsetRGB.g)); // GreenOffset
			frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_BLUE_OFFSET], FLOAT_TO_U8(frame.offsetRGB.b)); // BlueOffset
			frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_ROTATION], frame.rotation); // Rotation
			frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_INTERPOLATED], frame.isInterpolated); // Interpolated
		
			rootAnimationElement->InsertEndChild(frameElement);
		}

		animationElement->InsertEndChild(rootAnimationElement);

		// LayerAnimations
		layerAnimationsElement = document.NewElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_LAYER_ANIMATIONS]);

		for (auto& [layerIndex, layerID] : self->layerMap)
		{
			Anm2Item& layerAnimation = animation.layerAnimations[layerID];

			XMLElement* layerAnimationElement;

			// LayerAnimation
			layerAnimationElement = document.NewElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_LAYER_ANIMATION]);
			layerAnimationElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_LAYER_ID], layerID); // LayerId
			layerAnimationElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_VISIBLE], layerAnimation.isVisible); // Visible 
	
			for (auto& frame : layerAnimation.frames)
			{
				XMLElement* frameElement;

				// Frame
				frameElement = document.NewElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_FRAME]);
				
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_X_POSITION], frame.position.x); // XPosition
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_Y_POSITION], frame.position.y); // YPosition
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_X_PIVOT], frame.pivot.x); // XPivot
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_Y_PIVOT], frame.pivot.y); // YPivot
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_X_CROP], frame.crop.x); // XCrop
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_Y_CROP], frame.crop.y); // YCrop
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_WIDTH], frame.size.x); // Width
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_HEIGHT], frame.size.y); // Height
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_X_SCALE], frame.scale.x); // XScale
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_Y_SCALE], frame.scale.y); // YScale
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_DELAY], frame.delay); /* Delay */
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_VISIBLE], frame.isVisible); // Visible
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_RED_TINT], FLOAT_TO_U8(frame.tintRGBA.r)); // RedTint
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_GREEN_TINT], FLOAT_TO_U8(frame.tintRGBA.g)); // GreenTint
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_BLUE_TINT], FLOAT_TO_U8(frame.tintRGBA.b)); // BlueTint
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_ALPHA_TINT], FLOAT_TO_U8(frame.tintRGBA.a)); // AlphaTint
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_RED_OFFSET], FLOAT_TO_U8(frame.offsetRGB.r)); // RedOffset
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_GREEN_OFFSET], FLOAT_TO_U8(frame.offsetRGB.g)); // GreenOffset
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_BLUE_OFFSET], FLOAT_TO_U8(frame.offsetRGB.b)); // BlueOffset
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_ROTATION], frame.rotation); // Rotation
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_INTERPOLATED], frame.isInterpolated); // Interpolated
			
				layerAnimationElement->InsertEndChild(frameElement);
			}

			layerAnimationsElement->InsertEndChild(layerAnimationElement);
		}

		animationElement->InsertEndChild(layerAnimationsElement);

		// NullAnimations
		nullAnimationsElement = document.NewElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_NULL_ANIMATIONS]);

		for (const auto& [nullID, nullAnimation] : animation.nullAnimations)
		{
			XMLElement* nullAnimationElement;

			// NullAnimation
			nullAnimationElement = document.NewElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_NULL_ANIMATION]);
			nullAnimationElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_NULL_ID], nullID); // NullId
			nullAnimationElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_VISIBLE], nullAnimation.isVisible); // Visible 
			
			for (const auto& frame : nullAnimation.frames)
			{
				XMLElement* frameElement;

				// Frame
				frameElement = document.NewElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_FRAME]);
				
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_X_POSITION], frame.position.x); // XPosition
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_Y_POSITION], frame.position.y); // YPosition
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_X_PIVOT], frame.pivot.x); // XPivot
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_Y_PIVOT], frame.pivot.y); // YPivot
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_X_SCALE], frame.scale.x); // XScale
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_Y_SCALE], frame.scale.y); // XScale
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_DELAY], frame.delay); // Delay 
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_VISIBLE], frame.isVisible); // Visible 
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_RED_TINT], FLOAT_TO_U8(frame.tintRGBA.r)); // RedTint 
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_GREEN_TINT], FLOAT_TO_U8(frame.tintRGBA.g)); // GreenTint 
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_BLUE_TINT], FLOAT_TO_U8(frame.tintRGBA.b)); // BlueTint 
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_ALPHA_TINT], FLOAT_TO_U8(frame.tintRGBA.a)); // AlphaTint 
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_RED_OFFSET], FLOAT_TO_U8(frame.offsetRGB.r)); // RedOffset 
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_GREEN_OFFSET], FLOAT_TO_U8(frame.offsetRGB.g)); // GreenOffset 
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_BLUE_OFFSET], FLOAT_TO_U8(frame.offsetRGB.b)); // BlueOffset 
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_ROTATION], frame.rotation); // Rotation 
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_INTERPOLATED], frame.isInterpolated); // Interpolated 
			
				nullAnimationElement->InsertEndChild(frameElement);
			}

			nullAnimationsElement->InsertEndChild(nullAnimationElement);
		}

		animationElement->InsertEndChild(nullAnimationsElement);

		// Triggers 
		triggersElement = document.NewElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_TRIGGERS]);

		for (const auto& frame : animation.triggers.frames)
		{
			XMLElement* triggerElement;

			// Trigger 
			triggerElement = document.NewElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_TRIGGER]);
			triggerElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_EVENT_ID], frame.eventID); // EventID 
			triggerElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_AT_FRAME], frame.atFrame); // AtFrame 
			triggersElement->InsertEndChild(triggerElement);
		}

		animationElement->InsertEndChild(triggersElement);
		
		animationsElement->InsertEndChild(animationElement);
	}

	animatedActorElement->InsertEndChild(animationsElement);

	error = document.SaveFile(path.c_str());

	if (error != XML_SUCCESS)
	{
		log_error(std::format(ANM2_WRITE_ERROR, path));
		return false;
	}

	log_info(std::format(ANM2_WRITE_INFO, path));
	
	return true;
}

bool anm2_deserialize(Anm2* self, const std::string& path)
{
	XMLDocument xmlDocument;
	XMLError xmlError;
	const XMLElement* xmlElement;
	const XMLElement* xmlRoot;
	Anm2Animation* animation = nullptr;
	Anm2Layer* layer = nullptr;
	Anm2Null* null = nullptr;
	Anm2Item* item = nullptr;
	Anm2Event* event = nullptr;
	Anm2Frame* frame = nullptr;
	Anm2Spritesheet* spritesheet = nullptr;
	Anm2Element anm2Element = ANM2_ELEMENT_ANIMATED_ACTOR;
	Anm2Attribute anm2Attribute =  ANM2_ATTRIBUTE_ID;
	Anm2Item addItem;
	Anm2Layer addLayer;
	Anm2Null addNull;
	Anm2Event addEvent;
	Anm2Spritesheet addSpritesheet;
	s32 layerMapIndex = 0;
	bool isLayerMapSet = false;
	bool isFirstAnimationDone = false;
	std::string defaultAnimation{};

	if (!self || path.empty()) return false;

	anm2_new(self);

	xmlError = xmlDocument.LoadFile(path.c_str());

	if (xmlError != XML_SUCCESS)
	{
		log_error(std::format(ANM2_READ_ERROR, xmlDocument.ErrorStr()));
		return false;
	}

	// Save old working directory and then use anm2's path as directory
	// (used for loading textures from anm2 correctly which are relative)
    std::filesystem::path workingPath = std::filesystem::current_path();
	working_directory_from_file_set(path);
	
	self->path = path;
	
    xmlRoot = xmlDocument.FirstChildElement(ANM2_ELEMENT_ENUM_TO_STRING(ANM2_ELEMENT_ANIMATED_ACTOR).c_str());
	xmlElement = xmlRoot;

	// Iterate through elements
	while (xmlElement)
	{
		const XMLAttribute* xmlAttribute = nullptr;
		const XMLElement* xmlChild = nullptr;
		s32 id = 0;
		
		anm2Element = ANM2_ELEMENT_STRING_TO_ENUM(xmlElement->Name());

		switch (anm2Element)
		{
			case ANM2_ELEMENT_SPRITESHEET: // Spritesheet 
				spritesheet = &addSpritesheet;
				break;
			case ANM2_ELEMENT_LAYER: // Layer
				layer = &addLayer;
				break;
			case ANM2_ELEMENT_NULL: // Null
				null = &addNull;
				break;
			case ANM2_ELEMENT_EVENT: // Event
				event = &addEvent;
				break;
			case ANM2_ELEMENT_ANIMATION: // Animation
				id = map_next_id_get(self->animations);
				self->animations[id] = Anm2Animation{};
				animation = &self->animations[id];

				if (isFirstAnimationDone)
					isLayerMapSet = true;

				isFirstAnimationDone = true;
				break;
			case ANM2_ELEMENT_ROOT_ANIMATION: // RootAnimation
				item = &animation->rootAnimation;
				break;
			case ANM2_ELEMENT_LAYER_ANIMATION: // LayerAnimation
			case ANM2_ELEMENT_NULL_ANIMATION: // NullAnimation
				item = &addItem;
				break;
			case ANM2_ELEMENT_TRIGGERS: // Triggers
				item = &animation->triggers;
				break;
			case ANM2_ELEMENT_FRAME: // Frame
			case ANM2_ELEMENT_TRIGGER: // Trigger
				item->frames.push_back(Anm2Frame{});
				frame = &item->frames.back();
			default:
				break;
		}

		/* Attributes */
		xmlAttribute = xmlElement->FirstAttribute();
		
		while (xmlAttribute)
		{
			anm2Attribute = ANM2_ATTRIBUTE_STRING_TO_ENUM(xmlAttribute->Name());

			switch (anm2Attribute)
			{
				case ANM2_ATTRIBUTE_CREATED_BY: // CreatedBy
					self->createdBy = xmlAttribute->Value();
					break;
				case ANM2_ATTRIBUTE_CREATED_ON: // CreatedOn
					self->createdOn = xmlAttribute->Value();
					break;
				case ANM2_ATTRIBUTE_VERSION: // Version
					self->version = std::atoi(xmlAttribute->Value());
					break;
				case ANM2_ATTRIBUTE_FPS: // FPS
					self->fps = std::atoi(xmlAttribute->Value());
					break;
				case ANM2_ATTRIBUTE_ID: // ID
					id = std::atoi(xmlAttribute->Value());
					switch (anm2Element)
					{
						case ANM2_ELEMENT_SPRITESHEET: // Spritesheet
							self->spritesheets[id] = addSpritesheet;
							spritesheet = &self->spritesheets[id];
							break;
						case ANM2_ELEMENT_LAYER: // Layer
							self->layers[id] = addLayer;
							layer = &self->layers[id];
							break;
						case ANM2_ELEMENT_NULL: // Null
							self->nulls[id] = addNull;
							null = &self->nulls[id];
							break;
						case ANM2_ELEMENT_EVENT: // Event
							self->events[id] = addEvent;
							event = &self->events[id];
							break;
						default:
							break;
					}
					break;
				case ANM2_ATTRIBUTE_LAYER_ID: // LayerId
					id = std::atoi(xmlAttribute->Value());
					
					if (!isLayerMapSet)
					{
						self->layerMap[layerMapIndex] = id;
						layerMapIndex++;
					}

					animation->layerAnimations[id] = addItem;
					item = &animation->layerAnimations[id];
					break;
				case ANM2_ATTRIBUTE_NULL_ID: // NullId
					id = std::atoi(xmlAttribute->Value());
					animation->nullAnimations[id] = addItem;
					item = &animation->nullAnimations[id];
					break;
				case ANM2_ATTRIBUTE_PATH: // Path
					spritesheet->path = xmlAttribute->Value();
					break;
				case ANM2_ATTRIBUTE_NAME: // Name
					switch (anm2Element)
					{
						case ANM2_ELEMENT_LAYER:
							layer->name = std::string(xmlAttribute->Value());
							break;
						case ANM2_ELEMENT_NULL:
							null->name = std::string(xmlAttribute->Value());
							break;
						case ANM2_ELEMENT_ANIMATION:
							animation->name = std::string(xmlAttribute->Value());
							break;
						case ANM2_ELEMENT_EVENT:
							event->name = std::string(xmlAttribute->Value());
							break;
						default:
							break;
					}
					break;
				case ANM2_ATTRIBUTE_SPRITESHEET_ID:
					layer->spritesheetID = std::atoi(xmlAttribute->Value());
					break;
				case ANM2_ATTRIBUTE_SHOW_RECT:
					null->isShowRect = string_to_bool(xmlAttribute->Value());
					break;
				case ANM2_ATTRIBUTE_DEFAULT_ANIMATION:
					defaultAnimation = xmlAttribute->Value();
					break;
				case ANM2_ATTRIBUTE_FRAME_NUM:
					animation->frameNum = std::atoi(xmlAttribute->Value());
					break;
				case ANM2_ATTRIBUTE_LOOP:
					animation->isLoop = string_to_bool(xmlAttribute->Value());
					break;
				case ANM2_ATTRIBUTE_X_POSITION:
					frame->position.x = std::atof(xmlAttribute->Value());
					break;
				case ANM2_ATTRIBUTE_Y_POSITION:
					frame->position.y = std::atof(xmlAttribute->Value());
					break;
				case ANM2_ATTRIBUTE_X_PIVOT:
					frame->pivot.x = std::atof(xmlAttribute->Value());
					break;
				case ANM2_ATTRIBUTE_Y_PIVOT:
					frame->pivot.y = std::atof(xmlAttribute->Value());
					break;
				case ANM2_ATTRIBUTE_X_CROP:
					frame->crop.x = std::atof(xmlAttribute->Value());
					break;
				case ANM2_ATTRIBUTE_Y_CROP:
					frame->crop.y = std::atof(xmlAttribute->Value());
					break;	
				case ANM2_ATTRIBUTE_WIDTH:
					frame->size.x = std::atof(xmlAttribute->Value());
					break;		
				case ANM2_ATTRIBUTE_HEIGHT:
					frame->size.y = std::atof(xmlAttribute->Value());
					break;		
				case ANM2_ATTRIBUTE_X_SCALE:
					frame->scale.x = std::atof(xmlAttribute->Value());
					break;
				case ANM2_ATTRIBUTE_Y_SCALE:
					frame->scale.y = std::atof(xmlAttribute->Value());
					break;
				case ANM2_ATTRIBUTE_DELAY:
					frame->delay = std::atoi(xmlAttribute->Value());
					break;
				case ANM2_ATTRIBUTE_VISIBLE:
					switch (anm2Element)
					{
						case ANM2_ELEMENT_FRAME:
							frame->isVisible = string_to_bool(xmlAttribute->Value());
							break;
						case ANM2_ELEMENT_ROOT_ANIMATION:
						case ANM2_ELEMENT_LAYER_ANIMATION:
						case ANM2_ELEMENT_NULL_ANIMATION:
							item->isVisible = string_to_bool(xmlAttribute->Value());
							break;
						default:
							break;
					}
					break;
				case ANM2_ATTRIBUTE_RED_TINT:
					frame->tintRGBA.r = U8_TO_FLOAT(std::atoi(xmlAttribute->Value()));
					break;
				case ANM2_ATTRIBUTE_GREEN_TINT:
					frame->tintRGBA.g = U8_TO_FLOAT(std::atoi(xmlAttribute->Value()));
					break;
				case ANM2_ATTRIBUTE_BLUE_TINT:
					frame->tintRGBA.b = U8_TO_FLOAT(std::atoi(xmlAttribute->Value()));
					break;
				case ANM2_ATTRIBUTE_ALPHA_TINT:
					frame->tintRGBA.a = U8_TO_FLOAT(std::atoi(xmlAttribute->Value()));
					break;
				case ANM2_ATTRIBUTE_RED_OFFSET:
					frame->offsetRGB.r = U8_TO_FLOAT(std::atoi(xmlAttribute->Value()));
					break;
				case ANM2_ATTRIBUTE_GREEN_OFFSET:
					frame->offsetRGB.g = U8_TO_FLOAT(std::atoi(xmlAttribute->Value()));
					break;
				case ANM2_ATTRIBUTE_BLUE_OFFSET:
					frame->offsetRGB.b = U8_TO_FLOAT(std::atoi(xmlAttribute->Value()));
					break;
				case ANM2_ATTRIBUTE_ROTATION:
					frame->rotation = std::atof(xmlAttribute->Value());
					break;
				case ANM2_ATTRIBUTE_INTERPOLATED:
					frame->isInterpolated = string_to_bool(xmlAttribute->Value());
					break;
				case ANM2_ATTRIBUTE_EVENT_ID:
					frame->eventID = std::atoi(xmlAttribute->Value());
					break;
				case ANM2_ATTRIBUTE_AT_FRAME:
					frame->atFrame = std::atoi(xmlAttribute->Value());
					break;
				default:
					break;
			}

			xmlAttribute = xmlAttribute->Next();
		}

		if (anm2Element == ANM2_ELEMENT_SPRITESHEET)
			texture_from_path_init(&spritesheet->texture, spritesheet->path);

		xmlChild = xmlElement->FirstChildElement();

		if (xmlChild)
		{
			xmlElement = xmlChild;
			continue;
		}

		while (xmlElement)
		{
			const XMLElement* xmlNext;
			
			xmlNext = xmlElement->NextSiblingElement();
			
			if (xmlNext)
			{
				xmlElement = xmlNext;
				break;
			}

			xmlElement = xmlElement->Parent() ? xmlElement->Parent()->ToElement() : nullptr;
		}
	}

	// Set default animation ID
	for (auto& [id, animation] : self->animations)
		if (animation.name == defaultAnimation)
			self->defaultAnimationID = id;

	// Copy texture data to pixels (used for snapshots)
	anm2_spritesheet_texture_pixels_download(self);

	// Read
	log_info(std::format(ANM2_READ_INFO, path));
	
	// Return to old working directory
	std::filesystem::current_path(workingPath);

	return true;
}

void anm2_layer_add(Anm2* self)
{
	s32 id = map_next_id_get(self->layers);

	self->layers[id] = Anm2Layer{};
	self->layerMap[self->layers.size() - 1] = id;
	
	for (auto& [_, animation] : self->animations)
		animation.layerAnimations[id] = Anm2Item{};
}

void anm2_layer_remove(Anm2* self, s32 id)
{
    if (!self->layers.contains(id)) return;

    self->layers.erase(id);

    for (auto it = self->layerMap.begin(); it != self->layerMap.end(); ++it)
    {
        if (it->second == id)
        {
            self->layerMap.erase(it);
            break;
        }
    }

    std::map<s32, s32> newLayerMap;
    s32 newIndex = 0;

    for (const auto& [_, layerID] : self->layerMap)
        newLayerMap[newIndex++] = layerID;

    self->layerMap = std::move(newLayerMap);
 
	for (auto& [_, animation] : self->animations)
        animation.layerAnimations.erase(id);
}

void anm2_null_add(Anm2* self)
{
	s32 id = map_next_id_get(self->nulls);

	self->nulls[id] = Anm2Null{};

	for (auto& [_, animation] : self->animations)
		animation.nullAnimations[id] = Anm2Item{};
}

void anm2_null_remove(Anm2* self, s32 id)
{
    if (!self->nulls.contains(id))
        return;

    self->nulls.erase(id);

    std::map<s32, Anm2Null> newNulls;
    s32 newID = 0;
	
    for (const auto& [_, null] : self->nulls)
        newNulls[newID++] = null;

    self->nulls = std::move(newNulls);

    for (auto& [_, animation] : self->animations)
    {
        if (animation.nullAnimations.contains(id))
            animation.nullAnimations.erase(id);

        std::map<s32, Anm2Item> newNullAnims;
        s32 newAnimID = 0;
        for (const auto& [_, nullAnim] : animation.nullAnimations)
            newNullAnims[newAnimID++] = nullAnim;

        animation.nullAnimations = std::move(newNullAnims);
    }
}

s32 anm2_animation_add(Anm2* self)
{
    s32 id  = map_next_id_get(self->animations);
    Anm2Animation animation;

    for (auto& [layerID, layer] : self->layers) 
		animation.layerAnimations[layerID] = Anm2Item{};
    for (auto& [nullID, null] : self->nulls) 
		animation.nullAnimations[nullID] = Anm2Item{};

	animation.rootAnimation.frames.push_back(Anm2Frame{});
		
    self->animations[id] = animation;

	return id;
}

void anm2_animation_remove(Anm2* self, s32 id)
{
	self->animations.erase(id);
}

void anm2_new(Anm2* self)
{
	*self = Anm2{};
	_anm2_created_on_set(self);
}

Anm2Animation* anm2_animation_from_reference(Anm2* self, Anm2Reference* reference)
{
	return map_find(self->animations, reference->animationID);
}

Anm2Item* anm2_item_from_reference(Anm2* self, Anm2Reference* reference)
{
	if (reference->itemType == ANM2_NONE) return nullptr;

	Anm2Animation* animation = anm2_animation_from_reference(self, reference);
	
	if (!animation) return nullptr;

	switch (reference->itemType)
	{
		case ANM2_ROOT: return &animation->rootAnimation;
		case ANM2_LAYER: return map_find(animation->layerAnimations, reference->itemID);
		case ANM2_NULL: return map_find(animation->nullAnimations, reference->itemID);
		case ANM2_TRIGGERS: return &animation->triggers;
		default: return nullptr;
	}
}
Anm2Frame* anm2_frame_from_reference(Anm2* self, Anm2Reference* reference)
{
	Anm2Item* item = anm2_item_from_reference(self, reference);

	if (!item) return nullptr;
	
	if (reference->frameIndex <= INDEX_NONE || reference->frameIndex >= (s32)item->frames.size()) 
		return nullptr;

	return &item->frames[reference->frameIndex];
}

s32 anm2_frame_index_from_time(Anm2* self, Anm2Reference reference, f32 time)
{
	Anm2Animation* animation = anm2_animation_from_reference(self, &reference);

	if (!animation) return INDEX_NONE;
	if (time < 0 || time > animation->frameNum) return INDEX_NONE;

	Anm2Item* item = anm2_item_from_reference(self, &reference);

	if (!item) return INDEX_NONE;

	s32 delayCurrent = 0;
	s32 delayNext = 0;

	for (auto [i, frame] : std::views::enumerate(item->frames))
	{
		delayNext += frame.delay;

		if (time >= delayCurrent && time < delayNext)
			return i;

		delayCurrent += frame.delay;
	}

	return INDEX_NONE;
}

void anm2_frame_from_time(Anm2* self, Anm2Frame* frame, Anm2Reference reference, f32 time)
{
	Anm2Animation* animation = anm2_animation_from_reference(self, &reference);

	if (!animation) return;
	
	time = std::clamp(time, 0.0f, animation->frameNum - 1.0f);

	Anm2Item* item = anm2_item_from_reference(self, &reference);

	if (!item) return;

	Anm2Frame* frameNext = nullptr;
	s32 delayCurrent = 0;
	s32 delayNext = 0;

	for (auto [i, iFrame] : std::views::enumerate(item->frames))
	{
		if (reference.itemType == ANM2_TRIGGERS)
		{
			if ((s32)time == iFrame.atFrame)
			{
				*frame = iFrame;
				break;	
			}
		}
		else
		{
			*frame = iFrame;
			
			delayNext += frame->delay;

			if (time >= delayCurrent && time < delayNext)
			{
				if (i + 1 < (s32)item->frames.size())
					frameNext = &item->frames[i + 1];
				else
					frameNext = nullptr;
				break;
			}

			delayCurrent += frame->delay;
		}
	}

	if (reference.itemType == ANM2_TRIGGERS)
		return;

	if (frame->isInterpolated && frameNext && frame->delay > 1)
	{
		f32 interpolation = (time - delayCurrent) / (delayNext - delayCurrent);

		frame->rotation    = glm::mix(frame->rotation,    frameNext->rotation,    interpolation);
		frame->position    = glm::mix(frame->position,    frameNext->position,    interpolation);
		frame->scale       = glm::mix(frame->scale,       frameNext->scale,       interpolation);
		frame->offsetRGB   = glm::mix(frame->offsetRGB,   frameNext->offsetRGB,   interpolation);
		frame->tintRGBA    = glm::mix(frame->tintRGBA,    frameNext->tintRGBA,    interpolation);
	}
}

s32 anm2_animation_length_get(Anm2Animation* self)
{
	s32 length = 0;

	auto accumulate_max_delay = [&](const std::vector<Anm2Frame>& frames)
	{
		s32 delaySum = 0;
		for (const auto& frame : frames)
		{
			delaySum += frame.delay;
			length = std::max(length, delaySum);
		}
	};

	accumulate_max_delay(self->rootAnimation.frames);

	for (const auto& [_, item] : self->layerAnimations)
		accumulate_max_delay(item.frames);

	for (const auto& [_, item] : self->nullAnimations)
		accumulate_max_delay(item.frames);

	for (const auto& frame : self->triggers.frames)
		length = std::max(length, frame.atFrame + 1);

	return length;
}

void anm2_animation_length_set(Anm2Animation* self)
{
	self->frameNum = anm2_animation_length_get(self);
}

Anm2Frame* anm2_frame_add(Anm2* self, Anm2Frame* frame, Anm2Reference* reference, s32 time)
{
	Anm2Animation* animation = anm2_animation_from_reference(self, reference);
	Anm2Item* item = anm2_item_from_reference(self, reference);

	if (!animation || !item)
		return nullptr;

	Anm2Frame frameAdd = frame ? *frame : Anm2Frame{};
	s32 index = reference->frameIndex + 1;

	if (reference->itemType == ANM2_TRIGGERS)
	{
		s32 triggerIndex = time;

		for (auto& frameCheck : item->frames)
		{
			if (frameCheck.atFrame == time)
			{
				triggerIndex++;
				break;
			}
		}

		frameAdd.atFrame = triggerIndex;
		return &item->frames.emplace_back(frameAdd);
	}
	else
	{
		if (index >= static_cast<s32>(item->frames.size()))
		{
			item->frames.push_back(frameAdd);
			return &item->frames.back();
		}
		else
		{
			item->frames.insert(item->frames.begin() + index, frameAdd);
			return &item->frames[static_cast<size_t>(index)];
		}
	}
}

void anm2_frame_erase(Anm2* self, Anm2Reference* reference)
{
	Anm2Item* item = anm2_item_from_reference(self, reference);
	if (!item) return;
	item->frames.erase(item->frames.begin() + reference->frameIndex);
}

void anm2_reference_clear(Anm2Reference* self)
{
	*self = Anm2Reference{};
}

void anm2_reference_item_clear(Anm2Reference* self)
{
	*self = {self->animationID};
}

void anm2_reference_frame_clear(Anm2Reference* self)
{
	self->frameIndex = INDEX_NONE;
}

void anm2_item_frame_set(Anm2* self, Anm2Reference* reference, const Anm2FrameChange& change, Anm2ChangeType type, s32 start, s32 count)
{
    Anm2Item* item = anm2_item_from_reference(self, reference);
    if (!item) return;

    if (start < 0 || count <= 0) return;
    const s32 size = (s32)item->frames.size();
    if (size == 0 || start >= size) return;

    const s32 end = std::min(start + count, size);

    for (s32 i = start; i < end; ++i)
    {
        Anm2Frame& dest = item->frames[i];

        // Booleans always just set if provided
        if (change.isVisible)      dest.isVisible      = *change.isVisible;
        if (change.isInterpolated) dest.isInterpolated = *change.isInterpolated;

        switch (type)
        {
            case ANM2_CHANGE_SET:
                if (change.rotation)  dest.rotation  = *change.rotation;
                if (change.delay)     dest.delay     = std::max(ANM2_FRAME_DELAY_MIN, *change.delay);
                if (change.crop)      dest.crop      = *change.crop;
                if (change.pivot)     dest.pivot     = *change.pivot;
                if (change.position)  dest.position  = *change.position;
                if (change.size)      dest.size      = *change.size;
                if (change.scale)     dest.scale     = *change.scale;
                if (change.offsetRGB) dest.offsetRGB = glm::clamp(*change.offsetRGB, 0.0f, 1.0f);
                if (change.tintRGBA)  dest.tintRGBA  = glm::clamp(*change.tintRGBA, 0.0f, 1.0f);
                break;

            case ANM2_CHANGE_ADD:
                if (change.rotation)  dest.rotation  += *change.rotation;
                if (change.delay)     dest.delay      = std::max(ANM2_FRAME_DELAY_MIN, dest.delay + *change.delay);
                if (change.crop)      dest.crop      += *change.crop;
                if (change.pivot)     dest.pivot     += *change.pivot;
                if (change.position)  dest.position  += *change.position;
                if (change.size)      dest.size      += *change.size;
                if (change.scale)     dest.scale     += *change.scale;
                if (change.offsetRGB) dest.offsetRGB  = glm::clamp(dest.offsetRGB + *change.offsetRGB, 0.0f, 1.0f);
                if (change.tintRGBA)  dest.tintRGBA   = glm::clamp(dest.tintRGBA  + *change.tintRGBA,  0.0f, 1.0f);
                break;

            case ANM2_CHANGE_SUBTRACT:
                if (change.rotation)  dest.rotation  -= *change.rotation;
                if (change.delay)     dest.delay      = std::max(ANM2_FRAME_DELAY_MIN, dest.delay - *change.delay);
                if (change.crop)      dest.crop      -= *change.crop;
                if (change.pivot)     dest.pivot     -= *change.pivot;
                if (change.position)  dest.position  -= *change.position;
                if (change.size)      dest.size      -= *change.size;
                if (change.scale)     dest.scale     -= *change.scale;
                if (change.offsetRGB) dest.offsetRGB  = glm::clamp(dest.offsetRGB - *change.offsetRGB, 0.0f, 1.0f);
                if (change.tintRGBA)  dest.tintRGBA   = glm::clamp(dest.tintRGBA  - *change.tintRGBA,  0.0f, 1.0f);
                break;
        }
    }
}

void anm2_animation_merge(Anm2* self, s32 animationID, const std::vector<s32>& mergeIDs, Anm2MergeType type)
{
	Anm2Animation newAnimation = self->animations[animationID];

 	auto merge_item = [&](Anm2Item& destinationItem, const Anm2Item& sourceItem)
    {
        switch (type)
        {
            case ANM2_MERGE_APPEND_FRAMES:
                destinationItem.frames.insert(destinationItem.frames.end(), sourceItem.frames.begin(), sourceItem.frames.end());
                break;
            case ANM2_MERGE_PREPEND_FRAMES:
                destinationItem.frames.insert(destinationItem.frames.begin(), sourceItem.frames.begin(), sourceItem.frames.end());
                break;
            case ANM2_MERGE_REPLACE_FRAMES:
                if (destinationItem.frames.size() < sourceItem.frames.size())
                    destinationItem.frames.resize(sourceItem.frames.size());
                for (s32 i = 0; i < (s32)sourceItem.frames.size(); i++)
                    destinationItem.frames[i] = sourceItem.frames[i];
                break;
            case ANM2_MERGE_IGNORE:
                break;
        }
    };

    for (auto mergeID : mergeIDs)
    {
		if (animationID == mergeID) continue;

        const Anm2Animation& mergeAnimation = self->animations[mergeID];

        merge_item(newAnimation.rootAnimation, mergeAnimation.rootAnimation);

        for (const auto& [id, layerAnimation] : mergeAnimation.layerAnimations)
            merge_item(newAnimation.layerAnimations[id], layerAnimation);
        
		for (const auto& [id, nullAnimation] : mergeAnimation.nullAnimations)
            merge_item(newAnimation.nullAnimations[id], nullAnimation);
        
		merge_item(newAnimation.triggers, mergeAnimation.triggers);
    }
	
	self->animations[animationID] = newAnimation;

	anm2_animation_length_set(&self->animations[animationID]);
}

void anm2_frame_bake(Anm2* self, Anm2Reference* reference, s32 interval, bool isRoundScale, bool isRoundRotation)
{
	Anm2Item* item = anm2_item_from_reference(self, reference);
	if (!item) return;

	Anm2Frame* frame = anm2_frame_from_reference(self, reference);
	if (!frame) return;
	
	Anm2Reference referenceNext = *reference;
	referenceNext.frameIndex = reference->frameIndex + 1;

	Anm2Frame* frameNext = anm2_frame_from_reference(self, &referenceNext);
	if (!frameNext) frameNext = frame;

	const Anm2Frame baseFrame = *frame;
	const Anm2Frame baseFrameNext = *frameNext;
	
	s32 delay = 0;
	s32 insertIndex = reference->frameIndex;

	while (delay < baseFrame.delay)
	{
		f32 interpolation = (f32)delay / baseFrame.delay;
		
		Anm2Frame baked = *frame; 
		baked.delay = std::min(interval, baseFrame.delay - delay);
		baked.isInterpolated = (insertIndex == reference->frameIndex) ? baseFrame.isInterpolated : false;

		baked.rotation    = glm::mix(baseFrame.rotation,    baseFrameNext.rotation,    interpolation);
		baked.position    = glm::mix(baseFrame.position,    baseFrameNext.position,    interpolation);
		baked.scale       = glm::mix(baseFrame.scale,       baseFrameNext.scale,       interpolation);
		baked.offsetRGB   = glm::mix(baseFrame.offsetRGB,   baseFrameNext.offsetRGB,   interpolation);
		baked.tintRGBA    = glm::mix(baseFrame.tintRGBA,    baseFrameNext.tintRGBA,    interpolation);

		if (isRoundScale) baked.scale = vec2((s32)baked.scale.x, (s32)baked.scale.y);
		if (isRoundRotation) baked.rotation = (s32)baked.rotation;
		
		if (insertIndex == reference->frameIndex)
			item->frames[insertIndex] = baked;
		else
			item->frames.insert(item->frames.begin() + insertIndex, baked);
		insertIndex++;
		
		delay += baked.delay;
	}
}

void anm2_scale(Anm2* self, f32 scale)
{
 	auto frame_scale = [&](Anm2Frame& frame)
	{
		frame.position = vec2((s32)(frame.position.x * scale), (s32)(frame.position.y * scale));
		frame.size = vec2((s32)(frame.size.x * scale), (s32)(frame.size.y * scale));
		frame.crop = vec2((s32)(frame.crop.x * scale), (s32)(frame.crop.y * scale));
		frame.pivot = vec2((s32)(frame.pivot.x * scale), (s32)(frame.pivot.y * scale));
	};

	for (auto& [_, animation] : self->animations)
	{
		for (auto& frame : animation.rootAnimation.frames)
			frame_scale(frame);
	
		for (auto& [_, layerAnimation] : animation.layerAnimations)
				for (auto& frame : layerAnimation.frames)
					frame_scale(frame);

		for (auto& [_, nullAnimation] : animation.nullAnimations)
				for (auto& frame : nullAnimation.frames)
					frame_scale(frame);
	}
}

void anm2_generate_from_grid(Anm2* self, Anm2Reference* reference, vec2 startPosition, vec2 size, vec2 pivot, s32 columns, s32 count, s32 delay)
{
	Anm2Item* item = anm2_item_from_reference(self, reference);
	if (!item) return;

	Anm2Reference frameReference = *reference;

	for (s32 i = 0; i < count; i++)
	{
		const s32 row = i / columns;
		const s32 column = i % columns;

		Anm2Frame frame{};

		frame.delay = delay;
		frame.pivot = pivot;
		frame.size = size;
        frame.crop  = startPosition + vec2(size.x * column, size.y * row);
		
		anm2_frame_add(self, &frame, &frameReference);
		frameReference.frameIndex++;
	}
}

void anm2_free(Anm2* self)
{
	for (auto& [id, spritesheet] : self->spritesheets)
		texture_free(&spritesheet.texture);
}

void anm2_spritesheet_texture_pixels_upload(Anm2* self)
{
    for (auto& [_, spritesheet] : self->spritesheets)
    {
        Texture& texture = spritesheet.texture;

        if (texture.id != GL_ID_NONE && !texture.isInvalid)
        {
            assert(!spritesheet.pixels.empty());
            glBindTexture(GL_TEXTURE_2D, texture.id);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texture.size.x, texture.size.y, GL_RGBA, GL_UNSIGNED_BYTE, spritesheet.pixels.data());
        }
    }
}

void anm2_spritesheet_texture_pixels_download(Anm2* self)
{
    for (auto& [_, spritesheet] : self->spritesheets)
    {
        Texture& texture = spritesheet.texture;

        if (texture.id != GL_ID_NONE && !texture.isInvalid)
        {
            spritesheet.pixels.resize(texture.size.x * texture.size.y * texture.channels);
            glBindTexture(GL_TEXTURE_2D, texture.id);
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, spritesheet.pixels.data());
        }
    }
}