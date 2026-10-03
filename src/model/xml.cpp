#include "xml.hpp"

#include <map>

#include "anm2/internal.hpp"

using namespace tinyxml2;

namespace anm2ed::model
{
  constexpr std::size_t XML_NODE_BUFFER_SIZE = 64;

  // Records an emitted element as an XmlNode, attribute values formatted exactly as the XML writer formats them.
  struct XmlNodeSink : ElementSink
  {
    std::vector<XmlNode*> stack{};
    XmlNode root{};

    void open(std::string_view tag) override
    {
      auto& node = stack.empty() ? root : stack.back()->children.emplace_back();
      node.tag = std::string(tag);
      stack.push_back(&node);
    }

    template <class T> void formatted_add(const char* name, T value)
    {
      char buffer[XML_NODE_BUFFER_SIZE]{};
      XMLUtil::ToStr(value, buffer, sizeof(buffer));
      attribute(name, (const char*)buffer);
    }

    void attribute(const char* name, const char* value) override { stack.back()->attributes.emplace_back(name, value); }
    void attribute(const char* name, int value) override { formatted_add(name, value); }
    void attribute(const char* name, bool value) override { formatted_add(name, value); }
    void attribute(const char* name, float value) override { formatted_add(name, value); }
    void text(const char* value) override { stack.back()->text = value; }
    void close() override { stack.pop_back(); }
  };

  XmlNode node_get(const Element& element, ElementType parentType)
  {
    XmlNodeSink sink{};
    element_emit(sink, element, parentType, SERIALIZE_ANM2ED_DEFAULT);
    return sink.root;
  }

  Element node_element_get(const XmlNode& node)
  {
    Element element{};
    element.tag = node.tag;
    element.extraAttributes = node.attributes;
    element.text = node.text;
    for (const auto& child : node.children)
      element.children.push_back(node_element_get(child));
    return element;
  }

  Extras extras_read(const Element& element) { return {element.extraAttributes, element.text, {}}; }

  void extras_write(Element& element, const Extras& extras)
  {
    element.extraAttributes = extras.attributes;
    element.text = extras.text;
    for (const auto& [index, node] : extras.children)
      element.children.insert(element.children.begin() + std::min(index, (int)element.children.size()),
                              node_element_get(node));
  }

  // Reads children: `known` returns false for a child it does not take, which is kept as unknown XML.
  template <class Known> void children_read(const Element& element, Extras& extras, Known&& known)
  {
    for (int i = 0; i < (int)element.children.size(); ++i)
      if (!known(element.children[i])) extras.children.emplace_back(i, node_get(element.children[i], element.type));
  }

  Element element_make_with(ElementType type, const Extras& extras)
  {
    auto element = element_make(type);
    extras_write(element, extras);
    return element;
  }

  Frame frame_read(const Element& element)
  {
    Frame frame{};
    frame.crop = element.crop;
    frame.size = element.size;
    frame.pivot = element.pivot;
    frame.position = element.position;
    frame.scale = element.scale;
    frame.shear = element.shear;
    frame.rotation = element.rotation;
    frame.tint = element.tint;
    frame.colorOffset = element.colorOffset;
    frame.duration = element.duration;
    frame.isVisible = element.isVisible;
    frame.interpolation = element.interpolation;
    frame.regionId = element.regionId;
    frame.shaderId = element.shaderId;
    frame.atFrame = element.atFrame;
    frame.eventId = element.eventId;
    frame.soundIds = element.soundIds;
    frame.extras = extras_read(element);
    return frame;
  }

  Element frame_write(const Frame& frame, ItemType type)
  {
    auto element = element_make(type == ItemType::TRIGGER ? ElementType::TRIGGER : ElementType::FRAME);
    element.crop = frame.crop;
    element.size = frame.size;
    element.pivot = frame.pivot;
    element.position = frame.position;
    element.scale = frame.scale;
    element.shear = frame.shear;
    element.rotation = frame.rotation;
    element.tint = frame.tint;
    element.colorOffset = frame.colorOffset;
    element.duration = frame.duration;
    element.isVisible = frame.isVisible;
    element.interpolation = frame.interpolation;
    element.regionId = frame.regionId;
    element.shaderId = frame.shaderId;
    element.atFrame = frame.atFrame;
    element.eventId = frame.eventId;
    element.soundIds = frame.soundIds;
    extras_write(element, frame.extras);
    return element;
  }

