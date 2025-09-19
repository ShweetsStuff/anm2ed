#include "anm2.h"

using namespace tinyxml2;

static void _anm2_created_on_set(Anm2* self) {
  auto now = std::chrono::system_clock::now();
  std::time_t time = std::chrono::system_clock::to_time_t(now);
  std::tm localTime = *std::localtime(&time);

  std::ostringstream timeString;
  timeString << std::put_time(&localTime, ANM2_CREATED_ON_FORMAT);
  self->createdOn = timeString.str();
}

static void _anm2_frame_serialize(Anm2Frame* frame, Anm2Type type, XMLDocument* document = nullptr, XMLElement* addElement = nullptr,
                                  std::string* string = nullptr) {
  XMLDocument localDocument;
  XMLDocument* useDocument = document ? document : &localDocument;

  XMLElement* element = useDocument->NewElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_FRAME]);

  if (type == ANM2_TRIGGER) {
    element->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_EVENT_ID], frame->eventID); // EventID
    element->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_AT_FRAME], frame->atFrame); // AtFrame
  } else {
    element->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_X_POSITION], frame->position.x); // XPosition
    element->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_Y_POSITION], frame->position.y); // YPosition

    if (type == ANM2_LAYER) {
      element->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_X_PIVOT], frame->pivot.x); // XPivot
      element->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_Y_PIVOT], frame->pivot.y); // YPivot
      element->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_X_CROP], frame->crop.x);   // XCrop
      element->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_Y_CROP], frame->crop.y);   // YCrop
      element->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_WIDTH], frame->size.x);    // Width
      element->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_HEIGHT], frame->size.y);   // Height
    }

    element->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_X_SCALE], frame->scale.x);                          // XScale
    element->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_Y_SCALE], frame->scale.y);                          // YScale
    element->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_DELAY], frame->delay);                              /* Delay */
    element->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_VISIBLE], frame->isVisible);                        // Visible
    element->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_RED_TINT], FLOAT_TO_UINT8(frame->tintRGBA.r));      // RedTint
    element->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_GREEN_TINT], FLOAT_TO_UINT8(frame->tintRGBA.g));    // GreenTint
    element->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_BLUE_TINT], FLOAT_TO_UINT8(frame->tintRGBA.b));     // BlueTint
    element->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_ALPHA_TINT], FLOAT_TO_UINT8(frame->tintRGBA.a));    // AlphaTint
    element->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_RED_OFFSET], FLOAT_TO_UINT8(frame->offsetRGB.r));   // RedOffset
    element->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_GREEN_OFFSET], FLOAT_TO_UINT8(frame->offsetRGB.g)); // GreenOffset
    element->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_BLUE_OFFSET], FLOAT_TO_UINT8(frame->offsetRGB.b));  // BlueOffset
    element->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_ROTATION], frame->rotation);                        // Rotation
    element->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_INTERPOLATED], frame->isInterpolated);              // Interpolated
  }

  if (addElement)
    addElement->InsertEndChild(element);

  if (string && !document) {
    useDocument->InsertEndChild(element);
    XMLPrinter printer;
    useDocument->Print(&printer);
    *string = std::string(printer.CStr());
  }
}

static void _anm2_animation_serialize(Anm2Animation* animation, XMLDocument* document = nullptr, XMLElement* addElement = nullptr,
                                      std::string* string = nullptr) {
  XMLDocument localDocument;
  XMLDocument* useDocument = document ? document : &localDocument;

  // Animation
  XMLElement* element = useDocument->NewElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_ANIMATION]);
  element->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_NAME], animation->name.c_str());  // Name
  element->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_FRAME_NUM], animation->frameNum); // FrameNum
  element->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_LOOP], animation->isLoop);        // Loop

  // RootAnimation
  XMLElement* rootElement = useDocument->NewElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_ROOT_ANIMATION]);

  for (auto& frame : animation->rootAnimation.frames)
    _anm2_frame_serialize(&frame, ANM2_ROOT, useDocument, rootElement);

  element->InsertEndChild(rootElement);

  // LayerAnimations
  XMLElement* layersElement = useDocument->NewElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_LAYER_ANIMATIONS]);

  for (auto& id : animation->layerOrder) {
    // LayerAnimation
    Anm2Item& layerAnimation = animation->layerAnimations[id];
    XMLElement* layerElement = useDocument->NewElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_LAYER_ANIMATION]);
    layerElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_LAYER_ID], id);                      // LayerId
    layerElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_VISIBLE], layerAnimation.isVisible); // Visible

    for (auto& frame : layerAnimation.frames)
      _anm2_frame_serialize(&frame, ANM2_LAYER, useDocument, layerElement);

    layersElement->InsertEndChild(layerElement);
  }

  element->InsertEndChild(layersElement);

  // Nulls
  XMLElement* nullsElement = useDocument->NewElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_NULL_ANIMATIONS]);

  for (auto& [id, null] : animation->nullAnimations) {
    // NullAnimation
    XMLElement* nullElement = useDocument->NewElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_NULL_ANIMATION]);
    nullElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_NULL_ID], id);             // NullId
    nullElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_VISIBLE], null.isVisible); // Visible

    for (auto& frame : null.frames)
      _anm2_frame_serialize(&frame, ANM2_NULL, useDocument, nullElement);

    nullsElement->InsertEndChild(nullElement);
  }

  element->InsertEndChild(nullsElement);

  // Triggers
  XMLElement* triggersElement = useDocument->NewElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_TRIGGERS]);

  for (auto& frame : animation->triggers.frames)
    _anm2_frame_serialize(&frame, ANM2_TRIGGER, useDocument, triggersElement);

  element->InsertEndChild(triggersElement);

  if (addElement)
    addElement->InsertEndChild(element);

  if (string && !document) {
    useDocument->InsertEndChild(element);
    XMLPrinter printer;
    useDocument->Print(&printer);
    *string = std::string(printer.CStr());
  }
}

