#include "anm2.h"

using namespace tinyxml2;

/* Sets the anm2's date to the system's current date */
void
anm2_created_on_set(Anm2* self)
{
	time_t currentTime; 
    struct tm* local; 
	char date[ANM2_STRING_MAX];

	currentTime = time(NULL);
	local = localtime(&currentTime);

    strftime(date, ANM2_STRING_MAX, "%d-%B-%Y %I:%M:%S %p", local);
	
	strncpy(self->createdOn, date, ANM2_STRING_MAX);
}

/* Serializes the anm2 struct into XML and exports it to the given path */
bool
anm2_serialize(Anm2* self, const char* path)
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

	if (!self || !path)
		return false;


	/* Update creation date on first version */
	if (self->version == 0)
		anm2_created_on_set(self);

	/* Increment anm2's version */
	self->version++;

	/* Set the anm2's date to the system time */

	/* AnimatedActor */
	animatedActorElement = document.NewElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_ANIMATED_ACTOR]);
	document.InsertFirstChild(animatedActorElement);

	/* Info */
	infoElement = document.NewElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_INFO]);
	infoElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_CREATED_BY], self->createdBy); /* CreatedBy */
	infoElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_CREATED_ON], self->createdOn); /* CreatedOn */
	infoElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_VERSION], self->version); /* Version; note its incrementation */
	infoElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_FPS], self->fps); /* FPS */
	animatedActorElement->InsertEndChild(infoElement);

	/* Content */
	contentElement = document.NewElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_CONTENT]);
	
	/* Spritesheets */
	spritesheetsElement = document.NewElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_SPRITESHEETS]);

	for (auto & [id, spritesheet] : self->spritesheets)
	{
		XMLElement* spritesheetElement;
		
		/* Spritesheet */
		spritesheetElement = document.NewElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_SPRITESHEET]);
		spritesheetElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_PATH], spritesheet.path); /* Path */
		spritesheetElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_ID], id); /* ID */
		spritesheetsElement->InsertEndChild(spritesheetElement);
	}
	
	contentElement->InsertEndChild(spritesheetsElement);
	
	/* Layers */
	layersElement = document.NewElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_LAYERS]);

	for (auto & [id, layer] : self->layers)
	{
		XMLElement* layerElement;
		
		/* Layer */
		layerElement = document.NewElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_LAYER]);
		layerElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_NAME], layer.name); /* Path */
		layerElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_ID], id); /* ID */
		layerElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_SPRITESHEET_ID], layer.spritesheetID); /* SpritesheetId */
		
		layersElement->InsertEndChild(layerElement);
	}
	
	contentElement->InsertEndChild(layersElement);
	
	/* Nulls */
	nullsElement = document.NewElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_NULLS]);

	for (auto & [id, null] : self->nulls)
	{
		XMLElement* nullElement;
		
		/* Null */
		nullElement = document.NewElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_NULL]);
		nullElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_NAME], null.name); /* Name */
		nullElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_ID], id); /* ID */

		/* special case; only serialize if this is true */
		if (null.isShowRect)
			nullElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_SHOW_RECT], null.isShowRect); /* ShowRect */
		nullsElement->InsertEndChild(nullElement);
	}
	
	contentElement->InsertEndChild(nullsElement);

	/* Events */
	eventsElement = document.NewElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_EVENTS]);

	for (auto & [id, event] : self->events)
	{
		XMLElement* eventElement;
		
		/* Event */
		eventElement = document.NewElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_EVENT]);
		eventElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_NAME], event.name); /* Name */
		eventElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_ID], id); /* ID */
		eventsElement->InsertEndChild(eventElement);
	}
	
	contentElement->InsertEndChild(eventsElement);

	animatedActorElement->InsertEndChild(contentElement);

	/* Animations */
	animationsElement = document.NewElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_ANIMATIONS]);
	animationsElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_DEFAULT_ANIMATION], self->defaultAnimation);

	for (auto & [id, animation] : self->animations)
	{
		XMLElement* animationElement;
		XMLElement* rootAnimationElement;
		XMLElement* layerAnimationsElement;
		XMLElement* nullAnimationsElement;
		XMLElement* triggersElement;
		
		/* Animation */
		animationElement = document.NewElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_ANIMATION]);

		animationElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_NAME], animation.name); /* Name */
		animationElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_FRAME_NUM], animation.frameNum); /* FrameNum */
		animationElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_LOOP], animation.isLoop); /* Loop */

		/* RootAnimation */
		rootAnimationElement = document.NewElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_ROOT_ANIMATION]);

		for (auto & frame : animation.rootAnimation.frames)
		{
			XMLElement* frameElement;

			/* Frame */
			frameElement = document.NewElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_FRAME]);
			
			frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_X_POSITION], frame.position.x); /* XPosition */
			frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_Y_POSITION], frame.position.y); /* YPosition */
			frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_X_PIVOT], frame.pivot.x); /* XPivot */
			frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_Y_PIVOT], frame.pivot.y); /* YPivot */
			frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_X_SCALE], frame.scale.x); /* XScale */
			frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_Y_SCALE], frame.scale.y); /* XScale */
			frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_DELAY], frame.delay); /* Delay */
			frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_VISIBLE], frame.isVisible); /* Visible */
			frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_RED_TINT], COLOR_FLOAT_TO_INT(frame.tintRGBA.r)); /* RedTint */
			frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_GREEN_TINT], COLOR_FLOAT_TO_INT(frame.tintRGBA.g)); /* GreenTint */
			frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_BLUE_TINT], COLOR_FLOAT_TO_INT(frame.tintRGBA.b)); /* BlueTint */
			frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_ALPHA_TINT], COLOR_FLOAT_TO_INT(frame.tintRGBA.a)); /* AlphaTint */
			frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_RED_OFFSET], COLOR_FLOAT_TO_INT(frame.offsetRGB.r)); /* RedOffset */
			frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_GREEN_OFFSET], COLOR_FLOAT_TO_INT(frame.offsetRGB.g)); /* GreenOffset */
			frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_BLUE_OFFSET], COLOR_FLOAT_TO_INT(frame.offsetRGB.b)); /* BlueOffset */
			frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_ROTATION], frame.rotation); /* Rotation */
			frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_INTERPOLATED], frame.isInterpolated); /* Interpolated */
		
			rootAnimationElement->InsertEndChild(frameElement);
		}

		animationElement->InsertEndChild(rootAnimationElement);

		/* LayerAnimations */
		layerAnimationsElement = document.NewElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_LAYER_ANIMATIONS]);

		for (const auto & [layerID, layerAnimation] : animation.layerAnimations)
		{
			XMLElement* layerAnimationElement;

			/* LayerAnimation */
			layerAnimationElement = document.NewElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_LAYER_ANIMATION]);
			layerAnimationElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_LAYER_ID], layerID); /* LayerID */
			layerAnimationElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_VISIBLE], layerAnimation.isVisible); /* Visible */ 
	
			for (auto & frame : layerAnimation.frames)
			{
				XMLElement* frameElement;

				/* Frame */
				frameElement = document.NewElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_FRAME]);
				
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_X_POSITION], frame.position.x); /* XPosition */
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_Y_POSITION], frame.position.y); /* YPosition */
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_X_PIVOT], frame.pivot.x); /* XPivot */
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_Y_PIVOT], frame.pivot.y); /* YPivot */
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_X_CROP], frame.crop.x); /* XCrop */
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_Y_CROP], frame.crop.y); /* YCrop */
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_WIDTH], frame.size.x); /* Width */
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_HEIGHT], frame.size.y); /* Height */
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_X_SCALE], frame.scale.x); /* XScale */
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_Y_SCALE], frame.scale.y); /* XScale */
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_DELAY], frame.delay); /* Delay */
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_VISIBLE], frame.isVisible); /* Visible */
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_RED_TINT], COLOR_FLOAT_TO_INT(frame.tintRGBA.r)); /* RedTint */
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_GREEN_TINT], COLOR_FLOAT_TO_INT(frame.tintRGBA.g)); /* GreenTint */
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_BLUE_TINT], COLOR_FLOAT_TO_INT(frame.tintRGBA.b)); /* BlueTint */
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_ALPHA_TINT], COLOR_FLOAT_TO_INT(frame.tintRGBA.a)); /* AlphaTint */
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_RED_OFFSET], COLOR_FLOAT_TO_INT(frame.offsetRGB.r)); /* RedOffset */
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_GREEN_OFFSET], COLOR_FLOAT_TO_INT(frame.offsetRGB.g)); /* GreenOffset */
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_BLUE_OFFSET], COLOR_FLOAT_TO_INT(frame.offsetRGB.b)); /* BlueOffset */
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_ROTATION], frame.rotation); /* Rotation */
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_INTERPOLATED], frame.isInterpolated); /* Interpolated */
			
				layerAnimationElement->InsertEndChild(frameElement);
			}

			layerAnimationsElement->InsertEndChild(layerAnimationElement);
		}

		animationElement->InsertEndChild(layerAnimationsElement);

		/* NullAnimations */
		nullAnimationsElement = document.NewElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_NULL_ANIMATIONS]);

		for (const auto & [nullID, nullAnimation] : animation.nullAnimations)
		{
			XMLElement* nullAnimationElement;

			/* NullAnimation */
			nullAnimationElement = document.NewElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_NULL_ANIMATION]);
			nullAnimationElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_NULL_ID], nullID); /* NullID */
			nullAnimationElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_VISIBLE], nullAnimation.isVisible); /* Visible */ 
			
			for (auto & frame : nullAnimation.frames)
			{
				XMLElement* frameElement;

				/* Frame */
				frameElement = document.NewElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_FRAME]);
				
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_X_POSITION], frame.position.x); /* XPosition */
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_Y_POSITION], frame.position.y); /* YPosition */
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_X_PIVOT], frame.pivot.x); /* XPivot */
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_Y_PIVOT], frame.pivot.y); /* YPivot */
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_X_SCALE], frame.scale.x); /* XScale */
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_Y_SCALE], frame.scale.y); /* XScale */
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_DELAY], frame.delay); /* Delay */
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_VISIBLE], frame.isVisible); /* Visible */
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_RED_TINT], COLOR_FLOAT_TO_INT(frame.tintRGBA.r)); /* RedTint */
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_GREEN_TINT], COLOR_FLOAT_TO_INT(frame.tintRGBA.g)); /* GreenTint */
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_BLUE_TINT], COLOR_FLOAT_TO_INT(frame.tintRGBA.b)); /* BlueTint */
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_ALPHA_TINT], COLOR_FLOAT_TO_INT(frame.tintRGBA.a)); /* AlphaTint */
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_RED_OFFSET], COLOR_FLOAT_TO_INT(frame.offsetRGB.r)); /* RedOffset */
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_GREEN_OFFSET], COLOR_FLOAT_TO_INT(frame.offsetRGB.g)); /* GreenOffset */
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_BLUE_OFFSET], COLOR_FLOAT_TO_INT(frame.offsetRGB.b)); /* BlueOffset */
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_ROTATION], frame.rotation); /* Rotation */
				frameElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_INTERPOLATED], frame.isInterpolated); /* Interpolated */
			
				nullAnimationElement->InsertEndChild(frameElement);
			}

			nullAnimationsElement->InsertEndChild(nullAnimationElement);
		}

		animationElement->InsertEndChild(nullAnimationsElement);

		/* Triggers */
		triggersElement = document.NewElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_TRIGGERS]);

		for (auto & trigger : animation.triggers.items)
		{
			XMLElement* triggerElement;

			/* Trigger */
			triggerElement = document.NewElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_TRIGGER]);
			triggerElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_EVENT_ID], trigger.eventID); /* EventID */
			triggerElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_AT_FRAME], trigger.atFrame); /* AtFrame */
			triggersElement->InsertEndChild(triggerElement);
		}

		animationElement->InsertEndChild(triggersElement);
		
		animationsElement->InsertEndChild(animationElement);
	}

	animatedActorElement->InsertEndChild(animationsElement);

	error = document.SaveFile(path);

	if (error != XML_SUCCESS)
	{
		printf(STRING_ERROR_ANM2_WRITE, path, document.ErrorStr());
		return false;
	}

	working_directory_from_path_set(path);

	printf(STRING_INFO_ANM2_WRITE, path);
	strncpy(self->path, path, PATH_MAX - 1);
	
	return true;
}

