#pragma once

#include <array>
#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include <glm/glm/vec2.hpp>
#include <glm/glm/vec3.hpp>
#include <glm/glm/vec4.hpp>

#include "audio.hpp"
#include "texture.hpp"

namespace tinyxml2
{
  class XMLDocument;
  class XMLElement;
}

namespace anm2ed::anm2_new
{
  class Anm2
  {
  public:
    enum Compatibility
    {
      ISAAC,
      ANM2ED,
      ANM2ED_LIMITED,
      COMPATIBILITY_COUNT
    };

    enum Flag
    {
      NO_SOUNDS = 1 << 0,
      NO_REGIONS = 1 << 1,
      FRAME_NO_REGION_VALUES = 1 << 2,
      INTERPOLATION_BOOL_ONLY = 1 << 3
    };

    using Flags = int;

    inline static constexpr std::array<Flags, COMPATIBILITY_COUNT> COMPATIBILITY_FLAGS = {{
        NO_SOUNDS | NO_REGIONS | FRAME_NO_REGION_VALUES | INTERPOLATION_BOOL_ONLY,
        0,
        FRAME_NO_REGION_VALUES,
    }};

    class Element
    {
    public:
      enum Source
      {
        STRING
      };

      enum Type
      {
        UNKNOWN,
        ACTOR,
        INFO,
        CONTENT,
        SPRITESHEETS,
        SPRITESHEET,
        REGION,
        LAYERS,
        LAYER,
        NULLS,
        NULL_ELEMENT,
        EVENTS,
        EVENT,
        SOUNDS,
        SOUND,
        ANIMATIONS,
        ANIMATION,
        ROOT_ANIMATION,
        LAYER_ANIMATIONS,
        LAYER_ANIMATION,
        NULL_ANIMATIONS,
        NULL_ANIMATION,
        TRIGGERS,
        TRIGGER,
        FRAME,
        TYPE_COUNT
      };

      enum Origin
      {
        TOP_LEFT,
        ORIGIN_CENTER,
        CUSTOM
      };

      enum Interpolation
      {
        NONE,
        LINEAR,
        EASE_IN,
        EASE_OUT,
        EASE_IN_OUT
      };

      inline static constexpr std::array<std::pair<Type, std::string_view>, TYPE_COUNT - 1> TYPE_TAGS = {{
          {ACTOR, "AnimatedActor"},
          {INFO, "Info"},
          {CONTENT, "Content"},
          {SPRITESHEETS, "Spritesheets"},
          {SPRITESHEET, "Spritesheet"},
          {REGION, "Region"},
          {LAYERS, "Layers"},
          {LAYER, "Layer"},
          {NULLS, "Nulls"},
          {NULL_ELEMENT, "Null"},
          {EVENTS, "Events"},
          {EVENT, "Event"},
          {SOUNDS, "Sounds"},
          {SOUND, "Sound"},
          {ANIMATIONS, "Animations"},
          {ANIMATION, "Animation"},
          {ROOT_ANIMATION, "RootAnimation"},
          {LAYER_ANIMATIONS, "LayerAnimations"},
          {LAYER_ANIMATION, "LayerAnimation"},
          {NULL_ANIMATIONS, "NullAnimations"},
          {NULL_ANIMATION, "NullAnimation"},
          {TRIGGERS, "Triggers"},
          {TRIGGER, "Trigger"},
          {FRAME, "Frame"},
      }};

      inline static constexpr std::array<std::pair<Origin, std::string_view>, 2> ORIGIN_STRINGS = {{
          {TOP_LEFT, "TopLeft"},
          {ORIGIN_CENTER, "Center"},
      }};

      inline static constexpr std::array<std::pair<Interpolation, std::string_view>, 3> INTERPOLATION_STRINGS = {{
          {EASE_IN, "EaseIn"},
          {EASE_OUT, "EaseOut"},
          {EASE_IN_OUT, "EaseInOut"},
      }};

      Type type{UNKNOWN};
      std::vector<Element> children{};

      std::string createdBy{};
      std::string createdOn{};
      std::string defaultAnimation{};
      std::string name{};
      std::filesystem::path path{};

      int id{-1};
      int layerId{-1};
      int nullId{-1};
      int spritesheetId{-1};
      int eventId{-1};
      int regionId{-1};
      int atFrame{-1};
      int frameNum{};
      int fps{30};
      int version{};
      int delay{1};

      glm::vec2 crop{};
      glm::vec2 size{};
      glm::vec2 pivot{};
      glm::vec2 position{};
      glm::vec2 scale{100.0f, 100.0f};
      glm::vec4 tint{1.0f, 1.0f, 1.0f, 1.0f};
      glm::vec3 colorOffset{};
      float rotation{};

      bool isVisible{true};
      bool isShowRect{false};
      bool isLoop{false};
      Origin origin{CUSTOM};
      Interpolation interpolation{NONE};

      Element() = default;
      explicit Element(const std::filesystem::path&, std::string* = nullptr);
      Element(std::string_view, Source, std::string* = nullptr);

      bool serialize(const std::filesystem::path&, std::string* = nullptr, Compatibility = ANM2ED) const;
      std::string to_string(Compatibility = ANM2ED) const;

      Element* child_get(Type, std::size_t index = 0);
      const Element* child_get(Type, std::size_t index = 0) const;
      std::vector<Element*> children_get(Type);
      std::vector<const Element*> children_get(Type) const;

      static std::string_view tag_get(Type);
      static Type type_get(std::string_view, Type = UNKNOWN);
      static Element from_xml(const tinyxml2::XMLElement*, Type = UNKNOWN);
      static bool document_load(tinyxml2::XMLDocument&, const std::filesystem::path&, std::string*);
      static bool document_parse(tinyxml2::XMLDocument&, std::string_view, std::string*);

      tinyxml2::XMLElement* to_xml(tinyxml2::XMLDocument&, Type = UNKNOWN, Flags = 0) const;

    private:
      void attributes_read(const tinyxml2::XMLElement*, Type = UNKNOWN);
      void attributes_write(tinyxml2::XMLElement*, Type = UNKNOWN, Flags = 0) const;
    };

    bool isValid{true};
    std::filesystem::path path{};
    Element root{};
    std::unordered_map<int, resource::Texture> spritesheets{};
    std::unordered_map<int, resource::Audio> sounds{};

    Anm2() = default;
    explicit Anm2(const std::filesystem::path&, std::string* = nullptr);

    bool serialize(const std::filesystem::path&, std::string* = nullptr, Compatibility = ANM2ED) const;
    std::string to_string(Compatibility = ANM2ED) const;
    void assets_reload();

    static constexpr Flags flags_get(Compatibility compatibility)
    {
      return COMPATIBILITY_FLAGS[static_cast<std::size_t>(compatibility)];
    }

    Element* element_get(Element::Type, std::size_t index = 0);
    const Element* element_get(Element::Type, std::size_t index = 0) const;
    std::vector<Element*> elements_get(Element::Type);
    std::vector<const Element*> elements_get(Element::Type) const;

  private:
    static void elements_collect(Element&, Element::Type, std::vector<Element*>&);
    static void elements_collect(const Element&, Element::Type, std::vector<const Element*>&);
  };
}
