#include "json.hpp"

#include <algorithm>
#include <charconv>
#include <cstdio>
#include <format>
#include <set>

#include <simdjson/singleheader/simdjson.h>

#include "math.hpp"
#include "path.hpp"
#include "xml.hpp"

using namespace simdjson;
using namespace anm2ed::util;

namespace anm2ed::model
{
  constexpr std::string_view VERSION_KEY = "anm2ed";
  constexpr int VERSION = 1;
  constexpr std::string_view INDENT = "  ";
  constexpr std::size_t LINE_WIDTH_MAX = 100;
  constexpr std::string_view WHITESPACE = " \t\r\n";
  constexpr std::size_t NUMBER_BUFFER_SIZE = 64;
  constexpr std::pair<char, std::string_view> ESCAPES[] = {
      {'"', "\\\""}, {'\\', "\\\\"}, {'\n', "\\n"}, {'\r', "\\r"}, {'\t', "\\t"}};
  constexpr std::string_view INTERPOLATION_NAMES[] = {"", "linear", "easeIn", "easeOut", "easeInOut"};
  constexpr std::string_view CHILD_MODE_NAMES[] = {"loop", "once", "hold"};
  constexpr std::string_view ORIGIN_NAMES[] = {"", "topLeft", "center"};
  constexpr std::size_t ISSUES_SHOWN_MAX = 5;
  constexpr int HOURS_HALF_DAY = 12;
  constexpr int DATE_FIELDS = 6;
  constexpr int DATE_MERIDIEM_SIZE = 3;
  const std::vector<Region> REGIONS_NONE{};

  bool is_json(std::string_view text)
  {
    auto start = text.find_first_not_of(WHITESPACE);
    return start != std::string_view::npos && text[start] == '{';
  }

  template <class Items> int index_get(const Items& items, int id)
  {
    auto it = std::ranges::find(items, id, &std::ranges::range_value_t<Items>::id);
    return it == items.end() ? -1 : (int)(it - items.begin());
  }

  glm::ivec4 color_to_bytes(const glm::vec4& color)
  {
    return {math::float_to_uint8(color.r), math::float_to_uint8(color.g), math::float_to_uint8(color.b),
            math::float_to_uint8(color.a)};
  }

  glm::vec4 color_from_bytes(const glm::ivec4& bytes)
  {
    return {math::uint8_to_float(bytes.r), math::uint8_to_float(bytes.g), math::uint8_to_float(bytes.b),
            math::uint8_to_float(bytes.a)};
  }

  // The editor keeps dates as "MM/DD/YYYY hh:mm:ss AM"; JSON has them as ISO 8601. Anything else is kept as it is.
  std::string date_to_iso(const std::string& date)
  {
    int month{}, day{}, year{}, hour{}, minute{}, second{};
    char meridiem[DATE_MERIDIEM_SIZE]{};
    if (std::sscanf(date.c_str(), "%d/%d/%d %d:%d:%d %2s", &month, &day, &year, &hour, &minute, &second, meridiem) !=
        DATE_FIELDS + 1)
      return date;
    hour = hour % HOURS_HALF_DAY + (std::string_view(meridiem) == "PM" ? HOURS_HALF_DAY : 0);
    return std::format("{:04}-{:02}-{:02}T{:02}:{:02}:{:02}", year, month, day, hour, minute, second);
  }

  std::string date_from_iso(const std::string& date)
  {
    int year{}, month{}, day{}, hour{}, minute{}, second{};
    if (std::sscanf(date.c_str(), "%d-%d-%dT%d:%d:%d", &year, &month, &day, &hour, &minute, &second) != DATE_FIELDS)
      return date;
    auto hour12 = hour % HOURS_HALF_DAY == 0 ? HOURS_HALF_DAY : hour % HOURS_HALF_DAY;
    return std::format("{:02}/{:02}/{:04} {:02}:{:02}:{:02} {}", month, day, year, hour12, minute, second,
                       hour < HOURS_HALF_DAY ? "AM" : "PM");
  }

  void line_break(std::string& text, int depth)
  {
    text += '\n';
    for (int i = 0; i < depth; ++i)
      text += INDENT;
  }