/* Loads the .anm2 file and deserializes it into the struct equivalent */
bool 
anm2_deserialize(Anm2* self, Resources* resources, const char* path)
{
	XMLDocument document;
	XMLError error;
	const XMLElement* element;
	const XMLElement* root;
	Anm2Spritesheet* lastSpritesheet = NULL;
	Anm2Layer* lastLayer = NULL;
	Anm2Null* lastNull = NULL;
	Anm2Event* lastEvent = NULL;
	Anm2Animation* lastAnimation = NULL;
	Anm2LayerAnimation* lastLayerAnimation = NULL;
	Anm2NullAnimation* lastNullAnimation = NULL;
	Anm2Frame* lastFrame = NULL;
	Anm2Trigger* lastTrigger = NULL;
	Anm2Element anm2Element = ANM2_ELEMENT_ANIMATED_ACTOR;
	Anm2Attribute anm2Attribute =  ANM2_ATTRIBUTE_ID;
	Anm2AnimationType animationType = ANM2_ROOT_ANIMATION;
	Anm2Null tempNull;
	Anm2Layer tempLayer;
	Anm2Spritesheet tempSpritesheet;
	Anm2Event tempEvent;
	char lastSpritesheetPath[PATH_MAX];

	*self = Anm2{};

	error = document.LoadFile(path);

	if (error != XML_SUCCESS)
	{
		printf(STRING_ERROR_ANM2_READ, path, document.ErrorStr());
		return false;
	}

	resources_textures_free(resources);
	strncpy(self->path, path, PATH_MAX - 1);
	working_directory_from_path_set(path);
	
    root = document.FirstChildElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_ANIMATED_ACTOR]);
	element = root;

	while (element)
	{
		const XMLAttribute* attribute;
		const XMLElement* child;
		s32 id;

		/* Elements */
		anm2Element = anm2_element_from_string(element->Name());
			
		switch (anm2Element)
		{
			case ANM2_ELEMENT_SPRITESHEET:
				lastSpritesheet = &tempSpritesheet;
				break;
			case ANM2_ELEMENT_LAYER:
				lastLayer = &tempLayer;
				break;
			case ANM2_ELEMENT_NULL:
				lastNull = &tempNull;
				break;
			case ANM2_ELEMENT_EVENT:
				lastEvent = &tempEvent;
				break;
			case ANM2_ELEMENT_ANIMATION:
				id = map_next_id_get(self->animations);
				self->animations[id] = Anm2Animation{};
				lastAnimation = &self->animations[id];
				break;
			case ANM2_ELEMENT_ROOT_ANIMATION:
				animationType = ANM2_ROOT_ANIMATION;
				break;
			case ANM2_ELEMENT_LAYER_ANIMATION:
				animationType = ANM2_LAYER_ANIMATION;
				lastLayerAnimation = NULL;
				break;
			case ANM2_ELEMENT_NULL_ANIMATION:
				animationType = ANM2_NULL_ANIMATION;
				lastNullAnimation = NULL;
				break;
			case ANM2_ELEMENT_FRAME:
				switch (animationType)
				{
					case ANM2_ROOT_ANIMATION:
						lastAnimation->rootAnimation.frames.push_back(Anm2Frame{});
						lastFrame = &lastAnimation->rootAnimation.frames.back();
						break;
					case ANM2_LAYER_ANIMATION:
						if (!lastLayerAnimation) break;
						lastLayerAnimation->frames.push_back(Anm2Frame{});
						lastFrame = &lastLayerAnimation->frames.back();
					break;
					case ANM2_NULL_ANIMATION:
						if (!lastNullAnimation) break;
						lastNullAnimation->frames.push_back(Anm2Frame{});
						lastFrame = &lastNullAnimation->frames.back();
					break;
					default:
						break;
				}
				break;
			case ANM2_ELEMENT_TRIGGER:
				lastAnimation->triggers.items.push_back(Anm2Trigger{});
				lastTrigger = &lastAnimation->triggers.items.back();
				break;
			default:
				break;
		}

		/* Attributes */
		attribute = element->FirstAttribute();

		while (attribute)
		{
			anm2Attribute = anm2_attribute_from_string(attribute->Name());
			
			switch (anm2Attribute)
			{
				case ANM2_ATTRIBUTE_CREATED_BY:
					strncpy(self->createdBy, attribute->Value(), ANM2_STRING_MAX - 1);
					break;
				case ANM2_ATTRIBUTE_CREATED_ON:
					strncpy(self->createdOn, attribute->Value(), ANM2_STRING_MAX - 1);
					break;
				case ANM2_ATTRIBUTE_VERSION:
					self->version = atoi(attribute->Value());
					break;
				case ANM2_ATTRIBUTE_FPS:
					self->fps = atoi(attribute->Value());
					break;
				case ANM2_ATTRIBUTE_ID:
					id = atoi(attribute->Value());
					switch (anm2Element)
					{
						case ANM2_ELEMENT_SPRITESHEET:
							self->spritesheets[id] = tempSpritesheet;
							lastSpritesheet = &self->spritesheets[id];
							break;
						case ANM2_ELEMENT_LAYER:
							self->layers[id] = tempLayer;
							lastLayer = &self->layers[id];
							break;
						case ANM2_ELEMENT_NULL:
							self->nulls[id] = tempNull;
							lastNull = &self->nulls[id];
							break;
						case ANM2_ELEMENT_EVENT:
							self->events[id] = tempEvent;
							lastEvent = &self->events[id];
							break;
						default:
							break;
					}
					break;
				case ANM2_ATTRIBUTE_PATH:
					/* Make path lowercase */
					strncpy(lastSpritesheetPath, attribute->Value(), PATH_MAX - 1);
					break;
				case ANM2_ATTRIBUTE_NAME:
					switch (anm2Element)
					{
						case ANM2_ELEMENT_LAYER:
							strncpy(lastLayer->name, attribute->Value(), ANM2_STRING_MAX - 1);
							break;
						case ANM2_ELEMENT_NULL:
							strncpy(lastNull->name, attribute->Value(), ANM2_STRING_MAX - 1);
							break;
						case ANM2_ELEMENT_ANIMATION:
							strncpy(lastAnimation->name, attribute->Value(), ANM2_STRING_MAX - 1);
							break;
						case ANM2_ELEMENT_EVENT:
							strncpy(lastEvent->name, attribute->Value(), ANM2_STRING_MAX - 1);
							break;
						default:
							break;
					}
					break;
				case ANM2_ATTRIBUTE_SPRITESHEET_ID:
					lastLayer->spritesheetID = atoi(attribute->Value());
					break;
				case ANM2_ATTRIBUTE_SHOW_RECT:
					switch (anm2Element)
					{
						case ANM2_ELEMENT_NULL:
							lastNull->isShowRect = string_to_bool(attribute->Value());
							break;
						default:
							break;
					}
					break;
				case ANM2_ATTRIBUTE_DEFAULT_ANIMATION:
					strncpy(self->defaultAnimation, attribute->Value(), ANM2_STRING_MAX - 1);
					break;
				case ANM2_ATTRIBUTE_FRAME_NUM:
					lastAnimation->frameNum = atoi(attribute->Value());
					break;
				case ANM2_ATTRIBUTE_LOOP:
					lastAnimation->isLoop = string_to_bool(attribute->Value());
					break;
				case ANM2_ATTRIBUTE_X_POSITION:
					lastFrame->position.x = atof(attribute->Value());
					break;
				case ANM2_ATTRIBUTE_Y_POSITION:
					lastFrame->position.y = atof(attribute->Value());
					break;
				case ANM2_ATTRIBUTE_X_PIVOT:
					lastFrame->pivot.x = atof(attribute->Value());
					break;
				case ANM2_ATTRIBUTE_Y_PIVOT:
					lastFrame->pivot.y = atof(attribute->Value());
					break;
				case ANM2_ATTRIBUTE_X_CROP:
					lastFrame->crop.x = atof(attribute->Value());
					break;
				case ANM2_ATTRIBUTE_Y_CROP:
					lastFrame->crop.y = atof(attribute->Value());
					break;	
				case ANM2_ATTRIBUTE_WIDTH:
					lastFrame->size.x = atof(attribute->Value());
					break;		
				case ANM2_ATTRIBUTE_HEIGHT:
					lastFrame->size.y = atof(attribute->Value());
					break;		
				case ANM2_ATTRIBUTE_X_SCALE:
					lastFrame->scale.x = atof(attribute->Value());
					break;
				case ANM2_ATTRIBUTE_Y_SCALE:
					lastFrame->scale.y = atof(attribute->Value());
					break;
				case ANM2_ATTRIBUTE_DELAY:
					lastFrame->delay = atoi(attribute->Value());
					break;
				case ANM2_ATTRIBUTE_VISIBLE:
					switch (anm2Element)
					{
						case ANM2_ELEMENT_FRAME:
							lastFrame->isVisible = string_to_bool(attribute->Value());
							break;
						case ANM2_LAYER_ANIMATION:
							lastLayerAnimation->isVisible = string_to_bool(attribute->Value());
							break;
						case ANM2_NULL_ANIMATION:
							lastNullAnimation->isVisible = string_to_bool(attribute->Value());
							break;
						default:
							break;
					}
					break;
				case ANM2_ATTRIBUTE_RED_TINT:
					lastFrame->tintRGBA.r = COLOR_INT_TO_FLOAT(atoi(attribute->Value()));
					break;
				case ANM2_ATTRIBUTE_GREEN_TINT:
					lastFrame->tintRGBA.g = COLOR_INT_TO_FLOAT(atoi(attribute->Value()));
					break;
				case ANM2_ATTRIBUTE_BLUE_TINT:
					lastFrame->tintRGBA.b = COLOR_INT_TO_FLOAT(atoi(attribute->Value()));
					break;
				case ANM2_ATTRIBUTE_ALPHA_TINT:
					lastFrame->tintRGBA.a = COLOR_INT_TO_FLOAT(atoi(attribute->Value()));
					break;
				case ANM2_ATTRIBUTE_RED_OFFSET:
					lastFrame->offsetRGB.r = COLOR_INT_TO_FLOAT(atoi(attribute->Value()));
					break;
				case ANM2_ATTRIBUTE_GREEN_OFFSET:
					lastFrame->offsetRGB.g = COLOR_INT_TO_FLOAT(atoi(attribute->Value()));
					break;
				case ANM2_ATTRIBUTE_BLUE_OFFSET:
					lastFrame->offsetRGB.b = COLOR_INT_TO_FLOAT(atoi(attribute->Value()));
					break;
				case ANM2_ATTRIBUTE_ROTATION:
					lastFrame->rotation = atof(attribute->Value());
					break;
				case ANM2_ATTRIBUTE_INTERPOLATED:
					lastFrame->isInterpolated = string_to_bool(attribute->Value());
					break;
				case ANM2_ATTRIBUTE_LAYER_ID:
					id = atoi(attribute->Value());
					lastAnimation->layerAnimations[id] = Anm2LayerAnimation{};
					lastLayerAnimation = &lastAnimation->layerAnimations[id];
					break;
				case ANM2_ATTRIBUTE_NULL_ID:
					id = atoi(attribute->Value());
					lastAnimation->nullAnimations[id] = Anm2NullAnimation{};
					lastNullAnimation = &lastAnimation->nullAnimations[id];
					break;
				case ANM2_ATTRIBUTE_EVENT_ID:
					lastTrigger->eventID = atoi(attribute->Value());
					break;
				case ANM2_ATTRIBUTE_AT_FRAME:
					lastTrigger->atFrame = atoi(attribute->Value());
					break;
				default:
					break;
			}

			attribute = attribute->Next();
		}

		/* Load spritesheet textures */
		if (anm2Element == ANM2_ELEMENT_SPRITESHEET)
		{
			strncpy(lastSpritesheet->path, lastSpritesheetPath, PATH_MAX);
			anm2_spritesheet_texture_load(self, resources, lastSpritesheetPath , id);
		}

		/* Iterate through children */
		child = element->FirstChildElement();

		if (child)
		{
			element = child;
			continue;
		}

		/* Iterate through siblings */
		while (element)
		{
			const XMLElement* next;
			
			next = element->NextSiblingElement();
			
			if (next)
			{
				element = next;
				break;
			}

			/* If no siblings, return to parent. If no parent, end parsing */
			element = element->Parent() ? element->Parent()->ToElement() : NULL;
		}
	}

	printf(STRING_INFO_ANM2_READ, path);

	return true;
}