  ElementType track_element_type_get(ItemType type)
  {
    switch (type)
    {
      case ItemType::ROOT:
        return ElementType::ROOT_ANIMATION;
      case ItemType::NULL_:
        return ElementType::NULL_ANIMATION;
      case ItemType::TRIGGER:
        return ElementType::TRIGGERS;
      default:
        return ElementType::LAYER_ANIMATION;
    }
  }

  Track track_read(const Element& element, ItemType type)
  {
    Track track{.type = type, .id = track_id_get(element), .isVisible = element.isVisible};
    auto frameType = track_frame_type_get(element);
    children_read(element, track.extras,
                  [&](const Element& child)
                  {
                    if (child.type != frameType) return false;
                    track.frames.push_back(frame_read(child));
                    return true;
                  });
    track.extras.attributes = element.extraAttributes;
    track.extras.text = element.text;
    return track;
  }

  Element track_write(const Track& track, int groupId)
  {
    auto element = element_make(track_element_type_get(track.type));
    if (track.type == ItemType::LAYER) element.layerId = track.id;
    if (track.type == ItemType::NULL_) element.nullId = track.id;
    element.isVisible = track.isVisible;
    element.groupId = groupId;
    for (const auto& frame : track.frames)
      element.children.push_back(frame_write(frame, track.type));
    extras_write(element, track.extras);
    return element;
  }

  // In memory the anm2 tracks are flat: a group element is followed (somewhere) by tracks carrying its id.
  std::vector<TrackEntry> track_entries_read(const Element& container, ItemType type, Extras& extras)
  {
    std::vector<TrackEntry> entries{};
    std::map<int, std::size_t> groupEntries{};
    auto trackType = track_element_type_get(type);
    children_read(container, extras,
                  [&](const Element& child)
                  {
                    if (child.type == ElementType::GROUP)
                    {
                      TrackGroup group{.id = child.id,
                                       .name = child.name,
                                       .isExpanded = child.isExpanded,
                                       .isVisible = child.isVisible,
                                       .extras = extras_read(child)};
                      if (auto root = child_first_get(child, ElementType::ROOT_ANIMATION))
                        group.root = track_read(*root, ItemType::ROOT);
                      groupEntries[child.id] = entries.size();
                      entries.emplace_back(std::move(group));
                      return true;
                    }
                    if (child.type != trackType) return false;
                    auto track = track_read(child, type);
                    if (auto group = groupEntries.find(child.groupId); group != groupEntries.end())
                      std::get<TrackGroup>(entries[group->second]).tracks.push_back(std::move(track));
                    else
                      entries.emplace_back(std::move(track));
                    return true;
                  });
    extras.attributes = container.extraAttributes;
    extras.text = container.text;
    return entries;
  }

  Element track_entries_write(const std::vector<TrackEntry>& entries, ElementType containerType, const Extras& extras)
  {
    auto container = element_make(containerType);
    for (const auto& entry : entries)
      if (auto track = std::get_if<Track>(&entry))
        container.children.push_back(track_write(*track, -1));
      else
      {
        const auto& group = std::get<TrackGroup>(entry);
        auto element = element_make(ElementType::GROUP);
        element.id = group.id;
        element.name = group.name;
        element.isExpanded = group.isExpanded;
        element.isVisible = group.isVisible;
        element.children.push_back(track_write(group.root, -1));
        extras_write(element, group.extras);
        container.children.push_back(std::move(element));
        for (const auto& groupTrack : group.tracks)
          container.children.push_back(track_write(groupTrack, group.id));
      }
    extras_write(container, extras);
    return container;
  }

