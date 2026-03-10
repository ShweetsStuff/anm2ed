#include "anm2.hpp"

#include <cstring>
#include <format>
#include <tinyxml2/tinyxml2.h>
#include <utility>

#include "../log.hpp"
#include "../util/path_.hpp"
#include "../util/working_directory.hpp"
#include "../util/xml_.hpp"

using namespace tinyxml2;
using namespace anm2ed::util;

namespace anm2ed::resource::anm2_new
{
  Anm2::Item::Item(XMLElement* element, Type _type)
  {
    if (!element) return;

    type = _type;

    auto isFrameOwnerType =
        _type == ROOT_ANIMATION || _type == LAYER_ANIMATION || _type == NULL_ANIMATION || _type == TRIGGERS;
    auto isFrameLikeElement =
        element->Name() && (std::strcmp(element->Name(), "Frame") == 0 || std::strcmp(element->Name(), "Trigger") == 0);

    if (isFrameOwnerType && isFrameLikeElement)
    {
      ownerType = _type;
      type = _type == TRIGGERS ? TRIGGER : FRAME;

      switch (_type)
      {
        case ROOT_ANIMATION:
        case NULL_ANIMATION:
          element->QueryFloatAttribute("XPosition", &position.x);
          element->QueryFloatAttribute("YPosition", &position.y);
          element->QueryFloatAttribute("XScale", &scale.x);
          element->QueryFloatAttribute("YScale", &scale.y);
          element->QueryIntAttribute("Delay", &duration);
          element->QueryBoolAttribute("Visible", &isVisible);
          xml::query_color_attribute(element, "RedTint", tint.r);
          xml::query_color_attribute(element, "GreenTint", tint.g);
          xml::query_color_attribute(element, "BlueTint", tint.b);
          xml::query_color_attribute(element, "AlphaTint", tint.a);
          xml::query_color_attribute(element, "RedOffset", colorOffset.r);
          xml::query_color_attribute(element, "GreenOffset", colorOffset.g);
          xml::query_color_attribute(element, "BlueOffset", colorOffset.b);
          element->QueryFloatAttribute("Rotation", &rotation);
          element->QueryBoolAttribute("Interpolated", &isInterpolated);
          break;
        case LAYER_ANIMATION:
          element->QueryIntAttribute("RegionId", &regionID);
          element->QueryFloatAttribute("XPosition", &position.x);
          element->QueryFloatAttribute("YPosition", &position.y);
          element->QueryFloatAttribute("XPivot", &pivot.x);
          element->QueryFloatAttribute("YPivot", &pivot.y);
          element->QueryFloatAttribute("XCrop", &crop.x);
          element->QueryFloatAttribute("YCrop", &crop.y);
          element->QueryFloatAttribute("Width", &size.x);
          element->QueryFloatAttribute("Height", &size.y);
          element->QueryFloatAttribute("XScale", &scale.x);
          element->QueryFloatAttribute("YScale", &scale.y);
          element->QueryIntAttribute("Delay", &duration);
          element->QueryBoolAttribute("Visible", &isVisible);
          xml::query_color_attribute(element, "RedTint", tint.r);
          xml::query_color_attribute(element, "GreenTint", tint.g);
          xml::query_color_attribute(element, "BlueTint", tint.b);
          xml::query_color_attribute(element, "AlphaTint", tint.a);
          xml::query_color_attribute(element, "RedOffset", colorOffset.r);
          xml::query_color_attribute(element, "GreenOffset", colorOffset.g);
          xml::query_color_attribute(element, "BlueOffset", colorOffset.b);
          element->QueryFloatAttribute("Rotation", &rotation);
          element->QueryBoolAttribute("Interpolated", &isInterpolated);
          break;
        case TRIGGERS:
        {
          element->QueryIntAttribute("EventId", &eventID);
          element->QueryIntAttribute("AtFrame", &atFrame);
          int soundID{};
          if (element->QueryIntAttribute("SoundId", &soundID) == XML_SUCCESS) soundIDs.push_back(soundID);
          for (auto* child = element->FirstChildElement("Sound"); child; child = child->NextSiblingElement("Sound"))
            if (child->QueryIntAttribute("Id", &soundID) == XML_SUCCESS) soundIDs.push_back(soundID);
          break;
        }
        default:
          break;
      }

      return;
    }

    switch (_type)
    {
      case INFO:
        xml::query_string_attribute(element, "CreatedBy", &createdBy);
        xml::query_string_attribute(element, "CreatedOn", &createdOn);
        element->QueryIntAttribute("Fps", &fps);
        element->QueryIntAttribute("Version", &version);
        break;
      case SPRITESHEET:
        element->QueryIntAttribute("Id", &id);
        xml::query_path_attribute(element, "Path", &path);
        path = util::path::lower_case_backslash_handle(path);
        texture = Texture(path);
        break;
      case REGION:
      {
        element->QueryIntAttribute("Id", &id);
        xml::query_string_attribute(element, "Name", &name);
        element->QueryFloatAttribute("XCrop", &crop.x);
        element->QueryFloatAttribute("YCrop", &crop.y);
        element->QueryFloatAttribute("Width", &size.x);
        element->QueryFloatAttribute("Height", &size.y);

        auto* origin = element->Attribute("Origin");
        if (origin && std::string(origin) == "TopLeft")
          pivot = {};
        else if (origin && std::string(origin) == "Center")
          pivot = glm::vec2((int)(size.x / 2.0f), (int)(size.y / 2.0f));
        else
        {
          element->QueryFloatAttribute("XPivot", &pivot.x);
          element->QueryFloatAttribute("YPivot", &pivot.y);
        }
        break;
      }
      case LAYER:
        element->QueryIntAttribute("Id", &id);
        xml::query_string_attribute(element, "Name", &name);
        element->QueryIntAttribute("SpritesheetId", &spritesheetID);
        break;
      case NULL_:
        element->QueryIntAttribute("Id", &id);
        xml::query_string_attribute(element, "Name", &name);
        element->QueryBoolAttribute("ShowRect", &isShowRect);
        break;
      case EVENT:
        element->QueryIntAttribute("Id", &id);
        xml::query_string_attribute(element, "Name", &name);
        break;
      case SOUND:
        element->QueryIntAttribute("Id", &id);
        xml::query_path_attribute(element, "Path", &path);
        path = util::path::lower_case_backslash_handle(path);
        sound = Audio(path);
        break;
      case ANIMATION:
        xml::query_string_attribute(element, "Name", &name);
        element->QueryIntAttribute("FrameNum", &frameNum);
        element->QueryBoolAttribute("Loop", &isLoop);
        break;
      case ROOT_ANIMATION:
      case TRIGGERS:
        break;
      case LAYER_ANIMATION:
        element->QueryIntAttribute("LayerId", &itemID);
        element->QueryBoolAttribute("Visible", &isVisible);
        break;
      case NULL_ANIMATION:
        element->QueryIntAttribute("NullId", &itemID);
        element->QueryBoolAttribute("Visible", &isVisible);
        break;
      case FRAME:
      case TRIGGER:
      default:
        break;
    }
  }