  // Writing: defaults are left out and ids become indices. Containers opened inline (and everything inside them) stay
  // on one line; any other that fits within LINE_WIDTH_MAX is joined onto one when it closes, unless it keeps its items
  // a line each (frames and content items).
  struct JsonWriter
  {
    static constexpr bool IS_READING = false;

    struct Scope
    {
      bool isInline{};
      bool isJoinable{true};
      std::size_t start{};
      bool isEmpty{true};
    };

    const Model& model;
    std::string text{};
    std::vector<Scope> scopes{};

    void separate()
    {
      if (scopes.empty()) return;
      auto& scope = scopes.back();
      if (!scope.isEmpty) text += scope.isInline ? ", " : ",";
      if (!scope.isInline) line_break(text, (int)scopes.size());
      scope.isEmpty = false;
    }

    void open(char bracket, bool isInline, bool isJoinable = true)
    {
      scopes.push_back({isInline || (!scopes.empty() && scopes.back().isInline), isJoinable, text.size()});
      text += bracket;
    }

    // Line breaks become nothing, or a space after a comma.
    void join(const Scope& scope)
    {
      std::string joined{};
      for (auto i = scope.start; i < text.size(); ++i)
        if (text[i] != '\n')
          joined += text[i];
        else
        {
          i = text.find_first_not_of(' ', i + 1) - 1;
          if (joined.back() == ',') joined += ' ';
        }
      auto column = scope.start - (text.rfind('\n', scope.start) + 1);
      if (column + joined.size() <= LINE_WIDTH_MAX) text.replace(scope.start, std::string::npos, joined);
    }

    void close(char bracket)
    {
      auto scope = scopes.back();
      scopes.pop_back();
      if (!scope.isEmpty && !scope.isInline) line_break(text, (int)scopes.size());
      text += bracket;
      if (!scope.isInline && scope.isJoinable) join(scope);
    }

    void key(std::string_view name)
    {
      separate();
      value(name);
      text += ": ";
    }

    template <class T> void number(T number)
    {
      char buffer[NUMBER_BUFFER_SIZE]{};
      text.append(buffer, std::to_chars(buffer, buffer + sizeof(buffer), number).ptr);
    }

    void value(bool value) { text += value ? "true" : "false"; }
    void value(int value) { number(value); }
    void value(float value) { number(value); }
    void value(const std::string& value) { this->value(std::string_view(value)); }
    void value(const std::filesystem::path& value) { this->value(path::to_utf8(value)); }

    void value(std::string_view value)
    {
      text += '"';
      for (auto character : value)
      {
        auto escape = std::ranges::find(ESCAPES, character, &std::pair<char, std::string_view>::first);
        if (escape != std::end(ESCAPES))
          text += escape->second;
        else if ((unsigned char)character < ' ')
          text += std::format("\\u{:04x}", (unsigned char)character);
        else
          text += character;
      }
      text += '"';
    }

    template <class Values> void array(const Values& values, int count)
    {
      open('[', true);
      for (int i = 0; i < count; ++i)
      {
        separate();
        value(values[i]);
      }
      close(']');
    }

    template <int L, class T, glm::qualifier Q> void value(const glm::vec<L, T, Q>& value) { array(value, L); }
    template <class T> void value(const std::vector<T>& value) { array(value, (int)value.size()); }

    template <class T> void field(std::string_view name, const T& value)
    {
      key(name);
      this->value(value);
    }

    template <class T> void field(std::string_view name, const T& value, const std::type_identity_t<T>& defaultValue)
    {
      if (value != defaultValue) field(name, value);
    }

    // Written as `to` makes it (and read back through `from`).
    template <class T, class To, class From>
    void field(std::string_view name, const T& value, const T& defaultValue, To&& to, From&&)
    {
      field(name, to(value), to(defaultValue));
    }

    template <class Items> int reference(std::string_view name, int id, const Items& items, int defaultIndex = -1)
    {
      auto index = index_get(items, id);
      field(name, index, defaultIndex);
      return index;
    }