bool anm2_serialize(Anm2* self, const std::string& path) {
  XMLDocument document;

  if (!self || path.empty())
    return false;

  if (self->version == 0)
    _anm2_created_on_set(self);

  self->path = path;
  self->version++;

  // AnimatedActor
  XMLElement* animatedActorElement = document.NewElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_ANIMATED_ACTOR]);
  document.InsertFirstChild(animatedActorElement);

  // Info
  XMLElement* infoElement = document.NewElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_INFO]);
  infoElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_CREATED_BY], self->createdBy.c_str()); // CreatedBy
  infoElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_CREATED_ON], self->createdOn.c_str()); // CreatedOn
  infoElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_VERSION], self->version);              // Version
  infoElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_FPS], self->fps);                      // FPS
  animatedActorElement->InsertEndChild(infoElement);

  // Content
  XMLElement* contentElement = document.NewElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_CONTENT]);

  // Spritesheets
  XMLElement* spritesheetsElement = document.NewElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_SPRITESHEETS]);

  for (auto& [id, spritesheet] : self->spritesheets) {
    // Spritesheet
    XMLElement* spritesheetElement = document.NewElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_SPRITESHEET]);
    spritesheetElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_PATH], spritesheet.path.c_str()); // Path
    spritesheetElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_ID], id);                         // ID
    spritesheetsElement->InsertEndChild(spritesheetElement);
  }

  contentElement->InsertEndChild(spritesheetsElement);

  // Layers
  XMLElement* layersElement = document.NewElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_LAYERS]);

  for (auto& [id, layer] : self->layers) {
    // Layer
    XMLElement* layerElement = document.NewElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_LAYER]);
    layerElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_NAME], layer.name.c_str());            // Path
    layerElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_ID], id);                              // ID
    layerElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_SPRITESHEET_ID], layer.spritesheetID); // SpritesheetId
    layersElement->InsertEndChild(layerElement);
  }

  contentElement->InsertEndChild(layersElement);

  // Nulls
  XMLElement* nullsElement = document.NewElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_NULLS]);

  for (auto& [id, null] : self->nulls) {
    // Null
    XMLElement* nullElement = document.NewElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_NULL]);
    nullElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_NAME], null.name.c_str()); // Name
    nullElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_ID], id);                  // ID
    if (null.isShowRect)
      nullElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_SHOW_RECT], null.isShowRect); // ShowRect
    nullsElement->InsertEndChild(nullElement);
  }

  contentElement->InsertEndChild(nullsElement);

  // Events
  XMLElement* eventsElement = document.NewElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_EVENTS]);

  for (auto& [id, event] : self->events) {
    // Event
    XMLElement* eventElement = document.NewElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_EVENT]);
    eventElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_NAME], event.name.c_str()); // Name
    eventElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_ID], id);                   // ID
    eventsElement->InsertEndChild(eventElement);
  }

  contentElement->InsertEndChild(eventsElement);

  animatedActorElement->InsertEndChild(contentElement);

  // Animations
  XMLElement* animationsElement = document.NewElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_ANIMATIONS]);
  if (self->defaultAnimationID != ID_NONE)
    animationsElement->SetAttribute(ANM2_ATTRIBUTE_STRINGS[ANM2_ATTRIBUTE_DEFAULT_ANIMATION],
                                    self->animations[self->defaultAnimationID].name.c_str()); // DefaultAnimation

  for (auto& [id, animation] : self->animations)
    _anm2_animation_serialize(&animation, &document, animationsElement);

  animatedActorElement->InsertEndChild(animationsElement);

  XMLError error = document.SaveFile(path.c_str());

  if (error != XML_SUCCESS) {
    log_error(std::format(ANM2_WRITE_ERROR, path));
    return false;
  }

  log_info(std::format(ANM2_WRITE_INFO, path));

  return true;
}

static void _anm2_frame_deserialize(Anm2Frame* frame, const XMLElement* element) {
  for (const XMLAttribute* attribute = element->FirstAttribute(); attribute; attribute = attribute->Next()) {
    switch (ANM2_ATTRIBUTE_STRING_TO_ENUM(attribute->Name())) {
    case ANM2_ATTRIBUTE_X_POSITION:
      frame->position.x = std::atof(attribute->Value());
      break; // XPosition
    case ANM2_ATTRIBUTE_Y_POSITION:
      frame->position.y = std::atof(attribute->Value());
      break; // YPosition
    case ANM2_ATTRIBUTE_X_PIVOT:
      frame->pivot.x = std::atof(attribute->Value());
      break; // XPivot
    case ANM2_ATTRIBUTE_Y_PIVOT:
      frame->pivot.y = std::atof(attribute->Value());
      break; // YPivot
    case ANM2_ATTRIBUTE_X_CROP:
      frame->crop.x = std::atof(attribute->Value());
      break; // XCrop
    case ANM2_ATTRIBUTE_Y_CROP:
      frame->crop.y = std::atof(attribute->Value());
      break; // YCrop
    case ANM2_ATTRIBUTE_WIDTH:
      frame->size.x = std::atof(attribute->Value());
      break; // Width
    case ANM2_ATTRIBUTE_HEIGHT:
      frame->size.y = std::atof(attribute->Value());
      break; // Height
    case ANM2_ATTRIBUTE_X_SCALE:
      frame->scale.x = std::atof(attribute->Value());
      break; // XScale
    case ANM2_ATTRIBUTE_Y_SCALE:
      frame->scale.y = std::atof(attribute->Value());
      break; // YScale
    case ANM2_ATTRIBUTE_RED_TINT:
      frame->tintRGBA.r = UINT8_TO_FLOAT(std::atoi(attribute->Value()));
      break; // RedTint
    case ANM2_ATTRIBUTE_GREEN_TINT:
      frame->tintRGBA.g = UINT8_TO_FLOAT(std::atoi(attribute->Value()));
      break; // GreenTint
    case ANM2_ATTRIBUTE_BLUE_TINT:
      frame->tintRGBA.b = UINT8_TO_FLOAT(std::atoi(attribute->Value()));
      break; // BlueTint
    case ANM2_ATTRIBUTE_ALPHA_TINT:
      frame->tintRGBA.a = UINT8_TO_FLOAT(std::atoi(attribute->Value()));
      break; // AlphaTint
    case ANM2_ATTRIBUTE_RED_OFFSET:
      frame->offsetRGB.r = UINT8_TO_FLOAT(std::atoi(attribute->Value()));
      break; // RedOffset
    case ANM2_ATTRIBUTE_GREEN_OFFSET:
      frame->offsetRGB.g = UINT8_TO_FLOAT(std::atoi(attribute->Value()));
      break; // GreenOffset
    case ANM2_ATTRIBUTE_BLUE_OFFSET:
      frame->offsetRGB.b = UINT8_TO_FLOAT(std::atoi(attribute->Value()));
      break; // BlueOffset
    case ANM2_ATTRIBUTE_ROTATION:
      frame->rotation = std::atof(attribute->Value());
      break; // Rotation
    case ANM2_ATTRIBUTE_VISIBLE:
      frame->isVisible = string_to_bool(attribute->Value());
      break; // Visible
    case ANM2_ATTRIBUTE_INTERPOLATED:
      frame->isInterpolated = string_to_bool(attribute->Value());
      break; // Interpolated
    case ANM2_ATTRIBUTE_AT_FRAME:
      frame->atFrame = std::atoi(attribute->Value());
      break; // AtFrame
    case ANM2_ATTRIBUTE_DELAY:
      frame->delay = std::atoi(attribute->Value());
      break; // Delay
    case ANM2_ATTRIBUTE_EVENT_ID:
      frame->eventID = std::atoi(attribute->Value());
      break; // EventID
    default:
      break;
    }
  }
}