  Animation animation_read(const Element& element)
  {
    Animation animation{.name = element.name,
                        .frameNum = element.frameNum,
                        .isLoop = element.isLoop,
                        .root = {.type = ItemType::ROOT},
                        .layout = {}};
    std::set<ElementType> read{};
    children_read(element, animation.extras,
                  [&](const Element& child)
                  {
                    if (read.contains(child.type)) return false;
                    switch (child.type)
                    {
                      case ElementType::ROOT_ANIMATION:
                        animation.root = track_read(child, ItemType::ROOT);
                        break;
                      case ElementType::LAYER_ANIMATIONS:
                        animation.layers = track_entries_read(child, ItemType::LAYER, animation.layersExtras);
                        break;
                      case ElementType::NULL_ANIMATIONS:
                        animation.nulls = track_entries_read(child, ItemType::NULL_, animation.nullsExtras);
                        break;
                      case ElementType::TRIGGERS:
                        animation.triggers = track_read(child, ItemType::TRIGGER);
                        break;
                      default:
                        return false;
                    }
                    read.insert(child.type);
                    animation.layout.push_back(child.type);
                    return true;
                  });
    animation.extras.attributes = element.extraAttributes;
    animation.extras.text = element.text;
    return animation;
  }

  // Known children in their recorded order; a part with content that was never in the file is appended.
  template <class Write>
  void layout_write(Element& element, const std::vector<ElementType>& layout,
                    const std::vector<std::pair<ElementType, bool>>& parts, Write&& write)
  {
    for (auto type : layout)
      element.children.push_back(write(type));
    for (auto [type, isContent] : parts)
      if (isContent && !std::ranges::contains(layout, type)) element.children.push_back(write(type));
  }

  Element animation_write(const Animation& animation, int groupId)
  {
    auto element = element_make(ElementType::ANIMATION);
    element.name = animation.name;
    element.frameNum = animation.frameNum;
    element.isLoop = animation.isLoop;
    element.groupId = groupId;
    layout_write(element, animation.layout,
                 {{ElementType::ROOT_ANIMATION, !animation.root.frames.empty()},
                  {ElementType::LAYER_ANIMATIONS, !animation.layers.empty()},
                  {ElementType::NULL_ANIMATIONS, !animation.nulls.empty()},
                  {ElementType::TRIGGERS, !animation.triggers.frames.empty()}},
                 [&](ElementType type)
                 {
                   switch (type)
                   {
                     case ElementType::ROOT_ANIMATION:
                       return track_write(animation.root, -1);
                     case ElementType::LAYER_ANIMATIONS:
                       return track_entries_write(animation.layers, type, animation.layersExtras);
                     case ElementType::NULL_ANIMATIONS:
                       return track_entries_write(animation.nulls, type, animation.nullsExtras);
                     default:
                       return track_write(animation.triggers, -1);
                   }
                 });
    extras_write(element, animation.extras);
    return element;
  }

  AnimationGroup animation_group_read(const Element& element)
  {
    return {.id = element.id,
            .name = element.name,
            .isExpanded = element.isExpanded,
            .isVisible = element.isVisible,
            .extras = extras_read(element)};
  }

  Element animation_group_write(const AnimationGroup& group)
  {
    auto element = element_make(ElementType::GROUP);
    element.id = group.id;
    element.name = group.name;
    element.isExpanded = group.isExpanded;
    element.isVisible = group.isVisible;
    extras_write(element, group.extras);
    return element;
  }

  Animations animations_read(const Element& element)
  {
    Animations animations{.defaultAnimation = element.defaultAnimation};
    std::map<int, std::size_t> groupEntries{};
    children_read(element, animations.extras,
                  [&](const Element& child)
                  {
                    if (child.type == ElementType::GROUP)
                    {
                      groupEntries[child.id] = animations.entries.size();
                      animations.entries.emplace_back(animation_group_read(child));
                      return true;
                    }
                    if (child.type != ElementType::ANIMATION) return false;
                    auto animation = std::make_shared<const Animation>(animation_read(child));
                    if (auto group = groupEntries.find(child.groupId); group != groupEntries.end())
                      std::get<AnimationGroup>(animations.entries[group->second]).animations.push_back(animation);
                    else
                      animations.entries.emplace_back(animation);
                    return true;
                  });
    animations.extras.attributes = element.extraAttributes;
    animations.extras.text = element.text;
    return animations;
  }