/* Adds a new layer to the anm2 */
void
anm2_layer_add(Anm2* self)
{
	s32 id = map_next_id_get(self->layers);

	self->layers[id] = Anm2Layer{};

	for (auto & [animationID, animation] : self->animations)
		animation.layerAnimations[id] = Anm2LayerAnimation{};
}

/* Removes a layer from the anm2 given the index/id */
void
anm2_layer_remove(Anm2* self, s32 id)
{
    self->layers.erase(id);

    for (auto& animationPair : self->animations)
        animationPair.second.layerAnimations.erase(id);
}

/* Adds a new null to the anm2 */
void
anm2_null_add(Anm2* self)
{
	s32 id = map_next_id_get(self->nulls);

	self->nulls[id] = Anm2Null{};

	for (auto & [animationID, animation] : self->animations)
		animation.nullAnimations[id] = Anm2NullAnimation{};
}

/* Removes a null from the anm2 given the index/id */
void
anm2_null_remove(Anm2* self, s32 id)
{
    self->nulls.erase(id);

    for (auto& animationPair : self->animations)
        animationPair.second.nullAnimations.erase(id);
}

/* Adds a new animation to the anm2, makes sure to keep the layeranimations/nullsanimation check */
s32
anm2_animation_add(Anm2* self)
{
    s32 id  = map_next_id_get(self->animations);
    Anm2Animation animation;

	/* match layers */
    for (auto & [layerID, layer] : self->layers)
	{
        animation.layerAnimations[layerID] = Anm2LayerAnimation{};
	}

	/* match nulls */
    for (auto & [nullID, null] : self->nulls)
	{
    	animation.nullAnimations[nullID] = Anm2NullAnimation{};
	}

	/* add a root frame */
	animation.rootAnimation.frames.push_back(Anm2Frame{});
		
    self->animations[id] = animation;

	return id;
}