static void _anm2_animation_deserialize(Anm2Animation* animation, const XMLElement* element) {
  auto frames_deserialize = [&](const XMLElement* itemElement, Anm2Item* item) {
    // Frame
    for (const XMLElement* frame = itemElement->FirstChildElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_FRAME]); frame;
         frame = frame->NextSiblingElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_FRAME]))
      _anm2_frame_deserialize(&item->frames.emplace_back(Anm2Frame()), frame);
  };

  int id{};

  for (const XMLAttribute* attribute = element->FirstAttribute(); attribute; attribute = attribute->Next()) {
    switch (ANM2_ATTRIBUTE_STRING_TO_ENUM(attribute->Name())) {
    case ANM2_ATTRIBUTE_NAME:
      animation->name = std::string(attribute->Value());
      break; // Name
    case ANM2_ATTRIBUTE_FRAME_NUM:
      animation->frameNum = std::atoi(attribute->Value());
      break; // FrameNum
    case ANM2_ATTRIBUTE_LOOP:
      animation->isLoop = string_to_bool(attribute->Value());
      break; // Loop
    default:
      break;
    }
  }

  // RootAnimation
  if (const XMLElement* rootAnimation = element->FirstChildElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_ROOT_ANIMATION]))
    frames_deserialize(rootAnimation, &animation->rootAnimation);

  // LayerAnimations
  if (const XMLElement* layerAnimations = element->FirstChildElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_LAYER_ANIMATIONS])) {
    // LayerAnimation
    for (const XMLElement* layerAnimation = layerAnimations->FirstChildElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_LAYER_ANIMATION]); layerAnimation;
         layerAnimation = layerAnimation->NextSiblingElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_LAYER_ANIMATION])) {
      Anm2Item layerAnimationItem;

      for (const XMLAttribute* attribute = layerAnimation->FirstAttribute(); attribute; attribute = attribute->Next()) {
        switch (ANM2_ATTRIBUTE_STRING_TO_ENUM(attribute->Name())) {
        case ANM2_ATTRIBUTE_LAYER_ID:
          id = std::atoi(attribute->Value());
          break; // LayerID
        case ANM2_ATTRIBUTE_VISIBLE:
          layerAnimationItem.isVisible = string_to_bool(attribute->Value());
          break; // Visible
        default:
          break;
        }
      }

      frames_deserialize(layerAnimation, &layerAnimationItem);
      animation->layerAnimations[id] = layerAnimationItem;
      animation->layerOrder.push_back(id);
    }
  }

  // NullAnimations
  if (const XMLElement* nullAnimations = element->FirstChildElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_NULL_ANIMATIONS])) {
    // NullAnimation
    for (const XMLElement* nullAnimation = nullAnimations->FirstChildElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_NULL_ANIMATION]); nullAnimation;
         nullAnimation = nullAnimation->NextSiblingElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_NULL_ANIMATION])) {
      Anm2Item nullAnimationItem;

      for (const XMLAttribute* attribute = nullAnimation->FirstAttribute(); attribute; attribute = attribute->Next()) {
        switch (ANM2_ATTRIBUTE_STRING_TO_ENUM(attribute->Name())) {
        case ANM2_ATTRIBUTE_NULL_ID:
          id = std::atoi(attribute->Value());
          break;
        case ANM2_ATTRIBUTE_VISIBLE:
          nullAnimationItem.isVisible = string_to_bool(attribute->Value());
          break;
        default:
          break;
        }
      }

      frames_deserialize(nullAnimation, &nullAnimationItem);
      animation->nullAnimations[id] = nullAnimationItem;
    }
  }

  // Triggers
  if (const XMLElement* triggers = element->FirstChildElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_TRIGGERS])) {
    // Trigger
    for (const XMLElement* trigger = triggers->FirstChildElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_TRIGGER]); trigger;
         trigger = trigger->NextSiblingElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_TRIGGER]))
      _anm2_frame_deserialize(&animation->triggers.frames.emplace_back(Anm2Frame()), trigger);
  }
}