  Element animations_write(const Animations& animations)
  {
    auto element = element_make(ElementType::ANIMATIONS);
    element.defaultAnimation = animations.defaultAnimation;
    for (const auto& entry : animations.entries)
      if (auto animation = std::get_if<std::shared_ptr<const Animation>>(&entry))
        element.children.push_back(animation_write(**animation, -1));
      else
      {
        const auto& group = std::get<AnimationGroup>(entry);
        element.children.push_back(animation_group_write(group));
        for (const auto& groupAnimation : group.animations)
          element.children.push_back(animation_write(*groupAnimation, group.id));
      }
    extras_write(element, animations.extras);
    return element;
  }

  Region region_read(const Element& element)
  {
    return {.id = element.id,
            .name = element.name,
            .crop = element.crop,
            .size = element.size,
            .pivot = element.pivot,
            .origin = element.origin,
            .extras = extras_read(element)};
  }

  Element item_write(const Region& region)
  {
    auto element = element_make(ElementType::REGION);
    element.id = region.id;
    element.name = region.name;
    element.crop = region.crop;
    element.size = region.size;
    element.pivot = region.pivot;
    element.origin = region.origin;
    extras_write(element, region.extras);
    return element;
  }

  Spritesheet spritesheet_read(const Element& element)
  {
    Spritesheet spritesheet{.id = element.id, .path = element.path};
    children_read(element, spritesheet.extras,
                  [&](const Element& child)
                  {
                    if (child.type != ElementType::REGION) return false;
                    spritesheet.regions.push_back(region_read(child));
                    return true;
                  });
    spritesheet.extras.attributes = element.extraAttributes;
    spritesheet.extras.text = element.text;
    return spritesheet;
  }

  Element item_write(const Spritesheet& spritesheet)
  {
    auto element = element_make(ElementType::SPRITESHEET);
    element.id = spritesheet.id;
    element.path = spritesheet.path;
    for (const auto& region : spritesheet.regions)
      element.children.push_back(item_write(region));
    extras_write(element, spritesheet.extras);
    return element;
  }

  Component component_read(const Element& element)
  {
    return {.index = element.index, .binding = element.binding, .value = element.value, .extras = extras_read(element)};
  }

  Uniform uniform_read(const Element& element)
  {
    Uniform uniform{.name = element.name, .binding = element.binding, .value = element.value};
    children_read(element, uniform.extras,
                  [&](const Element& child)
                  {
                    if (child.type != ElementType::COMPONENT) return false;
                    uniform.components.push_back(component_read(child));
                    return true;
                  });
    uniform.extras.attributes = element.extraAttributes;
    uniform.extras.text = element.text;
    return uniform;
  }

  Shader shader_read(const Element& element)
  {
    Shader shader{.id = element.id, .name = element.name, .vertex = element.vertex, .fragment = element.fragment};
    children_read(element, shader.extras,
                  [&](const Element& child)
                  {
                    if (child.type != ElementType::UNIFORM) return false;
                    shader.uniforms.push_back(uniform_read(child));
                    return true;
                  });
    shader.extras.attributes = element.extraAttributes;
    shader.extras.text = element.text;
    return shader;
  }

  Element item_write(const Shader& shader)
  {
    auto element = element_make(ElementType::SHADER);
    element.id = shader.id;
    element.name = shader.name;
    element.vertex = shader.vertex;
    element.fragment = shader.fragment;
    for (const auto& uniform : shader.uniforms)
    {
      auto& uniformElement = element.children.emplace_back(element_make(ElementType::UNIFORM));
      uniformElement.name = uniform.name;
      uniformElement.binding = uniform.binding;
      uniformElement.value = uniform.value;
      for (const auto& component : uniform.components)
      {
        auto& componentElement = uniformElement.children.emplace_back(element_make(ElementType::COMPONENT));
        componentElement.index = component.index;
        componentElement.binding = component.binding;
        componentElement.value = component.value;
        extras_write(componentElement, component.extras);
      }
      extras_write(uniformElement, uniform.extras);
    }
    extras_write(element, shader.extras);
    return element;
  }

  Layer layer_read(const Element& element)
  {
    return {
        .id = element.id, .name = element.name, .spritesheetId = element.spritesheetId, .extras = extras_read(element)};
  }

  Element item_write(const Layer& layer)
  {
    auto element = element_make(ElementType::LAYER_ELEMENT);
    element.id = layer.id;
    element.name = layer.name;
    element.spritesheetId = layer.spritesheetId;
    extras_write(element, layer.extras);
    return element;
  }