    template <class Items> void references(std::string_view name, const std::vector<int>& ids, const Items& items)
    {
      std::vector<int> indices{};
      for (auto id : ids)
        if (auto index = index_get(items, id); index != -1) indices.push_back(index);
      field(name, indices, {});
    }

    template <class E, std::size_t N>
    void choice(std::string_view name, E value, const std::string_view (&names)[N], E defaultValue)
    {
      if (value != defaultValue) field(name, names[(int)value]);
    }

    template <class Fn> void object(std::string_view name, Fn&& fn, bool isInline)
    {
      key(name);
      open('{', isInline);
      fn(*this);
      close('}');
    }

    template <class T, class Fn>
    void list(std::string_view name, const std::vector<T>& items, Fn&& fn, bool isInline, bool isJoinable = true)
    {
      if (items.empty()) return;
      key(name);
      open('[', false, isJoinable);
      for (const auto& item : items)
      {
        separate();
        open('{', isInline);
        fn(*this, item);
        close('}');
      }
      close(']');
    }

    template <class T, class Get> void values(std::string_view name, const std::vector<T>& items, Get&& get)
    {
      if (items.empty()) return;
      key(name);
      open('[', false, false);
      for (const auto& item : items)
      {
        separate();
        value(get(item));
      }
      close(']');
    }
  };

  // Each returns what it expected when the value is the wrong kind (and leaves the value as it was).
  std::string_view value_read(dom::element element, bool& value)
  {
    bool read{};
    if (element.get(read)) return "true or false";
    value = read;
    return {};
  }

  template <class T>
    requires std::is_same_v<T, int> || std::is_same_v<T, float>
  std::string_view value_read(dom::element element, T& value)
  {
    double read{};
    if (element.get(read)) return "a number";
    value = (T)read;
    return {};
  }

  std::string_view value_read(dom::element element, std::string& value)
  {
    std::string_view read{};
    if (element.get(read)) return "a string";
    value = read;
    return {};
  }

  std::string_view value_read(dom::element element, std::filesystem::path& value)
  {
    std::string read{};
    auto expected = value_read(element, read);
    if (expected.empty()) value = path::from_utf8(read);
    return expected;
  }

  template <int L, class T, glm::qualifier Q>
  std::string_view value_read(dom::element element, glm::vec<L, T, Q>& value)
  {
    static const auto EXPECTED = std::format("an array of {} numbers", L);
    dom::array array{};
    if (element.get(array) || array.size() != L) return EXPECTED;
    auto read = value;
    int i{};
    for (auto item : array)
      if (!value_read(item, read[i++]).empty()) return EXPECTED;
    value = read;
    return {};
  }

  template <class T> std::string_view value_read(dom::element element, std::vector<T>& value)
  {
    dom::array array{};
    if (element.get(array)) return "an array";
    std::vector<T> read{};
    for (auto item : array)
      if (auto expected = value_read(item, read.emplace_back()); !expected.empty()) return expected;
    value = std::move(read);
    return {};
  }

  // Reading: only the keys present change the model, and an item's id is its index. Whatever can't be read (and any
  // key it doesn't know) is noted in `issues` by where it is, e.g. "animations.Idle.layers[0].frames[2].pos".
  struct JsonReader
  {
    static constexpr bool IS_READING = true;

    const Model& model;
    std::vector<std::string>* issues{};
    std::string path{};
    dom::object fields{};
    std::set<std::string_view> used{};
    bool isKeyed{}; // its keys are names rather than fields

    void issue_add(std::string_view name, std::string_view message)
    {
      issues->push_back(std::format("{}{}: {}", path, name, message));
    }

    bool element_get(std::string_view name, dom::element& element)
    {
      used.insert(name);
      return !fields[name].get(element);
    }

    bool has(std::string_view name)
    {
      dom::element element{};
      return element_get(name, element);
    }

    template <class T> bool field(std::string_view name, T& value, const std::type_identity_t<T>& = {})
    {
      dom::element element{};
      if (!element_get(name, element)) return false;
      if (auto expected = value_read(element, value); !expected.empty())
        issue_add(name, std::format("expected {}", expected));
      return true;
    }