bool anm2_deserialize(Anm2* self, const std::string& path, bool isTextures) {
  if (!self)
    return false;

  if (path.empty()) {
    log_error(ANM2_EMPTY_ERROR);
    return false;
  }

  XMLDocument document;
  if (document.LoadFile(path.c_str()) != XML_SUCCESS) {
    log_error(std::format(ANM2_PARSE_ERROR, path, document.ErrorStr()));
    return false;
  }

  anm2_new(self);
  self->path = path;
  std::string defaultAnimation{};
  int id{};

  // Save old working directory and then use anm2's path as directory
  // (used for loading textures from anm2 correctly which are relative)
  std::filesystem::path workingPath = std::filesystem::current_path();
  working_directory_from_file_set(path);

  const XMLElement* root = document.RootElement();

  // Info
  if (const XMLElement* info = root->FirstChildElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_INFO])) {
    for (const XMLAttribute* attribute = info->FirstAttribute(); attribute; attribute = attribute->Next()) {
      switch (ANM2_ATTRIBUTE_STRING_TO_ENUM(attribute->Name())) {
      case ANM2_ATTRIBUTE_CREATED_BY:
        self->createdBy = std::string(attribute->Value());
        break; // CreatedBy
      case ANM2_ATTRIBUTE_CREATED_ON:
        self->createdOn = std::string(attribute->Value());
        break; // CreatedOn
      case ANM2_ATTRIBUTE_VERSION:
        self->version = std::atoi(attribute->Value());
        break; // Version
      case ANM2_ATTRIBUTE_FPS:
        self->fps = std::atoi(attribute->Value());
        break; // FPS
      default:
        break;
      }
    }
  }

  // Content
  if (const XMLElement* content = root->FirstChildElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_CONTENT])) {
    // Spritesheets
    if (const XMLElement* spritesheets = content->FirstChildElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_SPRITESHEETS])) {
      for (const XMLElement* spritesheet = spritesheets->FirstChildElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_SPRITESHEET]); spritesheet;
           spritesheet = spritesheet->NextSiblingElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_SPRITESHEET])) {
        Anm2Spritesheet addSpritesheet;

        for (const XMLAttribute* attribute = spritesheet->FirstAttribute(); attribute; attribute = attribute->Next()) {
          switch (ANM2_ATTRIBUTE_STRING_TO_ENUM(attribute->Name())) {
          case ANM2_ATTRIBUTE_PATH:
            // Spritesheet paths from Isaac Rebirth are made with the assumption that
            // the paths are case-insensitive (as the game was developed on Windows)
            // However when using the resource dumper, the spritesheet paths are all lowercase (on Linux anyways)
            // If the check doesn't work, set the spritesheet path to lowercase
            // If the check doesn't work, replace backslashes with slashes
            // At the minimum this should make all textures be able to be loaded on Linux
            // If it doesn't work beyond that then that's on the user :^)
            addSpritesheet.path = attribute->Value();
            if (!path_exists(addSpritesheet.path))
              addSpritesheet.path = string_to_lowercase(addSpritesheet.path);
            if (!path_exists(addSpritesheet.path))
              addSpritesheet.path = string_backslash_replace(addSpritesheet.path);
            if (isTextures)
              texture_from_path_init(&addSpritesheet.texture, addSpritesheet.path);
            break;
          case ANM2_ATTRIBUTE_ID:
            id = std::atoi(attribute->Value());
            break;
          default:
            break;
          }
        }

        self->spritesheets[id] = addSpritesheet;
      }
    }

    // Layers
    if (const XMLElement* layers = content->FirstChildElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_LAYERS])) {
      for (const XMLElement* layer = layers->FirstChildElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_LAYER]); layer;
           layer = layer->NextSiblingElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_LAYER])) {
        Anm2Layer addLayer;

        for (const XMLAttribute* attribute = layer->FirstAttribute(); attribute; attribute = attribute->Next()) {
          switch (ANM2_ATTRIBUTE_STRING_TO_ENUM(attribute->Name())) {
          case ANM2_ATTRIBUTE_NAME:
            addLayer.name = std::string(attribute->Value());
            break; // Name
          case ANM2_ATTRIBUTE_ID:
            id = std::atoi(attribute->Value());
            break; // ID
          case ANM2_ATTRIBUTE_SPRITESHEET_ID:
            addLayer.spritesheetID = std::atoi(attribute->Value());
            break; // ID
          default:
            break;
          }
        }

        self->layers[id] = addLayer;
      }
    }

    // Nulls
    if (const XMLElement* nulls = content->FirstChildElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_NULLS])) {
      for (const XMLElement* null = nulls->FirstChildElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_NULL]); null;
           null = null->NextSiblingElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_NULL])) {
        Anm2Null addNull;

        for (const XMLAttribute* attribute = null->FirstAttribute(); attribute; attribute = attribute->Next()) {
          switch (ANM2_ATTRIBUTE_STRING_TO_ENUM(attribute->Name())) {
          case ANM2_ATTRIBUTE_NAME:
            addNull.name = std::string(attribute->Value());
            break; // Name
          case ANM2_ATTRIBUTE_ID:
            id = std::atoi(attribute->Value());
            break; // IDs
          default:
            break;
          }
        }

        self->nulls[id] = addNull;
      }
    }

    // Events
    if (const XMLElement* events = content->FirstChildElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_EVENTS])) {
      // Event
      for (const XMLElement* event = events->FirstChildElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_EVENT]); event;
           event = event->NextSiblingElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_EVENT])) {
        Anm2Event addEvent;

        for (const XMLAttribute* attribute = event->FirstAttribute(); attribute; attribute = attribute->Next()) {
          switch (ANM2_ATTRIBUTE_STRING_TO_ENUM(attribute->Name())) {
          case ANM2_ATTRIBUTE_NAME:
            addEvent.name = std::string(attribute->Value());
            break; // Name
          case ANM2_ATTRIBUTE_ID:
            id = std::atoi(attribute->Value());
            break; // ID
          default:
            break;
          }
        }

        self->events[id] = addEvent;
      }
    }
  }

  // Animations
  if (const XMLElement* animations = root->FirstChildElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_ANIMATIONS])) {
    for (const XMLAttribute* attribute = animations->FirstAttribute(); attribute; attribute = attribute->Next()) {
      switch (ANM2_ATTRIBUTE_STRING_TO_ENUM(attribute->Name())) {
      case ANM2_ATTRIBUTE_DEFAULT_ANIMATION:
        defaultAnimation = std::string(attribute->Value());
        break; // DefaultAnimation
      default:
        break;
      }
    }

    // Animation
    for (const XMLElement* animation = animations->FirstChildElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_ANIMATION]); animation;
         animation = animation->NextSiblingElement(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_ANIMATION]))
      _anm2_animation_deserialize(&self->animations[map_next_id_get(self->animations)], animation);
  }

  for (auto& [id, animation] : self->animations)
    if (animation.name == defaultAnimation)
      self->defaultAnimationID = id;

  if (isTextures)
    anm2_spritesheet_texture_pixels_download(self);

  std::filesystem::current_path(workingPath);

  log_info(std::format(ANM2_READ_INFO, path));

  return true;
}