  Null null_read(const Element& element)
  {
    return {.id = element.id, .name = element.name, .isShowRect = element.isShowRect, .extras = extras_read(element)};
  }

  Element item_write(const Null& null)
  {
    auto element = element_make(ElementType::NULL_ELEMENT);
    element.id = null.id;
    element.name = null.name;
    element.isShowRect = null.isShowRect;
    extras_write(element, null.extras);
    return element;
  }

  Event event_read(const Element& element)
  {
    return {.id = element.id, .name = element.name, .extras = extras_read(element)};
  }

  Element item_write(const Event& event)
  {
    auto element = element_make(ElementType::EVENT_ELEMENT);
    element.id = event.id;
    element.name = event.name;
    extras_write(element, event.extras);
    return element;
  }

  Sound sound_read(const Element& element)
  {
    return {.id = element.id, .path = element.path, .extras = extras_read(element)};
  }

  Element item_write(const Sound& sound)
  {
    auto element = element_make(ElementType::SOUND_ELEMENT);
    element.id = sound.id;
    element.path = sound.path;
    extras_write(element, sound.extras);
    return element;
  }

  template <class Item> Item item_read(const Element&);
  template <> Spritesheet item_read(const Element& element) { return spritesheet_read(element); }
  template <> Region item_read(const Element& element) { return region_read(element); }
  template <> Shader item_read(const Element& element) { return shader_read(element); }
  template <> Layer item_read(const Element& element) { return layer_read(element); }
  template <> Null item_read(const Element& element) { return null_read(element); }
  template <> Event item_read(const Element& element) { return event_read(element); }
  template <> Sound item_read(const Element& element) { return sound_read(element); }

  template <class Item> constexpr ElementType ITEM_TYPE{};
  template <> constexpr ElementType ITEM_TYPE<Spritesheet>{ElementType::SPRITESHEET};
  template <> constexpr ElementType ITEM_TYPE<Region>{ElementType::REGION};
  template <> constexpr ElementType ITEM_TYPE<Shader>{ElementType::SHADER};
  template <> constexpr ElementType ITEM_TYPE<Layer>{ElementType::LAYER_ELEMENT};
  template <> constexpr ElementType ITEM_TYPE<Null>{ElementType::NULL_ELEMENT};
  template <> constexpr ElementType ITEM_TYPE<Event>{ElementType::EVENT_ELEMENT};
  template <> constexpr ElementType ITEM_TYPE<Sound>{ElementType::SOUND_ELEMENT};

  template <class Item> std::vector<Item> items_read(const Element& container, Content& content)
  {
    std::vector<Item> items{};
    Extras extras{extras_read(container)};
    children_read(container, extras,
                  [&](const Element& child)
                  {
                    if (child.type != ITEM_TYPE<Item>) return false;
                    items.push_back(item_read<Item>(child));
                    return true;
                  });
    if (extras != Extras{}) content.containerExtras.emplace_back(container.type, std::move(extras));
    return items;
  }

  template <class Item> Element items_write(const std::vector<Item>& items, ElementType type, const Content& content)
  {
    auto container = element_make(type);
    for (const auto& item : items)
      container.children.push_back(item_write(item));
    for (const auto& [extrasType, extras] : content.containerExtras)
      if (extrasType == type) extras_write(container, extras);
    return container;
  }

  Content content_read(const Element& element)
  {
    Content content{.layout = {}};
    std::set<ElementType> read{};
    children_read(element, content.extras,
                  [&](const Element& child)
                  {
                    if (read.contains(child.type)) return false;
                    switch (child.type)
                    {
                      case ElementType::SPRITESHEETS:
                        content.spritesheets = items_read<Spritesheet>(child, content);
                        break;
                      case ElementType::SHADERS:
                        content.shaders = items_read<Shader>(child, content);
                        break;
                      case ElementType::LAYERS:
                        content.layers = items_read<Layer>(child, content);
                        break;
                      case ElementType::NULLS:
                        content.nulls = items_read<Null>(child, content);
                        break;
                      case ElementType::EVENTS:
                        content.events = items_read<Event>(child, content);
                        break;
                      case ElementType::SOUNDS:
                        content.sounds = items_read<Sound>(child, content);
                        break;
                      default:
                        return false;
                    }
                    read.insert(child.type);
                    content.layout.push_back(child.type);
                    return true;
                  });
    content.extras.attributes = element.extraAttributes;
    content.extras.text = element.text;
    return content;
  }