    template <class T, class To, class From> void field(std::string_view name, T& value, const T&, To&& to, From&& from)
    {
      auto converted = to(value);
      if (field(name, converted)) value = from(converted);
    }

    template <class Items> bool is_index_valid(const Items& items, int index)
    {
      return index >= -1 && index < (int)items.size();
    }

    template <class Items> int reference(std::string_view name, int& id, const Items& items, int = -1)
    {
      if (field(name, id) && !is_index_valid(items, id)) issue_add(name, std::format("no item at index {}", id));
      return id;
    }

    template <class Items> void references(std::string_view name, std::vector<int>& ids, const Items& items)
    {
      if (!field(name, ids)) return;
      for (auto id : ids)
        if (!is_index_valid(items, id)) issue_add(name, std::format("no item at index {}", id));
    }

    template <class E, std::size_t N>
    void choice(std::string_view name, E& value, const std::string_view (&names)[N], E)
    {
      std::string read{};
      if (!field(name, read)) return;
      auto it = std::ranges::find(names, read);
      if (read.empty() || it == std::end(names))
        issue_add(name, std::format("unknown value \"{}\"", read));
      else
        value = (E)(it - std::begin(names));
    }

    void unknown_keys_check()
    {
      if (isKeyed) return;
      for (auto [key, value] : fields)
        if (!used.contains(key)) issue_add(key, "unknown key");
    }

    // Reads `element` as an object at `childPath` (ending in '.'), then notes the keys it never asked for.
    template <class Fn> void object_each(dom::element element, std::string childPath, Fn&& fn)
    {
      JsonReader reader{model, issues, std::move(childPath)};
      if (element.get(reader.fields))
      {
        issues->push_back(std::format("{}: expected an object", reader.path.substr(0, reader.path.size() - 1)));
        return;
      }
      fn(reader);
      reader.unknown_keys_check();
    }

    template <class Fn> void object(std::string_view name, Fn&& fn, bool)
    {
      dom::element element{};
      if (element_get(name, element)) object_each(element, std::format("{}{}.", path, name), fn);
    }

    bool array_get(std::string_view name, dom::array& array)
    {
      dom::element element{};
      if (!element_get(name, element)) return false;
      if (!element.get(array)) return true;
      issue_add(name, "expected an array");
      return false;
    }

    template <class T, class Fn> void list(std::string_view name, std::vector<T>& items, Fn&& fn, bool, bool = true)
    {
      dom::array array{};
      if (!array_get(name, array)) return;
      items.clear();
      for (auto element : array)
      {
        auto& item = items.emplace_back();
        auto index = (int)items.size() - 1;
        if constexpr (requires { item.id; }) item.id = index;
        object_each(element, std::format("{}{}[{}].", path, name, index),
                    [&](JsonReader& reader) { fn(reader, item); });
      }
    }

    template <class T, class Get> void values(std::string_view name, std::vector<T>& items, Get&& get)
    {
      dom::array array{};
      if (!array_get(name, array)) return;
      items.clear();
      for (auto element : array)
      {
        auto& item = items.emplace_back();
        item.id = (int)items.size() - 1;
        if (auto expected = value_read(element, get(item)); !expected.empty())
          issue_add(std::format("{}[{}]", name, item.id), std::format("expected {}", expected));
      }
    }
  };

  // Each part of the document is described once; with a JsonWriter it is written, with a JsonReader read.

