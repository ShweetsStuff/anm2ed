// Anm2 file format; serializing/deserializing

#include "anm2.h"

using namespace tinyxml2;

static void _anm2_created_on_set(Anm2* self)
{
	auto now = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::tm localTime = *std::localtime(&time);

	std::ostringstream timeString;
	timeString << std::put_time(&localTime, STRING_ANM2_CREATED_ON_FORMAT);
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

	for (auto & [id, spritesheet] : self->spritesheets)
	{
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

	for (auto & [id, layer] : self->layers)
	{
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

	for (auto & [id, null] : self->nulls)
	{
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

	for (auto & [id, event] : self->events)
	{
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
	animationsElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_DEFAULT_ANIMATION], self->defaultAnimation.c_str()); // DefaultAnimation

	for (auto& [id, animation] : self->animations)
	{
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
			frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_RED_TINT], COLOR_FLOAT_TO_INT(frame.tintRGBA.r)); // RedTint
			frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_GREEN_TINT], COLOR_FLOAT_TO_INT(frame.tintRGBA.g)); // GreenTint
			frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_BLUE_TINT], COLOR_FLOAT_TO_INT(frame.tintRGBA.b)); // BlueTint
			frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_ALPHA_TINT], COLOR_FLOAT_TO_INT(frame.tintRGBA.a)); // AlphaTint
			frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_RED_OFFSET], COLOR_FLOAT_TO_INT(frame.offsetRGB.r)); // RedOffset
			frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_GREEN_OFFSET], COLOR_FLOAT_TO_INT(frame.offsetRGB.g)); // GreenOffset
			frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_BLUE_OFFSET], COLOR_FLOAT_TO_INT(frame.offsetRGB.b)); // BlueOffset
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
	
			for (auto & frame : layerAnimation.frames)
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
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_RED_TINT], COLOR_FLOAT_TO_INT(frame.tintRGBA.r)); // RedTint
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_GREEN_TINT], COLOR_FLOAT_TO_INT(frame.tintRGBA.g)); // GreenTint
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_BLUE_TINT], COLOR_FLOAT_TO_INT(frame.tintRGBA.b)); // BlueTint
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_ALPHA_TINT], COLOR_FLOAT_TO_INT(frame.tintRGBA.a)); // AlphaTint
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_RED_OFFSET], COLOR_FLOAT_TO_INT(frame.offsetRGB.r)); // RedOffset
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_GREEN_OFFSET], COLOR_FLOAT_TO_INT(frame.offsetRGB.g)); // GreenOffset
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_BLUE_OFFSET], COLOR_FLOAT_TO_INT(frame.offsetRGB.b)); // BlueOffset
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_ROTATION], frame.rotation); // Rotation
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_INTERPOLATED], frame.isInterpolated); // Interpolated
			
				layerAnimationElement->InsertEndChild(frameElement);
			}

			layerAnimationsElement->InsertEndChild(layerAnimationElement);
		}

		animationElement->InsertEndChild(layerAnimationsElement);

		// NullAnimations
		nullAnimationsElement = document.NewElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_NULL_ANIMATIONS]);

		for (const auto & [nullID, nullAnimation] : animation.nullAnimations)
		{
			XMLElement* nullAnimationElement;

			// NullAnimation
			nullAnimationElement = document.NewElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_NULL_ANIMATION]);
			nullAnimationElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_NULL_ID], nullID); // NullId
			nullAnimationElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_VISIBLE], nullAnimation.isVisible); // Visible 
			
			for (const auto & frame : nullAnimation.frames)
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
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_RED_TINT], COLOR_FLOAT_TO_INT(frame.tintRGBA.r)); // RedTint 
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_GREEN_TINT], COLOR_FLOAT_TO_INT(frame.tintRGBA.g)); // GreenTint 
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_BLUE_TINT], COLOR_FLOAT_TO_INT(frame.tintRGBA.b)); // BlueTint 
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_ALPHA_TINT], COLOR_FLOAT_TO_INT(frame.tintRGBA.a)); // AlphaTint 
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_RED_OFFSET], COLOR_FLOAT_TO_INT(frame.offsetRGB.r)); // RedOffset 
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_GREEN_OFFSET], COLOR_FLOAT_TO_INT(frame.offsetRGB.g)); // GreenOffset 
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_BLUE_OFFSET], COLOR_FLOAT_TO_INT(frame.offsetRGB.b)); // BlueOffset 
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_ROTATION], frame.rotation); // Rotation 
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_INTERPOLATED], frame.isInterpolated); // Interpolated 
			
				nullAnimationElement->InsertEndChild(frameElement);
			}

			nullAnimationsElement->InsertEndChild(nullAnimationElement);
		}

		animationElement->InsertEndChild(nullAnimationsElement);

		// Triggers 
		triggersElement = document.NewElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_TRIGGERS]);

		for (const auto & frame : animation.triggers.frames)
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