  XMLElement* Anm2::Item::to_element(XMLDocument& document) const
  {
    switch (type)
    {
      case INFO:
      {
        auto* element = document.NewElement("Info");
        element->SetAttribute("CreatedBy", createdBy.c_str());
        element->SetAttribute("CreatedOn", createdOn.c_str());
        element->SetAttribute("Fps", fps);
        element->SetAttribute("Version", version);
        return element;
      }
      case SPRITESHEET:
      {
        auto* element = document.NewElement("Spritesheet");
        element->SetAttribute("Id", id);
        auto pathString = util::path::to_utf8(path);
        element->SetAttribute("Path", pathString.c_str());
        return element;
      }
      case REGION:
      {
        auto* element = document.NewElement("Region");
        element->SetAttribute("Id", id);
        element->SetAttribute("Name", name.c_str());
        element->SetAttribute("XCrop", crop.x);
        element->SetAttribute("YCrop", crop.y);
        element->SetAttribute("Width", size.x);
        element->SetAttribute("Height", size.y);
        element->SetAttribute("XPivot", pivot.x);
        element->SetAttribute("YPivot", pivot.y);
        return element;
      }
      case LAYER:
      {
        auto* element = document.NewElement("Layer");
        element->SetAttribute("Id", id);
        element->SetAttribute("Name", name.c_str());
        element->SetAttribute("SpritesheetId", spritesheetID);
        return element;
      }
      case NULL_:
      {
        auto* element = document.NewElement("Null");
        element->SetAttribute("Id", id);
        element->SetAttribute("Name", name.c_str());
        if (isShowRect) element->SetAttribute("ShowRect", isShowRect);
        return element;
      }
      case EVENT:
      {
        auto* element = document.NewElement("Event");
        element->SetAttribute("Id", id);
        element->SetAttribute("Name", name.c_str());
        return element;
      }
      case SOUND:
      {
        auto* element = document.NewElement("Sound");
        element->SetAttribute("Id", id);
        auto pathString = util::path::to_utf8(path);
        element->SetAttribute("Path", pathString.c_str());
        return element;
      }
      case ANIMATION:
      {
        auto* element = document.NewElement("Animation");
        element->SetAttribute("Name", name.c_str());
        element->SetAttribute("FrameNum", frameNum);
        element->SetAttribute("Loop", isLoop);
        return element;
      }
      case ROOT_ANIMATION:
        return document.NewElement("RootAnimation");
      case LAYER_ANIMATION:
      {
        auto* element = document.NewElement("LayerAnimation");
        element->SetAttribute("LayerId", itemID);
        element->SetAttribute("Visible", isVisible);
        return element;
      }
      case NULL_ANIMATION:
      {
        auto* element = document.NewElement("NullAnimation");
        element->SetAttribute("NullId", itemID);
        element->SetAttribute("Visible", isVisible);
        return element;
      }
      case TRIGGERS:
        return document.NewElement("Triggers");
      case TRIGGER:
      {
        auto* element = document.NewElement("Trigger");
        if (eventID != -1) element->SetAttribute("EventId", eventID);
        for (auto soundID : soundIDs)
        {
          if (soundID == -1) continue;
          auto* soundElement = element->InsertNewChildElement("Sound");
          soundElement->SetAttribute("Id", soundID);
        }
        element->SetAttribute("AtFrame", atFrame);
        return element;
      }
      case FRAME:
      {
        auto* element = document.NewElement("Frame");

        if (ownerType == LAYER_ANIMATION && regionID != -1) element->SetAttribute("RegionId", regionID);
        element->SetAttribute("XPosition", position.x);
        element->SetAttribute("YPosition", position.y);

        if (ownerType == LAYER_ANIMATION)
        {
          element->SetAttribute("XPivot", pivot.x);
          element->SetAttribute("YPivot", pivot.y);
          element->SetAttribute("XCrop", crop.x);
          element->SetAttribute("YCrop", crop.y);
          element->SetAttribute("Width", size.x);
          element->SetAttribute("Height", size.y);
        }

        element->SetAttribute("XScale", scale.x);
        element->SetAttribute("YScale", scale.y);
        element->SetAttribute("Delay", duration);
        element->SetAttribute("Visible", isVisible);
        element->SetAttribute("RedTint", (int)glm::clamp(tint.r * 255.0f, 0.0f, 255.0f));
        element->SetAttribute("GreenTint", (int)glm::clamp(tint.g * 255.0f, 0.0f, 255.0f));
        element->SetAttribute("BlueTint", (int)glm::clamp(tint.b * 255.0f, 0.0f, 255.0f));
        element->SetAttribute("AlphaTint", (int)glm::clamp(tint.a * 255.0f, 0.0f, 255.0f));
        element->SetAttribute("RedOffset", (int)glm::clamp(colorOffset.r * 255.0f, 0.0f, 255.0f));
        element->SetAttribute("GreenOffset", (int)glm::clamp(colorOffset.g * 255.0f, 0.0f, 255.0f));
        element->SetAttribute("BlueOffset", (int)glm::clamp(colorOffset.b * 255.0f, 0.0f, 255.0f));
        element->SetAttribute("Rotation", rotation);
        element->SetAttribute("Interpolated", isInterpolated);

        return element;
      }
      default:
        break;
    }

    return document.NewElement("Item");
  }