  template <class Io, class F> void frame_io(Io& io, F& frame, ItemType type, const std::vector<Region>& regions)
  {
    static const Frame FRAME_DEFAULT{};
    auto& content = io.model.content;
    if (type == ItemType::TRIGGER)
    {
      io.field("at", frame.atFrame, FRAME_DEFAULT.atFrame);
      io.references("events", frame.eventIds, content.events);
      io.references("sounds", frame.soundIds, content.sounds);
      return;
    }
    // A frame on a region takes the region's rectangle.
    if (type == ItemType::LAYER && io.reference("region", frame.regionId, regions) == -1)
    {
      io.field("crop", frame.crop, FRAME_DEFAULT.crop);
      io.field("size", frame.size, FRAME_DEFAULT.size);
      io.field("pivot", frame.pivot, FRAME_DEFAULT.pivot);
    }
    io.field("pos", frame.position, FRAME_DEFAULT.position);
    io.field("scale", frame.scale, FRAME_DEFAULT.scale);
    io.field("skew", frame.skew, FRAME_DEFAULT.skew);
    io.field("rotation", frame.rotation, FRAME_DEFAULT.rotation);
    io.field("tint", frame.tint, FRAME_DEFAULT.tint, color_to_bytes, color_from_bytes);
    io.field("offset", frame.colorOffset, FRAME_DEFAULT.colorOffset, color_to_bytes, color_from_bytes);
    io.field("delay", frame.duration, FRAME_DEFAULT.duration);
    io.field("visible", frame.isVisible, FRAME_DEFAULT.isVisible);
    io.choice("interpolation", frame.interpolation, INTERPOLATION_NAMES, FRAME_DEFAULT.interpolation);
    io.reference("shader", frame.shaderId, content.shaders);
    if (type != ItemType::LAYER) return;
    io.field("depth", frame.depth, FRAME_DEFAULT.depth);
    io.field("blur", frame.blur, FRAME_DEFAULT.blur);
    io.field("child", frame.childAnimation, FRAME_DEFAULT.childAnimation);
    io.choice("childMode", frame.childMode, CHILD_MODE_NAMES, FRAME_DEFAULT.childMode);
    io.field("childStart", frame.childStart, FRAME_DEFAULT.childStart);
    io.field("childStartRandom", frame.isChildStartRandom, FRAME_DEFAULT.isChildStartRandom);
    io.field("restart", frame.isChildRestart, FRAME_DEFAULT.isChildRestart);
  }

  template <class Io, class Frames>
  void frames_io(Io& io, std::string_view name, Frames& frames, ItemType type,
                 const std::vector<Region>& regions = REGIONS_NONE)
  {
    // Each frame keeps a line of its own.
    io.list(name, frames, [&](auto& frameIo, auto& frame) { frame_io(frameIo, frame, type, regions); }, true, false);
  }

  template <class Io, class T> void track_io(Io& io, T& track, ItemType type)
  {
    auto isLayer = type == ItemType::LAYER;
    if constexpr (Io::IS_READING) track.type = type;
    if (isLayer)
      io.reference("layer", track.id, io.model.content.layers);
    else
      io.reference("null", track.id, io.model.content.nulls);
    io.field("visible", track.isVisible, true);
    auto spritesheet = isLayer ? io.model.layer_spritesheet_get(track.id) : nullptr;
    frames_io(io, "frames", track.frames, type, spritesheet ? spritesheet->regions : REGIONS_NONE);
    if constexpr (Io::IS_READING)
      if (isLayer)
        for (auto& frame : track.frames)
          frame = io.model.frame_effective(track.id, frame);
  }

  // A group is an entry with a "group" name, its root and its own tracks.
  template <class Io, class Entries> void entries_io(Io& io, std::string_view name, Entries& entries, ItemType type)
  {
    io.list(
        name, entries,
        [&](auto& entryIo, auto& entry)
        {
          if constexpr (Io::IS_READING)
            if (entryIo.has("group")) entry = TrackGroup{};
          std::visit(
              [&](auto& item)
              {
                if constexpr (std::is_same_v<std::decay_t<decltype(item)>, Track>)
                  return track_io(entryIo, item, type);
                else
                {
                  entryIo.field("group", item.name);
                  entryIo.field("expanded", item.isExpanded, true);
                  entryIo.field("visible", item.isVisible, true);
                  frames_io(entryIo, "root", item.root.frames, ItemType::ROOT);
                  entryIo.list(
                      name, item.tracks, [&](auto& trackIo, auto& track) { track_io(trackIo, track, type); }, false);
                }
              },
              entry);
        },
        false);
    if constexpr (Io::IS_READING)
    {
      int id{};
      groups_each(entries, [&](TrackGroup& group) { group.id = id++; });
    }
  }