bool anm2_deserialize(Anm2* self, Resources* resources, const std::string& path)
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

	if (!self || path.empty()) return false;

	*self = Anm2{};

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
					self->version = atoi(xmlAttribute->Value());
					break;
				case ANM2_ATTRIBUTE_FPS: // FPS
					self->fps = atoi(xmlAttribute->Value());
					break;
				case ANM2_ATTRIBUTE_ID: // ID
					id = atoi(xmlAttribute->Value());
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
					id = atoi(xmlAttribute->Value());
					
					if (!isLayerMapSet)
					{
						self->layerMap[layerMapIndex] = id;
						layerMapIndex++;
					}

					animation->layerAnimations[id] = addItem;
					item = &animation->layerAnimations[id];
					break;
				case ANM2_ATTRIBUTE_NULL_ID: // NullId
					id = atoi(xmlAttribute->Value());
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
					layer->spritesheetID = atoi(xmlAttribute->Value());
					break;
				case ANM2_ATTRIBUTE_SHOW_RECT:
					null->isShowRect = string_to_bool(xmlAttribute->Value());
					break;
				case ANM2_ATTRIBUTE_DEFAULT_ANIMATION:
					self->defaultAnimation = xmlAttribute->Value();
					break;
				case ANM2_ATTRIBUTE_FRAME_NUM:
					animation->frameNum = atoi(xmlAttribute->Value());
					break;
				case ANM2_ATTRIBUTE_LOOP:
					animation->isLoop = string_to_bool(xmlAttribute->Value());
					break;
				case ANM2_ATTRIBUTE_X_POSITION:
					frame->position.x = atof(xmlAttribute->Value());
					break;
				case ANM2_ATTRIBUTE_Y_POSITION:
					frame->position.y = atof(xmlAttribute->Value());
					break;
				case ANM2_ATTRIBUTE_X_PIVOT:
					frame->pivot.x = atof(xmlAttribute->Value());
					break;
				case ANM2_ATTRIBUTE_Y_PIVOT:
					frame->pivot.y = atof(xmlAttribute->Value());
					break;
				case ANM2_ATTRIBUTE_X_CROP:
					frame->crop.x = atof(xmlAttribute->Value());
					break;
				case ANM2_ATTRIBUTE_Y_CROP:
					frame->crop.y = atof(xmlAttribute->Value());
					break;	
				case ANM2_ATTRIBUTE_WIDTH:
					frame->size.x = atof(xmlAttribute->Value());
					break;		
				case ANM2_ATTRIBUTE_HEIGHT:
					frame->size.y = atof(xmlAttribute->Value());
					break;		
				case ANM2_ATTRIBUTE_X_SCALE:
					frame->scale.x = atof(xmlAttribute->Value());
					break;
				case ANM2_ATTRIBUTE_Y_SCALE:
					frame->scale.y = atof(xmlAttribute->Value());
					break;
				case ANM2_ATTRIBUTE_DELAY:
					frame->delay = atoi(xmlAttribute->Value());
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
					frame->tintRGBA.r = COLOR_INT_TO_FLOAT(atoi(xmlAttribute->Value()));
					break;
				case ANM2_ATTRIBUTE_GREEN_TINT:
					frame->tintRGBA.g = COLOR_INT_TO_FLOAT(atoi(xmlAttribute->Value()));
					break;
				case ANM2_ATTRIBUTE_BLUE_TINT:
					frame->tintRGBA.b = COLOR_INT_TO_FLOAT(atoi(xmlAttribute->Value()));
					break;
				case ANM2_ATTRIBUTE_ALPHA_TINT:
					frame->tintRGBA.a = COLOR_INT_TO_FLOAT(atoi(xmlAttribute->Value()));
					break;
				case ANM2_ATTRIBUTE_RED_OFFSET:
					frame->offsetRGB.r = COLOR_INT_TO_FLOAT(atoi(xmlAttribute->Value()));
					break;
				case ANM2_ATTRIBUTE_GREEN_OFFSET:
					frame->offsetRGB.g = COLOR_INT_TO_FLOAT(atoi(xmlAttribute->Value()));
					break;
				case ANM2_ATTRIBUTE_BLUE_OFFSET:
					frame->offsetRGB.b = COLOR_INT_TO_FLOAT(atoi(xmlAttribute->Value()));
					break;
				case ANM2_ATTRIBUTE_ROTATION:
					frame->rotation = atof(xmlAttribute->Value());
					break;
				case ANM2_ATTRIBUTE_INTERPOLATED:
					frame->isInterpolated = string_to_bool(xmlAttribute->Value());
					break;
				case ANM2_ATTRIBUTE_EVENT_ID:
					frame->eventID = atoi(xmlAttribute->Value());
					break;
				case ANM2_ATTRIBUTE_AT_FRAME:
					frame->atFrame = atoi(xmlAttribute->Value());
					break;
				default:
					break;
			}

			xmlAttribute = xmlAttribute->Next();
		}

		if (anm2Element == ANM2_ELEMENT_SPRITESHEET) resources_texture_init(resources, spritesheet->path , id);

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

	log_info(std::format(ANM2_READ_INFO, path));
	
	// Return to old working directory
	std::filesystem::current_path(workingPath);

	return true;
}