void anm2_animation_layer_animation_add(Anm2Animation* animation, int id) {
  animation->layerAnimations[id] = Anm2Item{};
  animation->layerOrder.push_back(id);
}

void anm2_animation_layer_animation_remove(Anm2Animation* animation, int id) {
  animation->layerAnimations.erase(id);
  vector_value_erase(animation->layerOrder, id);
}

void anm2_animation_null_animation_add(Anm2Animation* animation, int id) { animation->nullAnimations[id] = Anm2Item{}; }

void anm2_animation_null_animation_remove(Anm2Animation* animation, int id) { animation->nullAnimations.erase(id); }

int anm2_layer_add(Anm2* self) {
  int id = map_next_id_get(self->layers);
  self->layers[id] = Anm2Layer{};
  return id;
}

void anm2_layer_remove(Anm2* self, int id) {
  self->layers.erase(id);

  for (auto& [_, animation] : self->animations)
    anm2_animation_layer_animation_remove(&animation, id);
}

int anm2_null_add(Anm2* self) {
  int id = map_next_id_get(self->nulls);
  self->nulls[id] = Anm2Null{};
  return id;
}

void anm2_null_remove(Anm2* self, int id) {
  if (!self->nulls.contains(id))
    return;

  self->nulls.erase(id);

  for (auto& [_, animation] : self->animations)
    anm2_animation_null_animation_remove(&animation, id);
}

int anm2_animation_add(Anm2* self, Anm2Animation* animation, int id) {
  int addID = map_next_id_get(self->animations);

  Anm2Animation localAnimation;
  Anm2Animation* addAnimation = animation ? animation : &localAnimation;

  if (!animation)
    addAnimation->rootAnimation.frames.push_back(Anm2Frame{});

  if (id != ID_NONE) {
    map_insert_shift(self->animations, id, *addAnimation);
    return id + 1;
  } else
    self->animations[addID] = *addAnimation;

  return addID;
}

void anm2_animation_remove(Anm2* self, int id) { self->animations.erase(id); }

void anm2_new(Anm2* self) {
  *self = Anm2{};
  _anm2_created_on_set(self);
}

Anm2Animation* anm2_animation_from_reference(Anm2* self, Anm2Reference reference) { return map_find(self->animations, reference.animationID); }

Anm2Item* anm2_item_from_reference(Anm2* self, Anm2Reference reference) {
  if (reference.itemType == ANM2_NONE)
    return nullptr;

  Anm2Animation* animation = anm2_animation_from_reference(self, reference);

  if (!animation)
    return nullptr;

  switch (reference.itemType) {
  case ANM2_ROOT:
    return &animation->rootAnimation;
  case ANM2_LAYER:
    return map_find(animation->layerAnimations, reference.itemID);
  case ANM2_NULL:
    return map_find(animation->nullAnimations, reference.itemID);
  case ANM2_TRIGGER:
    return &animation->triggers;
  default:
    return nullptr;
  }
}

int anm2_frame_index_from_time(Anm2* self, Anm2Reference reference, float time) {
  Anm2Animation* animation = anm2_animation_from_reference(self, reference);
  if (!animation)
    return INDEX_NONE;
  if (time < 0 || time > animation->frameNum)
    return INDEX_NONE;

  Anm2Item* item = anm2_item_from_reference(self, reference);

  if (!item)
    return INDEX_NONE;

  if (reference.itemType == ANM2_TRIGGER)
    for (auto [i, frame] : std::views::enumerate(item->frames))
      if (frame.atFrame == (int)time)
        return i;

  int delayCurrent = 0;
  int delayNext = 0;

  for (auto [i, frame] : std::views::enumerate(item->frames)) {
    delayNext += frame.delay;

    if (time >= delayCurrent && time < delayNext)
      return i;

    delayCurrent += frame.delay;
  }

  return INDEX_NONE;
}

float anm2_time_from_reference(Anm2* self, Anm2Reference reference) {
  Anm2Animation* animation = anm2_animation_from_reference(self, reference);
  if (!animation)
    return INDEX_NONE;

  Anm2Item* item = anm2_item_from_reference(self, reference);
  if (!item)
    return INDEX_NONE;

  float time = 0.0f;

  if (reference.frameIndex <= 0)
    return 0.0f;

  if (reference.frameIndex >= (int)item->frames.size()) {
    for (auto& frame : item->frames)
      time += (float)frame.delay;
    return time;
  }

  if (reference.itemType == ANM2_TRIGGER)
    return (float)item->frames[reference.frameIndex].atFrame;

  for (int i = 0; i < reference.frameIndex; i++)
    time += (float)item->frames[i].delay;

  return time;
}