  Element content_write(const Content& content)
  {
    auto element = element_make(ElementType::CONTENT);
    layout_write(element, content.layout,
                 {{ElementType::SPRITESHEETS, !content.spritesheets.empty()},
                  {ElementType::SHADERS, !content.shaders.empty()},
                  {ElementType::LAYERS, !content.layers.empty()},
                  {ElementType::NULLS, !content.nulls.empty()},
                  {ElementType::EVENTS, !content.events.empty()},
                  {ElementType::SOUNDS, !content.sounds.empty()}},
                 [&](ElementType type)
                 {
                   switch (type)
                   {
                     case ElementType::SPRITESHEETS:
                       return items_write(content.spritesheets, type, content);
                     case ElementType::SHADERS:
                       return items_write(content.shaders, type, content);
                     case ElementType::LAYERS:
                       return items_write(content.layers, type, content);
                     case ElementType::NULLS:
                       return items_write(content.nulls, type, content);
                     case ElementType::EVENTS:
                       return items_write(content.events, type, content);
                     default:
                       return items_write(content.sounds, type, content);
                   }
                 });
    extras_write(element, content.extras);
    return element;
  }

  Model model_read(const Element& root)
  {
    Model model{.layout = {}};
    std::set<ElementType> read{};
    children_read(root, model.extras,
                  [&](const Element& child)
                  {
                    if (read.contains(child.type)) return false;
                    switch (child.type)
                    {
                      case ElementType::INFO:
                        model.info = {child.createdBy, child.createdOn, child.fps, child.version, {}};
                        children_read(child, model.info.extras, [](const Element&) { return false; });
                        model.info.extras.attributes = child.extraAttributes;
                        model.info.extras.text = child.text;
                        break;
                      case ElementType::CONTENT:
                        model.content = content_read(child);
                        break;
                      case ElementType::ANIMATIONS:
                        model.animations = animations_read(child);
                        break;
                      default:
                        return false;
                    }
                    read.insert(child.type);
                    model.layout.push_back(child.type);
                    return true;
                  });
    model.extras.attributes = root.extraAttributes;
    model.extras.text = root.text;
    return model;
  }

  Element model_write(const Model& model)
  {
    auto root = element_make(ElementType::ANIMATED_ACTOR);
    layout_write(root, model.layout,
                 {{ElementType::INFO, model.info.fps != Info{}.fps || model.info.createdBy != Info{}.createdBy},
                  {ElementType::CONTENT, true},
                  {ElementType::ANIMATIONS, !model.animations.entries.empty()}},
                 [&](ElementType type)
                 {
                   if (type == ElementType::CONTENT) return content_write(model.content);
                   if (type == ElementType::ANIMATIONS) return animations_write(model.animations);
                   auto info = element_make(ElementType::INFO);
                   info.createdBy = model.info.createdBy;
                   info.createdOn = model.info.createdOn;
                   info.fps = model.info.fps;
                   info.version = model.info.version;
                   extras_write(info, model.info.extras);
                   return info;
                 });
    extras_write(root, model.extras);
    return root;
  }

  Anm2 anm2_get(const Model& model)
  {
    Anm2 anm2{};
    anm2.root = model_write(model);
    anm2.isValid = model.isValid;
    return anm2;
  }

  Model model_from_anm2(const Anm2& anm2)
  {
    auto model = model_read(anm2.root);
    model.isValid = anm2.isValid;
    return model;
  }

  Model model_make() { return model_from_anm2(Anm2()); }

  bool model_load(Model& model, const std::filesystem::path& path, std::string* errorString)
  {
    Anm2 anm2{};
    auto isLoaded = anm2.load(path, errorString);
    model = model_from_anm2(anm2);
    return isLoaded;
  }

  bool model_load_string(Model& model, std::string_view text, std::string* errorString)
  {
    Anm2 anm2{};
    auto isLoaded = anm2.load_string(text, errorString);
    model = model_from_anm2(anm2);
    return isLoaded;
  }