void
anm2_animation_remove(Anm2* self, s32 id)
{
	self->animations.erase(id);
}

/* Makes an entirely new anm2 */
void
anm2_new(Anm2* self)
{
	*self = Anm2{};
	anm2_created_on_set(self);
}

void
anm2_spritesheet_texture_load(Anm2* self, Resources* resources, const char* path, s32 id)
{
	Texture texture;

	if (resources->textures.find(id) != resources->textures.end() && resources->textures[id].id != resources->textures[TEXTURE_ERROR].id)
		texture_free(&resources->textures[id]);

	if (texture_from_path_init(&texture, path))
		resources->textures[id] = texture;
	else
		texture.isInvalid = true;

	resources->textures[id] = texture;
}

/* Creates/fetches a frame from a given time. */
/* Returns true/false if frame will be valid or not. */
bool
anm2_frame_from_time(Anm2* self, Anm2Animation* animation, Anm2Frame* frame, Anm2AnimationType type, s32 id, f32 time)
{
	/* Out of range */
	if (time < 0 || time > animation->frameNum)
		return false;

	Anm2RootAnimation* rootAnimation;
	Anm2LayerAnimation* layerAnimation;
	Anm2NullAnimation* nullAnimation;
	Anm2Frame* nextFrame = NULL;
	std::vector<Anm2Frame>* frames = NULL;
	f32 delayCurrent = 0;
	f32 delayNext = 0;
	bool isTimeMatchedFrame = false;

	switch (type)
	{
		case ANM2_ROOT_ANIMATION:
			frames = &animation->rootAnimation.frames;
			break;
		case ANM2_LAYER_ANIMATION:
			if (id < 0 || id >= (s32)animation->layerAnimations.size())
				return false;
			frames = &animation->layerAnimations[id].frames;
			break;
		case ANM2_NULL_ANIMATION:
			if (id < 0 || id >= (s32)animation->nullAnimations.size())
				return false;
			frames = &animation->nullAnimations[id].frames;
			break;
		default:
			return false;
	}

	for (s32 i = 0; i < (s32)frames->size(); i++)
	{
		*frame = (*frames)[i];
		delayNext += frame->delay;

		/* If a frame is within the time constraints, it's a time matched frame, break */
		/* Otherwise, the last found frame parsed will be used. */
		if (time >= delayCurrent && time < delayNext)
		{
			if (i + 1 < (s32)frames->size())
				nextFrame = &(*frames)[i + 1];
			else
				nextFrame = NULL;

			isTimeMatchedFrame = true;
			break;
		}

		delayCurrent += frame->delay;
	}

	/* No valid frame found */
	if (!isTimeMatchedFrame)
		return false;

	/* interpolate only if there's a frame following */
	if (frame->isInterpolated && nextFrame)
	{
		f32 interpolationTime = (time - delayCurrent) / (delayNext - delayCurrent);

		frame->rotation    = glm::mix(frame->rotation,    nextFrame->rotation,    interpolationTime);;
		frame->position    = glm::mix(frame->position,    nextFrame->position,    interpolationTime);;
		frame->scale       = glm::mix(frame->scale,       nextFrame->scale,       interpolationTime);;
		frame->offsetRGB   = glm::mix(frame->offsetRGB,   nextFrame->offsetRGB,   interpolationTime);;
		frame->tintRGBA    = glm::mix(frame->tintRGBA,    nextFrame->tintRGBA,    interpolationTime);;
	}

	return true;
}