Anm2Frame* anm2_frame_from_reference(Anm2* self, Anm2Reference reference) {
  Anm2Item* item = anm2_item_from_reference(self, reference);

  if (!item)
    return nullptr;

  if (reference.frameIndex <= INDEX_NONE || reference.frameIndex >= (int)item->frames.size())
    return nullptr;

  return &item->frames[reference.frameIndex];
}

void anm2_frame_from_time(Anm2* self, Anm2Frame* frame, Anm2Reference reference, float time) {
  Anm2Animation* animation = anm2_animation_from_reference(self, reference);
  if (!animation)
    return;

  time = std::clamp(ROUND_NEAREST_MULTIPLE(time, 1.0f), 0.0f, animation->frameNum - 1.0f);

  Anm2Item* item = anm2_item_from_reference(self, reference);

  if (!item)
    return;

  Anm2Frame* frameNext = nullptr;
  int delayCurrent = 0;
  int delayNext = 0;

  for (auto [i, iFrame] : std::views::enumerate(item->frames)) {
    if (reference.itemType == ANM2_TRIGGER) {
      if ((int)time == iFrame.atFrame) {
        *frame = iFrame;
        break;
      }
    } else {
      *frame = iFrame;

      delayNext += frame->delay;

      if (time >= delayCurrent && time < delayNext) {
        if (i + 1 < (int)item->frames.size())
          frameNext = &item->frames[i + 1];
        else
          frameNext = nullptr;
        break;
      }

      delayCurrent += frame->delay;
    }
  }

  if (reference.itemType == ANM2_TRIGGER)
    return;

  if (frame->isInterpolated && frameNext && frame->delay > 1) {
    float interpolation = (time - delayCurrent) / (delayNext - delayCurrent);

    frame->rotation = glm::mix(frame->rotation, frameNext->rotation, interpolation);
    frame->position = glm::mix(frame->position, frameNext->position, interpolation);
    frame->scale = glm::mix(frame->scale, frameNext->scale, interpolation);
    frame->offsetRGB = glm::mix(frame->offsetRGB, frameNext->offsetRGB, interpolation);
    frame->tintRGBA = glm::mix(frame->tintRGBA, frameNext->tintRGBA, interpolation);
  }
}