  bool model_save(const Model& model, const std::filesystem::path& path, std::string* errorString, Options options)
  {
    return anm2_get(model).save(path, errorString, options);
  }

  std::string model_to_string(const Model& model, Options options) { return anm2_get(model).to_string(options); }

  std::string model_serialize(const Model& model, Flags flags)
  {
    return element_to_string(anm2_get(model).normalized_for_serialize(flags).root, flags);
  }

  std::uint64_t model_hash(const Model& model, Options options) { return anm2_hash_get(model_write(model), options); }

  bool is_special_interpolated_frames(const Model& model)
  {
    for (auto [index, animation] : model.animations_get())
    {
      bool isSpecial{};
      animation_tracks_each(*animation,
                            [&](const Track& track)
                            {
                              for (const auto& frame : track.frames)
                                isSpecial |= track.type != ItemType::TRIGGER &&
                                             frame.interpolation != Interpolation::NONE &&
                                             frame.interpolation != Interpolation::LINEAR;
                            });
      if (isSpecial) return true;
    }
    return false;
  }

  std::string frame_to_string(const Frame& frame, ItemType type)
  {
    return element_to_string(frame_write(frame, type), track_element_type_get(type));
  }

  std::string animation_to_string(const Animation& animation, int groupId)
  {
    return element_to_string(animation_write(animation, groupId));
  }

  std::string animation_group_to_string(const AnimationGroup& group)
  {
    return element_to_string(animation_group_write(group));
  }

  bool document_parse(XMLDocument& document, const std::string& text, std::string* errorString)
  {
    if (document.Parse(text.c_str()) == XML_SUCCESS) return true;
    if (errorString) *errorString = document.ErrorStr();
    return false;
  }

  std::vector<Frame> frames_from_string(const std::string& text, ItemType type, std::string* errorString)
  {
    XMLDocument document{};
    if (!document_parse(document, text, errorString)) return {};
    auto tag = element_tag_get(type == ItemType::TRIGGER ? ElementType::TRIGGER : ElementType::FRAME).data();
    std::vector<Frame> frames{};
    for (auto element = document.FirstChildElement(tag); element; element = element->NextSiblingElement(tag))
      frames.push_back(frame_read(element_read(element)));
    if (frames.empty() && errorString) *errorString = std::format("No valid {}(s).", tag);
    return frames;
  }

  // Animations and animation groups in clipboard order; an animation keeps the group id it was copied with.
  std::vector<AnimationEntry> animations_from_string(const std::string& text, std::string* errorString)
  {
    XMLDocument document{};
    if (!document_parse(document, text, errorString)) return {};
    auto container = element_make(ElementType::ANIMATIONS);
    for (auto element = document.FirstChildElement(); element; element = element->NextSiblingElement())
    {
      auto item = element_read(element);
      if (item.type == ElementType::ANIMATION || item.type == ElementType::GROUP)
        container.children.push_back(std::move(item));
    }
    auto entries = animations_read(container).entries;
    if (entries.empty() && errorString) *errorString = "No valid animation(s).";
    return entries;
  }

  template <class Item> std::string item_to_string(const Item& item) { return element_to_string(item_write(item)); }

  template <class Item> std::vector<Item> items_from_string(const std::string& text, std::string* errorString)
  {
    XMLDocument document{};
    if (!document_parse(document, text, errorString)) return {};
    auto tag = element_tag_get(ITEM_TYPE<Item>).data();
    std::vector<Item> items{};
    for (auto element = document.FirstChildElement(tag); element; element = element->NextSiblingElement(tag))
    {
      auto item = item_read<Item>(element_read(element));
      if constexpr (requires { item.path; }) item.path = util::path::backslash_handle(item.path);
      items.push_back(std::move(item));
    }
    if (items.empty() && errorString) *errorString = std::format("No valid {}(s).", tag);
    return items;
  }

#define X(Item)                                                                                                        \
  template std::string item_to_string(const Item&);                                                                    \
  template std::vector<Item> items_from_string(const std::string&, std::string*);
  X(Spritesheet)
  X(Region)
  X(Shader)
  X(Layer)
  X(Null)
  X(Event)
  X(Sound)
#undef X
}