  std::string Anm2::Item::to_string() const
  {
    XMLDocument document;
    document.InsertEndChild(to_element(document));
    return xml::document_to_string(document);
  }

  Anm2::Anm2(const std::filesystem::path& path)
  {
    XMLDocument document;
    auto pathString = util::path::to_utf8(path);

    if (document.LoadFile(pathString.c_str()) != XML_SUCCESS)
    {
      logger.error(std::format("Failed to initialize anm2: {} ({})", pathString, document.ErrorStr()));
      isValid = false;
      return;
    }

    WorkingDirectory workingDirectory(path, WorkingDirectory::FILE);

    this->path = path;
    isValid = true;

    auto item_add = [&](Item item) { items.emplace_back(std::move(item)); };

    if (auto* root = document.RootElement())
    {
      if (auto* infoElement = root->FirstChildElement("Info")) item_add(Item(infoElement, INFO));

      if (auto* contentElement = root->FirstChildElement("Content"))
      {
        if (auto* spritesheetsElement = contentElement->FirstChildElement("Spritesheets"))
        {
          for (auto* child = spritesheetsElement->FirstChildElement("Spritesheet"); child;
               child = child->NextSiblingElement("Spritesheet"))
          {
            auto spritesheet = Item(child, SPRITESHEET);
            auto spritesheetID = spritesheet.id;
            item_add(spritesheet);

            for (auto* regionChild = child->FirstChildElement("Region"); regionChild;
                 regionChild = regionChild->NextSiblingElement("Region"))
            {
              auto region = Item(regionChild, REGION);
              region.itemID = spritesheetID;
              item_add(region);
            }
          }
        }

        if (auto* layersElement = contentElement->FirstChildElement("Layers"))
          for (auto* child = layersElement->FirstChildElement("Layer"); child;
               child = child->NextSiblingElement("Layer"))
            item_add(Item(child, LAYER));

        if (auto* nullsElement = contentElement->FirstChildElement("Nulls"))
          for (auto* child = nullsElement->FirstChildElement("Null"); child; child = child->NextSiblingElement("Null"))
            item_add(Item(child, NULL_));

        if (auto* eventsElement = contentElement->FirstChildElement("Events"))
          for (auto* child = eventsElement->FirstChildElement("Event"); child;
               child = child->NextSiblingElement("Event"))
            item_add(Item(child, EVENT));

        if (auto* soundsElement = contentElement->FirstChildElement("Sounds"))
          for (auto* child = soundsElement->FirstChildElement("Sound"); child;
               child = child->NextSiblingElement("Sound"))
            item_add(Item(child, SOUND));
      }

      if (auto* animationsElement = root->FirstChildElement("Animations"))
      {
        int animationIndex = 0;
        for (auto* animationElement = animationsElement->FirstChildElement("Animation"); animationElement;
             animationElement = animationElement->NextSiblingElement("Animation"))
        {
          auto animation = Item(animationElement, ANIMATION);
          animation.id = animationIndex++;
          item_add(animation);

          if (auto* rootAnimationElement = animationElement->FirstChildElement("RootAnimation"))
          {
            Item rootAnimation{};
            rootAnimation.type = ROOT_ANIMATION;
            rootAnimation.animationID = animation.id;
            item_add(rootAnimation);

            for (auto* frameElement = rootAnimationElement->FirstChildElement("Frame"); frameElement;
                 frameElement = frameElement->NextSiblingElement("Frame"))
            {
              auto frame = Item(frameElement, ROOT_ANIMATION);
              frame.animationID = animation.id;
              frame.ownerID = -1;
              item_add(frame);
            }
          }

          if (auto* layerAnimationsElement = animationElement->FirstChildElement("LayerAnimations"))
          {
            for (auto* layerAnimationElement = layerAnimationsElement->FirstChildElement("LayerAnimation");
                 layerAnimationElement;
                 layerAnimationElement = layerAnimationElement->NextSiblingElement("LayerAnimation"))
            {
              auto layerAnimation = Item(layerAnimationElement, LAYER_ANIMATION);
              layerAnimation.animationID = animation.id;
              item_add(layerAnimation);

              for (auto* frameElement = layerAnimationElement->FirstChildElement("Frame"); frameElement;
                   frameElement = frameElement->NextSiblingElement("Frame"))
              {
                auto frame = Item(frameElement, LAYER_ANIMATION);
                frame.animationID = animation.id;
                frame.ownerID = layerAnimation.itemID;
                item_add(frame);
              }
            }
          }

          if (auto* nullAnimationsElement = animationElement->FirstChildElement("NullAnimations"))
          {
            for (auto* nullAnimationElement = nullAnimationsElement->FirstChildElement("NullAnimation");
                 nullAnimationElement; nullAnimationElement = nullAnimationElement->NextSiblingElement("NullAnimation"))
            {
              auto nullAnimation = Item(nullAnimationElement, NULL_ANIMATION);
              nullAnimation.animationID = animation.id;
              item_add(nullAnimation);

              for (auto* frameElement = nullAnimationElement->FirstChildElement("Frame"); frameElement;
                   frameElement = frameElement->NextSiblingElement("Frame"))
              {
                auto frame = Item(frameElement, NULL_ANIMATION);
                frame.animationID = animation.id;
                frame.ownerID = nullAnimation.itemID;
                item_add(frame);
              }
            }
          }

          if (auto* triggersElement = animationElement->FirstChildElement("Triggers"))
          {
            Item triggers{};
            triggers.type = TRIGGERS;
            triggers.animationID = animation.id;
            item_add(triggers);

            for (auto* triggerElement = triggersElement->FirstChildElement("Trigger"); triggerElement;
                 triggerElement = triggerElement->NextSiblingElement("Trigger"))
            {
              auto trigger = Item(triggerElement, TRIGGERS);
              trigger.animationID = animation.id;
              trigger.ownerID = -1;
              item_add(trigger);
            }
          }
        }
      }
    }

    logger.info(std::format("Initialized anm2: {}", pathString));
  }