  template <class Io, class A> void animation_io(Io& io, A& animation)
  {
    io.field("length", animation.frameNum, 1);
    io.field("loop", animation.isLoop, true);
    io.field("startMarker", animation.startMarker, -1);
    io.field("endMarker", animation.endMarker, -1);
    frames_io(io, "root", animation.root.frames, ItemType::ROOT);
    entries_io(io, "layers", animation.layers, ItemType::LAYER);
    entries_io(io, "nulls", animation.nulls, ItemType::NULL_);
    frames_io(io, "triggers", animation.triggers.frames, ItemType::TRIGGER);
  }

  // Animations (and groups, which hold an "animations" object) are keyed by name; a repeated name is renumbered.
  void animations_write(JsonWriter& io, const Model& model)
  {
    std::set<std::string> names{};
    auto name_unique_get = [&](const std::string& name)
    {
      auto candidate = name;
      for (int i = 2; names.contains(candidate); ++i)
        candidate = std::format("{} {}", name, i);
      names.insert(candidate);
      return candidate;
    };
    auto animation_write = [&](const Animation& animation)
    { io.object(name_unique_get(animation.name), [&](JsonWriter&) { animation_io(io, animation); }, false); };

    io.object(
        "animations",
        [&](JsonWriter&)
        {
          for (const auto& entry : model.animations.entries)
          {
            if (auto animation = std::get_if<std::shared_ptr<const Animation>>(&entry))
            {
              animation_write(**animation);
              continue;
            }
            auto& group = std::get<AnimationGroup>(entry);
            io.object(
                name_unique_get(group.name),
                [&](JsonWriter&)
                {
                  io.field("expanded", group.isExpanded, true);
                  io.field("visible", group.isVisible, true);
                  io.object(
                      "animations",
                      [&](JsonWriter&)
                      {
                        for (const auto& animation : group.animations)
                          animation_write(*animation);
                      },
                      false);
                },
                false);
          }
        },
        false);
  }

  void animations_read(JsonReader& io, Model& model)
  {
    auto animation_read = [&](std::string_view name, JsonReader& animationIo)
    {
      Animation animation{.name = std::string(name), .root = {.type = ItemType::ROOT}};
      animation_io(animationIo, animation);
      return std::make_shared<const Animation>(std::move(animation));
    };
    int groupId{};
    io.object(
        "animations",
        [&](JsonReader& animationsIo)
        {
          animationsIo.isKeyed = true;
          for (auto [name, value] : animationsIo.fields)
            io.object_each(value, std::format("{}{}.", animationsIo.path, name),
                           [&](JsonReader& entryIo)
                           {
                             if (!entryIo.has("animations"))
                             {
                               model.animations.entries.emplace_back(animation_read(name, entryIo));
                               return;
                             }
                             AnimationGroup group{.id = groupId++, .name = std::string(name)};
                             entryIo.field("expanded", group.isExpanded);
                             entryIo.field("visible", group.isVisible);
                             entryIo.object(
                                 "animations",
                                 [&](JsonReader& membersIo)
                                 {
                                   membersIo.isKeyed = true;
                                   for (auto [memberName, member] : membersIo.fields)
                                     io.object_each(
                                         member, std::format("{}{}.", membersIo.path, memberName),
                                         [&](JsonReader& animationIo)
                                         { group.animations.push_back(animation_read(memberName, animationIo)); });
                                 },
                                 false);
                             model.animations.entries.emplace_back(std::move(group));
                           });
        },
        false);
  }