void anm2_layer_add(Anm2* self)
{
	s32 id = map_next_id_get(self->layers);

	self->layers[id] = Anm2Layer{};

	for (auto & [animationID, animation] : self->animations)
		animation.layerAnimations[id] = Anm2Item{};
}

void anm2_layer_remove(Anm2* self, s32 id)
{
	// Make sure the layer exists
	auto it = self->layers.find(id);
	if (it == self->layers.end())
		return;

    self->layers.erase(id);

    for (auto & [animationID, animation] : self->animations)
        animation.layerAnimations.erase(id);
}

void anm2_null_add(Anm2* self)
{
	s32 id = map_next_id_get(self->nulls);

	self->nulls[id] = Anm2Null{};

	for (auto & [animationID, animation] : self->animations)
		animation.nullAnimations[id] = Anm2Item{};
}

void anm2_null_remove(Anm2* self, s32 id)
{
	// Make sure the null exists
	auto it = self->nulls.find(id);
	if (it == self->nulls.end())
		return;

    self->nulls.erase(id);

    for (auto & [animationID, animation] : self->animations)
        animation.nullAnimations.erase(id);
}

s32 anm2_animation_add(Anm2* self)
{
    s32 id  = map_next_id_get(self->animations);
    Anm2Animation animation;

    for (auto& [layerID, layer] : self->layers) 
		animation.layerAnimations[layerID] = Anm2Item{};
    for (auto & [nullID, null] : self->nulls) 
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
	auto it = self->animations.find(reference->animationID);
	if (it == self->animations.end()) return nullptr;
	return &it->second;
}