int anm2_animation_length_get(Anm2Animation* self) {
  int length = 0;

  auto accumulate_max_delay = [&](const std::vector<Anm2Frame>& frames) {
    int delaySum = 0;
    for (const auto& frame : frames) {
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

void anm2_animation_length_set(Anm2Animation* self) { self->frameNum = anm2_animation_length_get(self); }

Anm2Frame* anm2_frame_add(Anm2* self, Anm2Frame* frame, Anm2Reference reference) {
  Anm2Animation* animation = anm2_animation_from_reference(self, reference);
  Anm2Item* item = anm2_item_from_reference(self, reference);
  if (!animation || !item)
    return nullptr;

  Anm2Frame frameAdd = frame ? *frame : Anm2Frame{};

  if (reference.itemType == ANM2_TRIGGER) {
    int atFrame = frame ? frame->atFrame : 0;

    for (auto& frameCheck : item->frames) {
      if (frameCheck.atFrame == atFrame) {
        atFrame++;
        break;
      }
    }

    frameAdd.atFrame = atFrame;

    return &item->frames.emplace_back(frameAdd);
  } else {
    int index = reference.frameIndex;

    if (index < 0 || index >= (int)item->frames.size()) {
      item->frames.push_back(frameAdd);
      return &item->frames.back();
    } else {
      item->frames.insert(item->frames.begin() + index, frameAdd);
      return &item->frames[(size_t)(index)];
    }
  }
}

void anm2_frame_remove(Anm2* self, Anm2Reference reference) {
  Anm2Item* item = anm2_item_from_reference(self, reference);
  if (!item)
    return;
  item->frames.erase(item->frames.begin() + reference.frameIndex);
}

void anm2_item_frame_set(Anm2* self, Anm2Reference reference, const Anm2FrameChange& change, Anm2ChangeType changeType, int start, int count) {
  Anm2Item* item = anm2_item_from_reference(self, reference);
  if (!item)
    return;

  int size = (int)item->frames.size();
  if (size == 0 || start >= size)
    return;
  if (start < 0 || count <= 0)
    return;

  int end = std::min(start + count, size);

  for (int i = start; i < end; i++) {
    Anm2Frame& dest = item->frames[i];

#define X(name, ctype, ...)                                                                                                                                    \
  if (change.name) {                                                                                                                                           \
    if constexpr (std::is_same_v<decltype(dest.name), bool>) {                                                                                                 \
      dest.name = *change.name;                                                                                                                                \
    } else {                                                                                                                                                   \
      switch (changeType) {                                                                                                                                    \
      case ANM2_CHANGE_SET:                                                                                                                                    \
        dest.name = *change.name;                                                                                                                              \
        break;                                                                                                                                                 \
      case ANM2_CHANGE_ADD:                                                                                                                                    \
        dest.name += *change.name;                                                                                                                             \
        break;                                                                                                                                                 \
      case ANM2_CHANGE_SUBTRACT:                                                                                                                               \
        dest.name -= *change.name;                                                                                                                             \
        break;                                                                                                                                                 \
      }                                                                                                                                                        \
    }                                                                                                                                                          \
  }
    ANM2_FRAME_MEMBERS
#undef X
  }
}

void anm2_animation_merge(Anm2* self, int animationID, const std::vector<int>& mergeIDs, Anm2MergeType type) {
  Anm2Animation newAnimation = self->animations[animationID];

  auto merge_item = [&](Anm2Item& destinationItem, const Anm2Item& sourceItem) {
    switch (type) {
    case ANM2_MERGE_APPEND:
      destinationItem.frames.insert(destinationItem.frames.end(), sourceItem.frames.begin(), sourceItem.frames.end());
      break;
    case ANM2_MERGE_PREPEND:
      destinationItem.frames.insert(destinationItem.frames.begin(), sourceItem.frames.begin(), sourceItem.frames.end());
      break;
    case ANM2_MERGE_REPLACE:
      if (destinationItem.frames.size() < sourceItem.frames.size())
        destinationItem.frames.resize(sourceItem.frames.size());
      for (int i = 0; i < (int)sourceItem.frames.size(); i++)
        destinationItem.frames[i] = sourceItem.frames[i];
      break;
    case ANM2_MERGE_IGNORE:
      break;
    }
  };

  for (auto mergeID : mergeIDs) {
    if (animationID == mergeID)
      continue;

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

void anm2_frame_bake(Anm2* self, Anm2Reference reference, int interval, bool isRoundScale, bool isRoundRotation) {
  Anm2Item* item = anm2_item_from_reference(self, reference);
  if (!item)
    return;

  Anm2Frame* frame = anm2_frame_from_reference(self, reference);
  if (!frame)
    return;

  Anm2Reference referenceNext = {reference.animationID, reference.itemType, reference.itemID, reference.frameIndex + 1};
  Anm2Frame* frameNext = anm2_frame_from_reference(self, referenceNext);
  if (!frameNext)
    frameNext = frame;

  const Anm2Frame baseFrame = *frame;
  const Anm2Frame baseFrameNext = *frameNext;

  int delay = 0;
  int insertIndex = reference.frameIndex;

  while (delay < baseFrame.delay) {
    float interpolation = (float)delay / baseFrame.delay;

    Anm2Frame baked = *frame;
    baked.delay = std::min(interval, baseFrame.delay - delay);
    baked.isInterpolated = (insertIndex == reference.frameIndex) ? baseFrame.isInterpolated : false;

    baked.rotation = glm::mix(baseFrame.rotation, baseFrameNext.rotation, interpolation);
    baked.position = glm::mix(baseFrame.position, baseFrameNext.position, interpolation);
    baked.scale = glm::mix(baseFrame.scale, baseFrameNext.scale, interpolation);
    baked.offsetRGB = glm::mix(baseFrame.offsetRGB, baseFrameNext.offsetRGB, interpolation);
    baked.tintRGBA = glm::mix(baseFrame.tintRGBA, baseFrameNext.tintRGBA, interpolation);

    if (isRoundScale)
      baked.scale = vec2((int)baked.scale.x, (int)baked.scale.y);
    if (isRoundRotation)
      baked.rotation = (int)baked.rotation;

    if (insertIndex == reference.frameIndex)
      item->frames[insertIndex] = baked;
    else
      item->frames.insert(item->frames.begin() + insertIndex, baked);
    insertIndex++;

    delay += baked.delay;
  }
}

void anm2_scale(Anm2* self, float scale) {
  auto frame_scale = [&](Anm2Frame& frame) {
    frame.position = vec2((int)(frame.position.x * scale), (int)(frame.position.y * scale));
    frame.size = vec2((int)(frame.size.x * scale), (int)(frame.size.y * scale));
    frame.crop = vec2((int)(frame.crop.x * scale), (int)(frame.crop.y * scale));
    frame.pivot = vec2((int)(frame.pivot.x * scale), (int)(frame.pivot.y * scale));
  };

  for (auto& [_, animation] : self->animations) {
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

void anm2_generate_from_grid(Anm2* self, Anm2Reference reference, vec2 startPosition, vec2 size, vec2 pivot, int columns, int count, int delay) {
  Anm2Item* item = anm2_item_from_reference(self, reference);
  if (!item)
    return;

  Anm2Reference frameReference = reference;

  for (int i = 0; i < count; i++) {
    const int row = i / columns;
    const int column = i % columns;

    Anm2Frame frame{};

    frame.delay = delay;
    frame.pivot = pivot;
    frame.size = size;
    frame.crop = startPosition + vec2(size.x * column, size.y * row);

    anm2_frame_add(self, &frame, frameReference);
    frameReference.frameIndex++;
  }
}

void anm2_free(Anm2* self) {
  for (auto& [id, spritesheet] : self->spritesheets)
    texture_free(&spritesheet.texture);
}

void anm2_spritesheet_texture_pixels_upload(Anm2* self) {
  for (auto& [_, spritesheet] : self->spritesheets) {
    Texture& texture = spritesheet.texture;

    if (texture.id != GL_ID_NONE && !texture.isInvalid) {
      assert(!spritesheet.pixels.empty());
      glBindTexture(GL_TEXTURE_2D, texture.id);
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texture.size.x, texture.size.y, GL_RGBA, GL_UNSIGNED_BYTE, spritesheet.pixels.data());
    }
  }
}

void anm2_spritesheet_texture_pixels_download(Anm2* self) {
  for (auto& [_, spritesheet] : self->spritesheets) {
    Texture& texture = spritesheet.texture;

    if (texture.id != GL_ID_NONE && !texture.isInvalid) {
      size_t bufferSize = (size_t)texture.size.x * (size_t)texture.size.y * TEXTURE_CHANNELS;
      spritesheet.pixels.resize(bufferSize);
      glBindTexture(GL_TEXTURE_2D, texture.id);
      glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, spritesheet.pixels.data());
    }
  }
}

vec4 anm2_animation_rect_get(Anm2* self, Anm2Reference reference, bool isRootTransform) {
  float minX = std::numeric_limits<float>::infinity();
  float minY = std::numeric_limits<float>::infinity();
  float maxX = -std::numeric_limits<float>::infinity();
  float maxY = -std::numeric_limits<float>::infinity();

  bool any = false;

  Anm2Frame frame;
  Anm2Frame root;

  Anm2Animation* animation = anm2_animation_from_reference(self, reference);
  if (!animation)
    return vec4(-1.0f);

  for (float t = 0.0f; t <= animation->frameNum; t += 1.0f) {
    for (const auto& [id, _] : animation->layerAnimations) {
      anm2_frame_from_time(self, &frame, {reference.animationID, ANM2_LAYER, id}, t);
      if (!frame.isVisible)
        continue;
      if (frame.size.x <= 0 || frame.size.y <= 0)
        continue;

      mat4 rootModel(1.0f);
      if (isRootTransform) {
        anm2_frame_from_time(self, &root, {reference.animationID, ANM2_ROOT}, t);
        rootModel = quad_model_parent_get(root.position, root.pivot, PERCENT_TO_UNIT(root.scale), root.rotation);
      }

      mat4 model = quad_model_get(frame.size, frame.position, frame.pivot, PERCENT_TO_UNIT(frame.scale), frame.rotation);
      mat4 fullModel = rootModel * model;

      vec2 corners[4] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};

      for (auto& corner : corners) {
        vec4 world = fullModel * vec4(corner, 0.0f, 1.0f);
        minX = std::min(minX, world.x);
        minY = std::min(minY, world.y);
        maxX = std::max(maxX, world.x);
        maxY = std::max(maxY, world.y);
        any = true;
      }
    }
  }

  if (!any)
    return vec4(-1.0f);
  return {minX, minY, maxX - minX, maxY - minY};
}

void anm2_animation_serialize_to_string(Anm2Animation* animation, std::string* string) { _anm2_animation_serialize(animation, nullptr, nullptr, string); }

void anm2_frame_serialize_to_string(Anm2Frame* frame, Anm2Type type, std::string* string) { _anm2_frame_serialize(frame, type, nullptr, nullptr, string); }

bool anm2_animation_deserialize_from_xml(Anm2Animation* animation, const std::string& xml) {
  XMLDocument document;

  auto animation_deserialize_error = [&]() {
    log_error(std::format(ANM2_ANIMATION_PARSE_ERROR, xml, document.ErrorStr()));
    return false;
  };

  if (document.Parse(xml.c_str()) != XML_SUCCESS)
    return animation_deserialize_error();

  const XMLElement* element = document.RootElement();
  if (!element)
    return animation_deserialize_error();
  if (std::string(element->Name()) != std::string(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_ANIMATION]))
    return animation_deserialize_error();

  _anm2_animation_deserialize(animation, element);
  return true;
}

bool anm2_frame_deserialize_from_xml(Anm2Frame* frame, const std::string& xml) {
  XMLDocument document;

  auto frame_deserialize_error = [&]() {
    log_error(std::format(ANM2_FRAME_PARSE_ERROR, xml, document.ErrorStr()));
    return false;
  };

  if (document.Parse(xml.c_str()) != XML_SUCCESS)
    return frame_deserialize_error();

  const XMLElement* element = document.RootElement();
  if (!element)
    return frame_deserialize_error();

  if (std::string(element->Name()) != std::string(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_FRAME]) &&
      std::string(element->Name()) != std::string(ANM2_ELEMENT_STRINGS[ANM2_ELEMENT_TRIGGER]))
    return frame_deserialize_error();

  _anm2_frame_deserialize(frame, element);

  return true;
}

/*
void anm2_merge(Anm2* self, const std::string& path, Anm2MergeType type)
{
        Anm2 anm2;

        if (anm2_deserialize(&anm2, path, false))
        {
                std::unordered_map<int, int> spritesheetMap;
                for (auto& [id, spritesheet] : anm2.spritesheets)
                {
                        bool isExists = false;

                        for (auto& [selfID, selfSpritesheet] : self->spritesheets)
                        {
                                if (spritesheet.path == selfSpritesheet.path) isExists = true;
                                spritesheetMap[id] = selfID;
                        }

                        if (isExists) continue;

                        int nextID = map_next_id_get(self->spritesheets);
                        self->spritesheet[nextID] = spritesheet;
                        spritesheetMap[id] = nextID;
                }

                std::unordered_map<int, int> layerMap;
                for (auto& [id, layer] : anm2.layers)
                {
                        bool isExists = false;

                        layer.spritesheetID = spritesheetMap[layer.spritesheetID];

                        for (auto& [selfID, selfLayer] : self->layers)
                        {
                                if (layer.name == selfLayer.name) isExists = true;
                                layerMap[id] = selfID;
                        }

                        if (isExists) continue;

                        int nextID = map_next_id_get(self->layers);
                        self->layer[nextID] = layer;
                        layerMap[id] = nextID;
                }

                std::unordered_map<int, int> nullMap;
                for (auto& [id, null] : anm2.nulls)
                {
                        bool isExists = false;

                        for (auto& [selfID, selfNull] : self->nulls)
                        {
                                if (null.name == selfNull.name) isExists = true;
                                nullMap[id] = selfID;
                        }

                        if (isExists) continue;

                        int nextID = map_next_id_get(self->nulls);
                        self->null[nextID] = null;
                        nullMap[id] = nextID;
                }

                std::unordered_map<int, int> eventMap;
                for (auto& [id, event] : anm2.events)
                {
                        bool isExists = false;

                        for (auto& [selfID, selfEvent] : self->events)
                        {
                                if (event.name == selfEvent.name) isExists = true;
                                eventMap[id] = selfID;
                        }

                        if (isExists) continue;

                        int nextID = map_next_id_get(self->events);
                        self->event[nextID] = event;
                        eventMap[id] = nextID;
                }

                for (auto& [id, animation] : anm2.animations)
                {
                        bool isExists = false;

                        for (auto& [selfID, selfAnimation] : self->animations)
                        {
                                if (event.name == selfAnimation.name) isExists = true;
                                eventMap[id] = selfID;
                        }

                        if (isExists) continue;

                        for (auto& frame : animation.rootAnimation.frames)
                        {


                        }

                        for (auto& [layerID, layerAnimation] : animation.layerAnimations)
                        {
                                int newLayerID = layerMap[layerID];


                        }
                }
        }
}
*/