  XMLElement* Anm2::to_element(XMLDocument& document) const
  {
    auto* root = document.NewElement("AnimatedActor");
    auto add_frames = [&](XMLElement* parent, Type ownerType, int animationID, int ownerID)
    {
      for (auto& item : items)
      {
        if (item.type != FRAME && item.type != TRIGGER) continue;
        if (item.ownerType != ownerType) continue;
        if (item.animationID != animationID) continue;
        if (item.ownerID != ownerID) continue;
        parent->InsertEndChild(item.to_element(document));
      }
    };

    const Item* info = nullptr;
    for (auto& item : items)
      if (item.type == INFO)
      {
        info = &item;
        break;
      }
    if (info) root->InsertEndChild(info->to_element(document));

    auto* contentElement = document.NewElement("Content");
    auto* spritesheetsElement = document.NewElement("Spritesheets");
    auto* layersElement = document.NewElement("Layers");
    auto* nullsElement = document.NewElement("Nulls");
    auto* eventsElement = document.NewElement("Events");
    auto* soundsElement = document.NewElement("Sounds");

    for (auto& item : items)
    {
      if (item.type != SPRITESHEET) continue;
      auto* spritesheetElement = item.to_element(document);
      for (auto& region : items)
        if (region.type == REGION && region.itemID == item.id)
          spritesheetElement->InsertEndChild(region.to_element(document));
      spritesheetsElement->InsertEndChild(spritesheetElement);
    }

    for (auto& item : items)
      if (item.type == LAYER) layersElement->InsertEndChild(item.to_element(document));

    for (auto& item : items)
      if (item.type == NULL_) nullsElement->InsertEndChild(item.to_element(document));

    for (auto& item : items)
      if (item.type == EVENT) eventsElement->InsertEndChild(item.to_element(document));

    for (auto& item : items)
      if (item.type == SOUND) soundsElement->InsertEndChild(item.to_element(document));

    contentElement->InsertEndChild(spritesheetsElement);
    contentElement->InsertEndChild(layersElement);
    contentElement->InsertEndChild(nullsElement);
    contentElement->InsertEndChild(eventsElement);
    if (soundsElement->FirstChildElement("Sound")) contentElement->InsertEndChild(soundsElement);
    root->InsertEndChild(contentElement);

    auto* animationsElement = document.NewElement("Animations");

    for (auto& animation : items)
    {
      if (animation.type != ANIMATION) continue;
      auto* animationElement = animation.to_element(document);

      auto* rootAnimationElement = document.NewElement("RootAnimation");
      add_frames(rootAnimationElement, ROOT_ANIMATION, animation.id, -1);
      animationElement->InsertEndChild(rootAnimationElement);

      auto* layerAnimationsElement = document.NewElement("LayerAnimations");
      for (auto& layerAnimation : items)
      {
        if (layerAnimation.type != LAYER_ANIMATION || layerAnimation.animationID != animation.id) continue;
        auto* layerAnimationElement = layerAnimation.to_element(document);
        add_frames(layerAnimationElement, LAYER_ANIMATION, animation.id, layerAnimation.itemID);
        layerAnimationsElement->InsertEndChild(layerAnimationElement);
      }
      animationElement->InsertEndChild(layerAnimationsElement);

      auto* nullAnimationsElement = document.NewElement("NullAnimations");
      for (auto& nullAnimation : items)
      {
        if (nullAnimation.type != NULL_ANIMATION || nullAnimation.animationID != animation.id) continue;
        auto* nullAnimationElement = nullAnimation.to_element(document);
        add_frames(nullAnimationElement, NULL_ANIMATION, animation.id, nullAnimation.itemID);
        nullAnimationsElement->InsertEndChild(nullAnimationElement);
      }
      animationElement->InsertEndChild(nullAnimationsElement);

      auto* triggersElement = document.NewElement("Triggers");
      add_frames(triggersElement, TRIGGERS, animation.id, -1);
      animationElement->InsertEndChild(triggersElement);

      animationsElement->InsertEndChild(animationElement);
    }

    root->InsertEndChild(animationsElement);
    return root;
  }

  bool Anm2::serialize(const std::filesystem::path& path, std::string* errorString) const
  {
    XMLDocument document;
    document.InsertEndChild(to_element(document));

    auto pathString = util::path::to_utf8(path);
    if (document.SaveFile(pathString.c_str()) != XML_SUCCESS)
    {
      if (errorString) *errorString = document.ErrorStr();
      return false;
    }

    return true;
  }

  std::string Anm2::to_string() const
  {
    XMLDocument document;
    document.InsertEndChild(to_element(document));
    return xml::document_to_string(document);
  }
}