Anm2Item* anm2_item_from_reference(Anm2* self, Anm2Reference* reference)
{
	Anm2Animation* animation = anm2_animation_from_reference(self, reference);
	
	if (!animation) return nullptr;

	switch (reference->itemType)
	{
		case ANM2_ROOT:
			return &animation->rootAnimation;
		case ANM2_LAYER:
		{
			auto it = animation->layerAnimations.find(reference->itemID);
			if (it == animation->layerAnimations.end())
				return nullptr;
			return &it->second;
		}
		case ANM2_NULL:
		{
			auto it = animation->nullAnimations.find(reference->itemID);
			if (it == animation->nullAnimations.end())
				return nullptr;
			return &it->second;
		}
		case ANM2_TRIGGERS:
			return &animation->triggers;
		default:
			return nullptr;
	}
}

Anm2Frame* anm2_frame_from_reference(Anm2* self, Anm2Reference* reference)
{
	Anm2Item* item = anm2_item_from_reference(self, reference);

	if (!item) return nullptr;
	if (reference->frameIndex < 0 || reference->frameIndex >= (s32)item->frames.size()) return nullptr;

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
	if (time < 0 || time > animation->frameNum) return;

	Anm2Item* item = anm2_item_from_reference(self, &reference);

	if (!item) return;

	Anm2Frame* nextFrame = nullptr;
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
					nextFrame = &item->frames[i + 1];
				else
					nextFrame = nullptr;
				break;
			}

			delayCurrent += frame->delay;
		}
	}

	if (reference.itemType == ANM2_TRIGGERS)
		return;

	if (frame->isInterpolated && nextFrame)
	{
		f32 interpolationTime = (time - delayCurrent) / (delayNext - delayCurrent);

		frame->rotation    = glm::mix(frame->rotation,    nextFrame->rotation,    interpolationTime);;
		frame->position    = glm::mix(frame->position,    nextFrame->position,    interpolationTime);;
		frame->scale       = glm::mix(frame->scale,       nextFrame->scale,       interpolationTime);;
		frame->offsetRGB   = glm::mix(frame->offsetRGB,   nextFrame->offsetRGB,   interpolationTime);;
		frame->tintRGBA    = glm::mix(frame->tintRGBA,    nextFrame->tintRGBA,    interpolationTime);;
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
		length = std::max(length, frame.atFrame);

	return length;
}

void anm2_animation_length_set(Anm2Animation* self)
{
	self->frameNum = anm2_animation_length_get(self);
}

Anm2Frame* anm2_frame_add(Anm2* self, Anm2Reference* reference, s32 time)
{
	Anm2Animation* animation = anm2_animation_from_reference(self, reference);
	Anm2Item* item = anm2_item_from_reference(self, reference);
	
	if (!animation || !item) return nullptr;

	if (item)
	{
		Anm2Frame frame = Anm2Frame{};
		s32 index = INDEX_NONE;

		if (reference->itemType == ANM2_TRIGGERS)
		{
			for (auto & frameCheck : item->frames) if (frameCheck.atFrame == time) return nullptr;

			frame.atFrame = time;
			index = item->frames.size();
		}
		else
		{
			s32 frameDelayCount = 0;

			for (auto & frameCheck : item->frames)
				frameDelayCount += frameCheck.delay;
			
			if (frameDelayCount + ANM2_FRAME_DELAY_MIN > animation->frameNum) return nullptr;

			Anm2Frame* checkFrame = anm2_frame_from_reference(self, reference);

			if (checkFrame)
			{
				if (frameDelayCount + checkFrame->delay > animation->frameNum)
					frame.delay = animation->frameNum - frameDelayCount;

				index = reference->frameIndex + 1;
			}
			else
				index = (s32)item->frames.size();
		}

		item->frames.insert(item->frames.begin() + index, frame);

		return &item->frames[index];
	}

	return nullptr;
}

void anm2_reference_clear(Anm2Reference* self)
{
	*self = Anm2Reference{};
}

void anm2_reference_item_clear(Anm2Reference* self)
{
	self->itemType = ANM2_NONE;
	self->itemID = ID_NONE;
	self->frameIndex = INDEX_NONE;
}

void anm2_reference_frame_clear(Anm2Reference* self)
{
	self->frameIndex = INDEX_NONE;
}