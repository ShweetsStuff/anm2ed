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

    strftime(date, ANM2_STRING_MAX, STRING_ANM2_CREATED_ON_FORMAT, local);

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

		for (auto & frame : animation.triggers.frames)
		{
			XMLElement* triggerElement;

			/* Trigger */
			triggerElement = document.NewElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_TRIGGER]);
			triggerElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_EVENT_ID], frame.eventID); /* EventID */
			triggerElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_AT_FRAME], frame.atFrame); /* AtFrame */
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

	printf(STRING_INFO_ANM2_WRITE, path);
	strncpy(self->path, path, PATH_MAX - 1);
	
	return true;
}

/* Loads the .anm2 file and deserializes it into the struct equivalent */
bool 
anm2_deserialize(Anm2* self, Resources* resources, const char* path)
{
	XMLDocument xmlDocument;
	XMLError xmlError;
	const XMLElement* xmlElement;
	const XMLElement* xmlRoot;
	Anm2Animation* animation = NULL;
	Anm2Layer* layer = NULL;
	Anm2Null* null = NULL;
	Anm2Item* item = NULL;
	Anm2Event* event = NULL;
	Anm2Frame* frame = NULL;
	Anm2Spritesheet* spritesheet = NULL;
	Anm2Element anm2Element = ANM2_ELEMENT_ANIMATED_ACTOR;
	Anm2Attribute anm2Attribute =  ANM2_ATTRIBUTE_ID;
	Anm2Item addItem;
	Anm2Layer addLayer;
	Anm2Null addNull;
	Anm2Event addEvent;
	Anm2Spritesheet addSpritesheet;

	*self = Anm2{};

	xmlError = xmlDocument.LoadFile(path);

	if (xmlError != XML_SUCCESS)
	{
		printf(STRING_ERROR_ANM2_READ, path, xmlDocument.ErrorStr());
		return false;
	}

	resources_textures_free(resources);
	
	/* Save old working directory and then use anm2's path as directory */
	/* (useful for loading textures from anm2 correctly) */
    std::filesystem::path workingPath = std::filesystem::current_path();
    std::filesystem::path filePath = path;
    std::filesystem::path parentPath = filePath.parent_path();
	std::filesystem::current_path(parentPath);
	strncpy(self->path, path, PATH_MAX - 1);
	
    xmlRoot = xmlDocument.FirstChildElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_ANIMATED_ACTOR]);
	xmlElement = xmlRoot;

	while (xmlElement)
	{
		const XMLAttribute* xmlAttribute = NULL;
		const XMLElement* xmlChild = NULL;
		s32 id = 0;

		/* Elements */
		anm2Element = anm2_element_from_string(xmlElement->Name());

		switch (anm2Element)
		{
			case ANM2_ELEMENT_SPRITESHEET:
				spritesheet = &addSpritesheet;
				break;
			case ANM2_ELEMENT_LAYER:
				layer = &addLayer;
				break;
			case ANM2_ELEMENT_NULL:
				null = &addNull;
				break;
			case ANM2_ELEMENT_EVENT:
				event = &addEvent;
				break;
			case ANM2_ELEMENT_ANIMATION:
				id = map_next_id_get(self->animations);
				self->animations[id] = Anm2Animation{};
				animation = &self->animations[id];
				break;
			case ANM2_ELEMENT_ROOT_ANIMATION:
				item = &animation->rootAnimation;
				break;
			case ANM2_ELEMENT_LAYER_ANIMATION:
			case ANM2_ELEMENT_NULL_ANIMATION:
				item = &addItem;
				break;
			case ANM2_ELEMENT_TRIGGERS:
				item = &animation->triggers;
				break;
			case ANM2_ELEMENT_FRAME:
			case ANM2_ELEMENT_TRIGGER:
				item->frames.push_back(Anm2Frame{});
				frame = &item->frames.back();
			default:
				break;
		}

		/* Attributes */
		xmlAttribute = xmlElement->FirstAttribute();

		while (xmlAttribute)
		{
			anm2Attribute = anm2_attribute_from_string(xmlAttribute->Name());

			switch (anm2Attribute)
			{
				case ANM2_ATTRIBUTE_CREATED_BY:
					strncpy(self->createdBy, xmlAttribute->Value(), ANM2_STRING_MAX - 1);
					break;
				case ANM2_ATTRIBUTE_CREATED_ON:
					strncpy(self->createdOn, xmlAttribute->Value(), ANM2_STRING_MAX - 1);
					break;
				case ANM2_ATTRIBUTE_VERSION:
					self->version = atoi(xmlAttribute->Value());
					break;
				case ANM2_ATTRIBUTE_FPS:
					self->fps = atoi(xmlAttribute->Value());
					break;
				case ANM2_ATTRIBUTE_ID:
					id = atoi(xmlAttribute->Value());
					switch (anm2Element)
					{
						case ANM2_ELEMENT_SPRITESHEET:
							self->spritesheets[id] = addSpritesheet;
							spritesheet = &self->spritesheets[id];
							break;
						case ANM2_ELEMENT_LAYER:
							self->layers[id] = addLayer;
							layer = &self->layers[id];
							break;
						case ANM2_ELEMENT_NULL:
							self->nulls[id] = addNull;
							null = &self->nulls[id];
							break;
						case ANM2_ELEMENT_EVENT:
							self->events[id] = addEvent;
							event = &self->events[id];
							break;
						default:
							break;
					}
					break;
				case ANM2_ATTRIBUTE_LAYER_ID:
					id = atoi(xmlAttribute->Value());
					animation->layerAnimations[id] = addItem;
					item = &animation->layerAnimations[id];
					break;
				case ANM2_ATTRIBUTE_NULL_ID:
					id = atoi(xmlAttribute->Value());
					animation->nullAnimations[id] = addItem;
					item = &animation->nullAnimations[id];
					break;
				case ANM2_ATTRIBUTE_PATH:
					strncpy(spritesheet->path, xmlAttribute->Value(), PATH_MAX - 1);
					break;
				case ANM2_ATTRIBUTE_NAME:
					switch (anm2Element)
					{
						case ANM2_ELEMENT_LAYER:
							strncpy(layer->name, xmlAttribute->Value(), ANM2_STRING_MAX - 1);
							break;
						case ANM2_ELEMENT_NULL:
							strncpy(null->name, xmlAttribute->Value(), ANM2_STRING_MAX - 1);
							break;
						case ANM2_ELEMENT_ANIMATION:
							strncpy(animation->name, xmlAttribute->Value(), ANM2_STRING_MAX - 1);
							break;
						case ANM2_ELEMENT_EVENT:
							strncpy(event->name, xmlAttribute->Value(), ANM2_STRING_MAX - 1);
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
					strncpy(self->defaultAnimation, xmlAttribute->Value(), ANM2_STRING_MAX - 1);
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

		/* Load spritesheet textures */
		if (anm2Element == ANM2_ELEMENT_SPRITESHEET)
			anm2_spritesheet_texture_load(self, resources, spritesheet->path , id);

		/* Iterate through children */
		xmlChild = xmlElement->FirstChildElement();

		if (xmlChild)
		{
			xmlElement = xmlChild;
			continue;
		}

		/* Iterate through siblings */
		while (xmlElement)
		{
			const XMLElement* xmlNext;
			
			xmlNext = xmlElement->NextSiblingElement();
			
			if (xmlNext)
			{
				xmlElement = xmlNext;
				break;
			}

			/* If no siblings, return to parent. If no parent, end parsing */
			xmlElement = xmlElement->Parent() ? xmlElement->Parent()->ToElement() : NULL;
		}
	}

	printf(STRING_INFO_ANM2_READ, path);

	/* Set working directory back to old */
	std::filesystem::current_path(workingPath);

	return true;
}

/* Adds a new layer to the anm2 */
void
anm2_layer_add(Anm2* self)
{
	s32 id = map_next_id_get(self->layers);

	self->layers[id] = Anm2Layer{};

	for (auto & [animationID, animation] : self->animations)
		animation.layerAnimations[id] = Anm2Item{};
}

/* Removes a layer from the anm2 given the index/id */
void
anm2_layer_remove(Anm2* self, s32 id)
{
    self->layers.erase(id);

    for (auto & [animationID, animation] : self->animations)
        animation.layerAnimations.erase(id);
}

/* Adds a new null to the anm2 */
void
anm2_null_add(Anm2* self)
{
	s32 id = map_next_id_get(self->nulls);

	self->nulls[id] = Anm2Null{};

	for (auto & [animationID, animation] : self->animations)
		animation.nullAnimations[id] = Anm2Item{};
}

/* Removes a null from the anm2 given the index/id */
void
anm2_null_remove(Anm2* self, s32 id)
{
    self->nulls.erase(id);

    for (auto & [animationID, animation] : self->animations)
        animation.nullAnimations.erase(id);
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
        animation.layerAnimations[layerID] = Anm2Item{};
	}

	/* match nulls */
    for (auto & [nullID, null] : self->nulls)
	{
    	animation.nullAnimations[nullID] = Anm2Item{};
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

Anm2Animation*
anm2_animation_from_reference(Anm2* self, Anm2Reference* reference)
{
	auto it = self->animations.find(reference->animationID);
	if (it == self->animations.end())
		return NULL;
	return &it->second;
}

/* Returns the item from a anm2 reference. */
Anm2Item*
anm2_item_from_reference(Anm2* self, Anm2Reference* reference)
{
	Anm2Animation* animation = anm2_animation_from_reference(self, reference);
	
	if (!animation)
		return NULL;

	switch (reference->itemType)
	{
		case ANM2_ROOT:
			return &animation->rootAnimation;
		case ANM2_LAYER:
		{
			auto it = animation->layerAnimations.find(reference->itemID);
			if (it == animation->layerAnimations.end())
				return NULL;
			return &it->second;
		}
		case ANM2_NULL:
		{
			auto it = animation->nullAnimations.find(reference->itemID);
			if (it == animation->nullAnimations.end())
				return NULL;
			return &it->second;
		}
		case ANM2_TRIGGERS:
			return &animation->triggers;
		default:
			return NULL;
	}
}

/* Gets the frame from the reference's properties */
Anm2Frame*
anm2_frame_from_reference(Anm2* self, Anm2Reference* reference)
{
	Anm2Item* item = anm2_item_from_reference(self, reference);

	if (!item)
		return NULL;

	if (reference->frameIndex < 0 || reference->frameIndex >= (s32)item->frames.size())
		return NULL;

	return &item->frames[reference->frameIndex];
}

/* Creates/fetches a frame from a given time. */
/* Returns true/false if frame will be valid or not. */
void 
anm2_frame_from_time(Anm2* self, Anm2Frame* frame, Anm2Reference reference, f32 time)
{
	Anm2Animation* animation = anm2_animation_from_reference(self, &reference);

	/* Out of range */
	if (time < 0 || time > animation->frameNum)
		return;

	Anm2Item* item = anm2_item_from_reference(self, &reference);

	if (!item)
		return;

	Anm2Frame* nextFrame = NULL;
	s32 delayCurrent = 0;
	s32 delayNext = 0;

	for (s32 i = 0; i < (s32)item->frames.size(); i++)
	{
		*frame = item->frames[i];
		delayNext += frame->delay;

		/* If a frame is within the time constraints, it's a time matched frame, break */
		/* Otherwise, the last found frame parsed will be used. */
		if (time >= delayCurrent && time < delayNext)
		{
			if (i + 1 < (s32)item->frames.size())
				nextFrame = &item->frames[i + 1];
			else
				nextFrame = NULL;
			break;
		}

		delayCurrent += frame->delay;
	}

	/* Interpolate only if there's a frame following */
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

/* Returns the current length of the animation, from the used frames. */
s32
anm2_animation_length_get(Anm2* self, s32 animationID)
{
	/* Get valid animation */
	auto it = self->animations.find(animationID);
	if (it == self->animations.end())
		return -1;

	Anm2Animation* animation = &it->second;
	s32 delayHighest = 0;

	/* Root frames */
	for (auto & frame : animation->rootAnimation.frames)
		delayHighest = std::max(delayHighest, frame.delay);

	/* Layer frames */
	for (auto & [id, item] : animation->layerAnimations)
		for (auto & frame : item.frames)
			delayHighest = std::max(delayHighest, frame.delay);

	/* Null frames */
	for (auto & [id, item] : animation->nullAnimations)
		for (auto & frame : item.frames)
			delayHighest = std::max(delayHighest, frame.delay);

	/* Trigger frames (assuming this is from `animation->triggers.frames`) */
	for (auto & trigger : animation->triggers.frames)
		delayHighest = std::max(delayHighest, trigger.atFrame);

	return delayHighest;
}

/* Will try adding a frame to the anm2 given the specified reference */
Anm2Frame*
anm2_frame_add(Anm2* self, Anm2Reference* reference, s32 time)
{
	Anm2Animation* animation = anm2_animation_from_reference(self, reference);
	Anm2Item* item = anm2_item_from_reference(self, reference);
	
	if (!animation || !item)
		return NULL;

	if (item)
	{
		Anm2Frame frame = Anm2Frame{};
		s32 index = -1;

		if (reference->itemType == ANM2_TRIGGERS)
		{
			/* don't add redudant triggers (i.e. at same time) */
			for (auto & frameCheck : item->frames)
			{
				if (frameCheck.atFrame == time)
					return NULL;
			}

			frame.atFrame = time;
			index = item->frames.size();
		}
		else
		{
			s32 frameDelayCount = 0;

			/* Add up all delay to see where this new frame might lie */
			for (auto & frameCheck : item->frames)
				frameDelayCount += frameCheck.delay;
			
			/* If adding the smallest frame would be over the length, don't bother */
			if (frameDelayCount + ANM2_FRAME_DELAY_MIN > animation->frameNum)
				return NULL;

			/* Will insert next to frame if frame exists */
			Anm2Frame* checkFrame = anm2_frame_from_reference(self, reference);

			if (checkFrame)
			{
				/* Will shrink frame delay to fit */
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

	return NULL;
}

/* Clears anm2 reference */
void
anm2_reference_clear(Anm2Reference* self)
{
	*self = Anm2Reference{};
}

/* Clears anm2 item reference */
void
anm2_reference_item_clear(Anm2Reference* self)
{
	self->itemType = ANM2_NONE;
	self->itemID = -1;
}

/* Clears anm2 reference */
void
anm2_reference_frame_clear(Anm2Reference* self)
{
	self->frameIndex = -1;
}