  // Content comes before animations, which read it (and anything named comes before what names it).
  template <class Io, class M> void model_io(Io& io, M& model)
  {
    auto& content = model.content;
    io.object(
        "info",
        [&](auto& infoIo)
        {
          infoIo.field("createdBy", model.info.createdBy, Info{}.createdBy);
          infoIo.field("createdOn", model.info.createdOn, {}, date_to_iso, date_from_iso);
          infoIo.field("fps", model.info.fps, Info{}.fps);
          infoIo.field("version", model.info.version, Info{}.version);
          infoIo.field("isaacIgnored", model.info.isIsaacIgnored, false);
        },
        true);
    io.list(
        "spritesheets", content.spritesheets,
        [](auto& spritesheetIo, auto& spritesheet)
        {
          spritesheetIo.field("path", spritesheet.path, {});
          spritesheetIo.list(
              "regions", spritesheet.regions,
              [](auto& regionIo, auto& region)
              {
                regionIo.field("name", region.name, {});
                regionIo.field("crop", region.crop, {});
                regionIo.field("size", region.size, {});
                regionIo.field("pivot", region.pivot, {});
                regionIo.choice("origin", region.origin, ORIGIN_NAMES, Origin::CUSTOM);
              },
              true, false);
        },
        false, false);
    io.list(
        "shaders", content.shaders,
        [](auto& shaderIo, auto& shader)
        {
          shaderIo.field("name", shader.name, {});
          shaderIo.field("vertex", shader.vertex, {});
          shaderIo.field("fragment", shader.fragment, {});
          shaderIo.list(
              "uniforms", shader.uniforms,
              [](auto& uniformIo, auto& uniform)
              {
                uniformIo.field("name", uniform.name, {});
                uniformIo.field("binding", uniform.binding, {});
                uniformIo.field("value", uniform.value, {});
                uniformIo.list(
                    "components", uniform.components,
                    [](auto& componentIo, auto& component)
                    {
                      componentIo.field("index", component.index);
                      componentIo.field("binding", component.binding, {});
                      componentIo.field("value", component.value, {});
                    },
                    true);
              },
              true);
        },
        false, false);
    // Libraries come before the layers that name them.
    io.values("libraries", content.libraries, [](auto& library) -> auto& { return library.path; });
    io.list(
        "layers", content.layers,
        [&](auto& layerIo, auto& layer)
        {
          layerIo.field("name", layer.name, {});
          layerIo.reference("sheet", layer.spritesheetId, content.spritesheets, Layer{}.spritesheetId);
          layerIo.reference("library", layer.libraryId, content.libraries);
          layerIo.field("animation", layer.animation, {});
        },
        true, false);
    io.list(
        "nulls", content.nulls,
        [](auto& nullIo, auto& null)
        {
          nullIo.field("name", null.name, {});
          nullIo.field("showRect", null.isShowRect, false);
        },
        true, false);
    io.values("events", content.events, [](auto& event) -> auto& { return event.name; });
    io.values("sounds", content.sounds, [](auto& sound) -> auto& { return sound.path; });
    io.field("defaultAnimation", model.animations.defaultAnimation, {});
    if constexpr (Io::IS_READING)
      animations_read(io, model);
    else
      animations_write(io, model);
  }

  std::string model_to_json(const Model& model)
  {
    JsonWriter io{model};
    io.open('{', false);
    io.field(VERSION_KEY, VERSION);
    model_io(io, model);
    io.close('}');
    io.text += '\n';
    return std::move(io.text);
  }

  // Fails only when the text isn't an anm2ed JSON document; anything it couldn't read is listed in `errorString`.
  bool model_from_json(Model& model, std::string_view text, std::string* errorString)
  {
    dom::parser parser{};
    padded_string json(text);
    dom::element root{};
    std::vector<std::string> issues{};
    JsonReader io{model, &issues};
    auto error = parser.parse(json).get(root);
    if (!error) error = root.get(io.fields);
    if (error || !io.has(VERSION_KEY))
    {
      if (errorString) *errorString = error ? error_message(error) : "Not an anm2ed JSON document.";
      return false;
    }
    model = Model{};
    model_io(io, model);
    io.unknown_keys_check();
    model_repair(model);
    if (!errorString || issues.empty()) return true;
    errorString->clear();
    for (std::size_t i = 0; i < std::min(issues.size(), ISSUES_SHOWN_MAX); ++i)
      *errorString += (i ? "\n" : "") + issues[i];
    if (issues.size() > ISSUES_SHOWN_MAX) *errorString += std::format("\n({} more)", issues.size() - ISSUES_SHOWN_MAX);
    return true;
  }
}
