#include "model/xml.h"

#include <algorithm>
#include <charconv>
#include <cstdio>
#include <format>
#include <functional>
#include <map>
#include <optional>
#include <ranges>
#include <tuple>

#include <pugixml/src/pugixml.hpp>

#include "file.h"
#include "model/frames.h"
#include "model/json.h"
#include "math_util.h"
#include "path.h"
#include "timestamp.h"

constexpr std::string_view ELEMENT_TAGS[] = {
#define X(symbol, tag) tag,
    ANM2_ELEMENT_TYPES
#undef X
};

constexpr std::string_view INTERPOLATION_VALUES[] = {"", "", "EaseIn", "EaseOut", "EaseInOut"};
constexpr std::string_view ORIGIN_VALUES[] = {"", "TopLeft", "Center"};
constexpr std::string_view CHILD_MODE_VALUES[] = {"Loop", "Once", "Hold"};
constexpr std::string_view CHILD_START_VALUES[] = {"", "Random"};
constexpr const char* TINT_ATTRIBUTES[] = {"RedTint", "GreenTint", "BlueTint", "AlphaTint"};
constexpr const char* OFFSET_ATTRIBUTES[] = {"RedOffset", "GreenOffset", "BlueOffset"};
// Attributes the editor reads (or older builds wrote, like ExtendedVersion). Any other attribute is kept verbatim as
// Extras.
// clang-format off
  constexpr std::string_view KNOWN_ATTRIBUTES[] = {
      "Name", "CreatedBy", "CreatedOn", "ExtendedVersion", "Binding", "Value", "DefaultAnimation", "Path", "Vertex", "Fragment", "Id",
      "LayerId", "NullId", "SpritesheetId", "Fps", "Version", "FrameNum", "Delay", "AtFrame", "EventId", "RegionId",
      "ShaderId", "Depth", "SoundId", "GroupId", "Index", "Loop", "StartMarker", "EndMarker", "Visible", "ShowRect", "IsExpanded", "Enabled", "Rotation",
      "XPivot", "YPivot", "XCrop", "YCrop", "XPosition", "YPosition", "Width", "Height", "XScale", "YScale", "SkewX",
      "SkewY", "ShearX", "ShearY", "RedTint", "GreenTint", "BlueTint", "AlphaTint", "RedOffset", "GreenOffset", "BlueOffset", "AlphaOffset",
      "Interpolated", "Origin", "BakeInterpolation", "BakeDelay", "BakeCount", "OriginalDelay", "Library", "Animation",
      "IsaacIgnored",
      "ChildAnimation", "ChildMode", "ChildStart", "Restart", "Blur"};
// clang-format on
constexpr std::pair<std::string_view, bool> BOOL_WORDS[] = {{"true", true},   {"True", true},   {"TRUE", true},
                                                            {"false", false}, {"False", false}, {"FALSE", false}};
constexpr std::string_view SOURCE_DOCUMENT_TAG = "SourceDocument";
constexpr std::string_view FRAME_RESTORE_TAG = "FrameRestore";
constexpr std::string_view LEGACY_OVERLAY_TAG = "Overlay";
constexpr std::string_view LEGACY_GROUPS_TAG = "Groups";
constexpr const char* CREATED_ON_FORMAT = "%m/%d/%Y %I:%M:%S %p";
constexpr const char* SHADER_NAME_DEFAULT = "New Shader";
constexpr std::size_t VALUE_BUFFER_SIZE = 64;
constexpr int FLOAT_DIGITS = 8;
constexpr int DOUBLE_DIGITS = 17;
constexpr double FLOAT_WHOLE_MAX = 1e8;
constexpr std::string_view INDENT = "    ";
constexpr int TEXT_ENTITY_COUNT = 3;
constexpr std::pair<char, std::string_view> ENTITIES[] = {
    {'&', "&amp;"}, {'<', "&lt;"}, {'>', "&gt;"}, {'"', "&quot;"}, {'\'', "&apos;"}};
constexpr std::uint64_t HASH_OFFSET = 14695981039346656037ull;
constexpr std::uint64_t HASH_PRIME = 1099511628211ull;

static const char* tag_get(ElementType type)
{
    return ELEMENT_TAGS[(int)type].data();
}

static ElementType type_get(pugi::xml_node element)
{
    auto it = std::ranges::find(ELEMENT_TAGS, std::string_view(element.name()));
    return it == std::end(ELEMENT_TAGS) ? ElementType::UNKNOWN : (ElementType)(it - std::begin(ELEMENT_TAGS));
}

static ElementType track_element_type_get(ItemType type)
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

// Reading: pugixml straight into the model.

// Numbers and booleans read as tinyxml2 (which older builds used) reads them: "%d" and "%f" on the leading text,
// and booleans as a number or true/false.
static const char* attribute_get(pugi::xml_node element, const char* name)
{
    auto attribute = element.attribute(name);
    return attribute ? attribute.value() : nullptr;
}

static bool is_int_read(const char* text, int& value)
{
    return text && std::sscanf(text, "%d", &value) == 1;
}

static int int_get(pugi::xml_node element, const char* name, int value = -1)
{
    is_int_read(attribute_get(element, name), value);
    return value;
}

static float float_get(pugi::xml_node element, const char* name, float value = 0.0f)
{
    if (auto text = attribute_get(element, name))
        std::sscanf(text, "%f", &value);
    return value;
}

static bool bool_get(pugi::xml_node element, const char* name, bool value)
{
    auto text = attribute_get(element, name);
    if (int number{}; is_int_read(text, number))
        return number != 0;
    if (!text)
        return value;
    for (auto [word, isTrue] : BOOL_WORDS)
        if (word == text)
            return isTrue;
    return value;
}

static std::string string_get(pugi::xml_node element, const char* name, std::string value = {})
{
    if (auto attribute = attribute_get(element, name))
        value = attribute;
    return value;
}

static float color_get(pugi::xml_node element, const char* name, float value)
{
    if (int color{}; is_int_read(attribute_get(element, name), color))
        value = uint8_to_float(color);
    return value;
}

// An element's text: its first child, when that is text.
static const char* text_get(pugi::xml_node element)
{
    auto first = element.first_child();
    return first.type() == pugi::node_pcdata || first.type() == pugi::node_cdata ? first.value() : nullptr;
}

static Interpolation interpolation_read(pugi::xml_node element, const char* name)
{
    auto value = attribute_get(element, name);
    if (!value)
        return Interpolation::NONE;
    for (std::size_t i = 0; i < std::size(INTERPOLATION_VALUES); ++i)
        if (!INTERPOLATION_VALUES[i].empty() && INTERPOLATION_VALUES[i] == value)
            return (Interpolation)i;
    return bool_get(element, name, false) ? Interpolation::LINEAR : Interpolation::NONE;
}

static pugi::xml_node element_next(pugi::xml_node node)
{
    do
        node = node.next_sibling();
    while (node && node.type() != pugi::node_element);
    return node;
}

static pugi::xml_node element_first(pugi::xml_node parent)
{
    auto child = parent.first_child();
    return !child || child.type() == pugi::node_element ? child : element_next(child);
}

// Shader elements are only read where they belong. One anywhere else is dropped.
static bool is_child_kept(pugi::xml_node parent, pugi::xml_node child)
{
    std::string_view tag = child.name();
    if (tag == LEGACY_OVERLAY_TAG || tag == SOURCE_DOCUMENT_TAG)
        return false;
    auto parentType = parent.type() == pugi::node_element ? type_get(parent) : ElementType::UNKNOWN;
    switch (type_get(child))
    {
        case ElementType::SHADERS:
            return parentType == ElementType::CONTENT;
        case ElementType::SHADER:
            return parentType == ElementType::SHADERS;
        case ElementType::UNIFORM:
            return parentType == ElementType::SHADER;
        case ElementType::COMPONENT:
            return parentType == ElementType::UNIFORM;
        default:
            return true;
    }
}

// Child elements with their position, skipping legacy Overlay, nested SourceDocument copies and misplaced shaders.
template <class Callback> static void children_each(pugi::xml_node parent, Callback&& callback)
{
    int position = 0;
    for (auto child : parent.children())
        if (child.type() == pugi::node_element && is_child_kept(parent, child))
            callback(child, type_get(child), position++);
}

// A repeated attribute keeps its first position and last value.
static void attribute_read(std::vector<std::pair<std::string, std::string>>& attributes, pugi::xml_attribute attribute)
{
    auto existing = std::ranges::find(attributes, std::string_view(attribute.name()),
                                      [](const auto& pair) { return std::string_view(pair.first); });
    if (existing != attributes.end())
        existing->second = attribute.value();
    else
        attributes.emplace_back(attribute.name(), attribute.value());
}

static XmlNode node_read(pugi::xml_node element)
{
    XmlNode node = {.tag = element.name()};
    for (auto attribute : element.attributes())
        attribute_read(node.attributes, attribute);
    if (auto text = text_get(element))
        node.text = text;
    children_each(element, [&](pugi::xml_node child, ElementType, int) { node.children.push_back(node_read(child)); });
    return node;
}

static Extras extras_read(pugi::xml_node element)
{
    Extras extras = {};
    for (auto attribute : element.attributes())
        if (!std::ranges::contains(KNOWN_ATTRIBUTES, std::string_view(attribute.name())))
            attribute_read(extras.attributes, attribute);
    if (auto text = text_get(element))
        extras.text = text;
    return extras;
}

static void extras_child_add(Extras& extras, pugi::xml_node child, int position)
{
    extras.children.emplace_back(position, node_read(child));
}

// Writing: the model goes straight to a Sink in the shape its SaveMode asks for, as text, a hash or an XmlNode tree.

struct Writer
{
    SaveMode mode = SAVE_GAME;
    const Model* model = nullptr;
    std::map<int, int> shaderIds;
    std::map<int, int> layerIds;
    std::map<int, std::map<int, int>> regionIds;

    // With a model, a whole document is written: shader, layer and region ids are renumbered by position.
    Writer(SaveMode mode, const Model* model = nullptr) : mode(mode), model(model)
    {
        if (!model)
            return;
        auto& content = model->content;
        for (int i = 0; i < (int)content.shaders.size(); ++i)
            shaderIds[content.shaders[i].id] = i;
        for (int i = 0; i < (int)content.layers.size(); ++i)
            layerIds[content.layers[i].id] = i;
        std::set<int> spritesheetIds;
        for (auto& spritesheet : content.spritesheets)
            if (spritesheetIds.insert(spritesheet.id).second && !spritesheet.regions.empty())
                for (int i = 0; i < (int)spritesheet.regions.size(); ++i)
                    regionIds[spritesheet.id][spritesheet.regions[i].id] = i;
    }

    // Groups, regions, sounds and the editor's own attributes are written in all but the game's copy.
    bool is_extended() const
    {
        return mode != SAVE_GAME;
    }
};

static int id_map_get(const std::map<int, int>& ids, int id)
{
    auto it = ids.find(id);
    return it == ids.end() ? -1 : it->second;
}

static bool is_float_whole(double value)
{
    return std::abs(value) < FLOAT_WHOLE_MAX && value == std::trunc(value) && !(value == 0 && std::signbit(value));
}

// Where written XML goes. TEXT prints as tinyxml2's XMLPrinter did (4-space indents, entities escaped,
// text keeping its element's children inline). HASH folds the same elements into an FNV hash. TREE builds an XmlNode.
struct Sink
{
    enum Mode
    {
        TEXT,
        HASH,
        TREE
    };

    struct Open
    {
        std::string_view tag;
        bool isEmpty = true;
        bool isTextDone = false;
    };

    Mode mode = TEXT;
    std::string text;
    std::uint64_t hash = HASH_OFFSET;
    XmlNode root = {};
    std::vector<Open> stack;
    std::vector<XmlNode*> nodes;
    int textDepth = -1;

    void hash_add(char marker, std::string_view value)
    {
        hash = (hash ^ (unsigned char)marker) * HASH_PRIME;
        for (unsigned char character : value)
            hash = (hash ^ character) * HASH_PRIME;
        hash *= HASH_PRIME;
    }

    // Attributes escape all five entities. Text only the first three.
    void escape(std::string_view value, bool isAttribute)
    {
        for (auto character : value)
        {
            auto entity = std::ranges::find(ENTITIES, character, &std::pair<char, std::string_view>::first);
            if (entity != std::end(ENTITIES) && (isAttribute || entity - ENTITIES < TEXT_ENTITY_COUNT))
                text += entity->second;
            else
                text += character;
        }
    }

    void indent(std::size_t depth)
    {
        for (std::size_t i = 0; i < depth; ++i)
            text += INDENT;
    }

    // The open element gets content: its start tag is finished, and its (empty) text is hashed.
    void content_begin()
    {
        if (stack.empty())
            return;
        auto& parent = stack.back();
        if (mode == TEXT && parent.isEmpty)
            text += '>';
        if (mode == HASH && !parent.isTextDone)
            hash_add('T', {});
        parent.isEmpty = false;
        parent.isTextDone = true;
    }

    void open(std::string_view tag)
    {
        content_begin();
        if (mode == TEXT)
        {
            if (!text.empty() && textDepth < 0)
                text += '\n';
            if (text.empty() || textDepth < 0)
                indent(stack.size());
            text += '<';
            text += tag;
        }
        if (mode == HASH)
            hash_add('E', tag);
        if (mode == TREE)
        {
            auto& node = nodes.empty() ? root : nodes.back()->children.emplace_back();
            node.tag = tag;
            nodes.push_back(&node);
        }
        stack.push_back({tag});
    }

    // Numbers never need escaping.
    void attribute_text(std::string_view name, std::string_view value, bool isEscaped = true)
    {
        if (mode == TEXT)
        {
            text += ' ';
            text += name;
            text += "=\"";
            if (isEscaped)
                escape(value, true);
            else
                text += value;
            text += '"';
        }
        if (mode == HASH)
        {
            hash_add('A', name);
            hash_add('V', value);
        }
        if (mode == TREE)
            nodes.back()->attributes.emplace_back(name, value);
    }

    // Numbers print as tinyxml2's XMLUtil::ToStr did ("%d", "%.8g", "true"/"false"), without printf. Whole numbers
    // (most values) print as integers, which "%.8g" also does below 1e8 (but keeps "-0").
    template <class T> void attribute(std::string_view name, const T& value)
    {
        if constexpr (std::is_convertible_v<T, std::string_view>)
            attribute_text(name, value);
        else
        {
            char buffer[VALUE_BUFFER_SIZE]{};
            auto end = buffer;
            if constexpr (std::is_same_v<T, bool>)
                end = std::ranges::copy(std::string_view(value ? "true" : "false"), buffer).out;
            else if constexpr (std::is_floating_point_v<T>)
                end = is_float_whole(value)
                          ? std::to_chars(buffer, buffer + sizeof(buffer), (long long)value).ptr
                          : std::to_chars(buffer, buffer + sizeof(buffer), value, std::chars_format::general,
                                          std::is_same_v<T, float> ? FLOAT_DIGITS : DOUBLE_DIGITS)
                                .ptr;
            else
                end = std::to_chars(buffer, buffer + sizeof(buffer), (long long)value).ptr;
            attribute_text(name, std::string_view(buffer, end), false);
        }
    }

    // An element's text, after its attributes and before its children.
    void text_add(std::string_view value)
    {
        auto& open = stack.back();
        if (mode == HASH)
            hash_add('T', value);
        if (mode == TREE)
            nodes.back()->text = value;
        if (mode == TEXT && !value.empty())
        {
            textDepth = (int)stack.size() - 1;
            text += '>';
            open.isEmpty = false;
            escape(value, false);
        }
        open.isTextDone = true;
    }

    void close()
    {
        auto open = stack.back();
        stack.pop_back();
        if (mode == HASH)
        {
            if (!open.isTextDone)
                hash_add('T', {});
            hash_add(']', {});
        }
        if (mode == TREE)
            nodes.pop_back();
        if (mode != TEXT)
            return;
        if (open.isEmpty)
            text += "/>";
        else
        {
            if (textDepth < 0)
            {
                text += '\n';
                indent(stack.size());
            }
            text += "</";
            text += open.tag;
            text += '>';
        }
        if (textDepth == (int)stack.size())
            textDepth = -1;
        if (stack.empty())
            text += '\n';
    }

    void node(const XmlNode& node)
    {
        open(node.tag);
        for (auto& [name, value] : node.attributes)
            attribute_text(name, value);
        text_add(node.text);
        for (auto& child : node.children)
            this->node(child);
        close();
    }
};

// Unknown XML goes back where it was read: attributes after the known ones, then text, and (through Children)
// children at their recorded positions among the known ones.
static void extras_attributes_write(Sink& sink, const Extras& extras)
{
    for (auto& [name, value] : extras.attributes)
        sink.attribute_text(name, value);
}

static void extras_write(Sink& sink, const Extras& extras)
{
    extras_attributes_write(sink, extras);
    sink.text_add(extras.text);
}

// Interleaves an element's unknown children with its known ones: add() comes before each known child.
struct Children
{
    Sink& sink;
    const Extras& extras;
    std::size_t next = 0;
    int position = 0;

    void add()
    {
        while (next < extras.children.size() && extras.children[next].first <= position)
        {
            sink.node(extras.children[next++].second);
            ++position;
        }
        ++position;
    }

    void flush()
    {
        while (next < extras.children.size())
            sink.node(extras.children[next++].second);
    }

    void close()
    {
        flush();
        sink.close();
    }
};

static bool is_special_interpolation(Interpolation interpolation)
{
    return interpolation != Interpolation::NONE && interpolation != Interpolation::LINEAR;
}

// Reading and writing share one description of each element (its *_io function), run by an XmlReader or an XmlWriter.
// The reader reads every attribute, whatever the writer leaves out (`isWritten`), and keeps a field's default when its
// attribute is missing.

template <class Item> constexpr ElementType ITEM_TYPE{};
template <> constexpr ElementType ITEM_TYPE<Spritesheet>{ElementType::SPRITESHEET};
template <> constexpr ElementType ITEM_TYPE<Region>{ElementType::REGION};
template <> constexpr ElementType ITEM_TYPE<Shader>{ElementType::SHADER};
template <> constexpr ElementType ITEM_TYPE<Uniform>{ElementType::UNIFORM};
template <> constexpr ElementType ITEM_TYPE<Component>{ElementType::COMPONENT};
template <> constexpr ElementType ITEM_TYPE<Layer>{ElementType::LAYER_ELEMENT};
template <> constexpr ElementType ITEM_TYPE<Null>{ElementType::NULL_ELEMENT};
template <> constexpr ElementType ITEM_TYPE<Event>{ElementType::EVENT_ELEMENT};
template <> constexpr ElementType ITEM_TYPE<Sound>{ElementType::SOUND_ELEMENT};
template <> constexpr ElementType ITEM_TYPE<Library>{ElementType::LIBRARY};

// A whole document renumbers shader, layer and region ids by position.
template <class Item>
constexpr bool IS_RENUMBERED =
    std::is_same_v<Item, Shader> || std::is_same_v<Item, Layer> || std::is_same_v<Item, Region>;

// An io function takes its item as written (const) or read.
template <class T, class Base>
concept Is = std::same_as<std::remove_const_t<T>, Base>;

template <class Item> static Item element_read(pugi::xml_node element);
template <class Item> static void element_write(Sink& sink, const Writer& writer, const Item& item, int id = -1);

// Children that no list takes are kept as extras when the element closes.
struct XmlReader
{
    static constexpr bool IS_READING = true;
    static constexpr SaveMode mode = SAVE_CLIPBOARD;

    pugi::xml_node element;
    Extras* extrasRead = nullptr;
    std::vector<ElementType> taken;

    bool is_extended()
    {
        return true;
    }

    template <class T> void attribute(const char* name, T& value, bool = true)
    {
        if constexpr (std::is_same_v<T, bool>)
            value = bool_get(element, name, value);
        else if constexpr (std::is_same_v<T, int>)
            value = int_get(element, name, value);
        else if constexpr (std::is_same_v<T, float>)
            value = float_get(element, name, value);
        else if constexpr (std::is_same_v<T, std::filesystem::path>)
        {
            if (auto text = attribute_get(element, name))
                value = path_from_utf8(text);
        }
        else
            value = string_get(element, name, value);
    }

    void id(const char* name, int& value)
    {
        attribute(name, value);
    }

    void color(const char* name, float& value, bool = true)
    {
        value = color_get(element, name, value);
    }

    template <class T, std::size_t N>
    void choice(const char* name, T& value, const std::string_view (&values)[N], bool = true)
    {
        if (auto text = attribute_get(element, name))
            for (std::size_t i = 0; i < N; ++i)
                if (!values[i].empty() && values[i] == text)
                    value = (T)i;
    }

    void interpolation(const char* name, Interpolation& value, bool = true)
    {
        value = interpolation_read(element, name);
    }

    void extras(Extras& extras)
    {
        extras = extras_read(element);
        extrasRead = &extras;
    }

    template <class Item> void list(std::vector<Item>& items, bool = true)
    {
        taken.push_back(ITEM_TYPE<Item>);
        children_each(element, [&](pugi::xml_node child, ElementType type, int) {
            if (type == ITEM_TYPE<Item>)
                items.push_back(element_read<Item>(child));
        });
    }

    // Ids as <Type Id> children, with `first` (from an attribute) first.
    void ids(ElementType type, std::vector<int>& ids, int first, bool = true)
    {
        taken.push_back(type);
        children_each(element, [&](pugi::xml_node child, ElementType childType, int) {
            if (childType == type)
                ids.push_back(int_get(child, "Id"));
        });
        if (first != -1 && !std::ranges::contains(ids, first))
            ids.insert(ids.begin(), first);
        std::erase(ids, -1);
    }

    void close()
    {
        children_each(element, [&](pugi::xml_node child, ElementType type, int position) {
            if (!std::ranges::contains(taken, type))
                extras_child_add(*extrasRead, child, position);
        });
    }
};

// Opens its element on construction. Lists go between the element's unknown children.
struct XmlWriter
{
    static constexpr bool IS_READING = false;

    Sink& sink;
    const Writer& writer;
    Children children;
    SaveMode mode;
    int idWritten;

    XmlWriter(Sink& sink, const Writer& writer, ElementType type, const Extras& extras, int id = -1)
        : sink(sink), writer(writer), children{sink, extras}, mode(writer.mode), idWritten(id)
    {
        sink.open(tag_get(type));
    }

    bool is_extended()
    {
        return writer.is_extended();
    }

    // Paths are only written when set.
    template <class T> void attribute(const char* name, const T& value, bool isWritten = true)
    {
        if constexpr (std::is_same_v<T, std::filesystem::path>)
        {
            if (isWritten && !value.empty())
                sink.attribute(name, path_to_utf8(value));
        }
        else if (isWritten)
            sink.attribute(name, value);
    }

    void id(const char* name, int)
    {
        sink.attribute(name, idWritten);
    }

    void color(const char* name, float value, bool isWritten = true)
    {
        if (isWritten)
            sink.attribute(name, float_to_uint8(value));
    }

    template <class T, std::size_t N>
    void choice(const char* name, T value, const std::string_view (&values)[N], bool isWritten = true)
    {
        if (isWritten)
            sink.attribute(name, values[(int)value]);
    }

    void interpolation(const char* name, Interpolation value, bool isWritten = true)
    {
        if (isWritten && is_special_interpolation(value))
            sink.attribute(name, INTERPOLATION_VALUES[(int)value]);
        else if (isWritten)
            sink.attribute(name, value == Interpolation::LINEAR);
    }

    void extras(const Extras& extras)
    {
        extras_write(sink, extras);
    }

    template <class Item> void list(const std::vector<Item>& items, bool isWritten = true)
    {
        for (int i = 0; isWritten && i < (int)items.size(); ++i)
        {
            children.add();
            if constexpr (requires { items[i].id; })
                element_write(sink, writer, items[i], writer.model && IS_RENUMBERED<Item> ? i : items[i].id);
            else
                element_write(sink, writer, items[i]);
        }
    }

    void ids(ElementType type, const std::vector<int>& ids, int first, bool isWritten)
    {
        for (int i = 0; isWritten && i < (int)ids.size(); ++i)
            if (ids[i] != -1 && (i > 0 || ids[i] != first))
            {
                children.add();
                sink.open(tag_get(type));
                sink.attribute("Id", ids[i]);
                sink.close();
            }
    }

    void close()
    {
        children.close();
    }
};

static void vec_io(auto& io, const char* xName, const char* yName, auto& value, bool isWritten = true)
{
    io.attribute(xName, value.x, isWritten);
    io.attribute(yName, value.y, isWritten);
}

// A frame, or a trigger on a triggers track. A layer frame on a region writes the region in place of its rectangle,
// and the game's copy leaves out the editor's own parts. A trigger's first event is its EventId and any others are
// <Event Id> children. Its sounds are <Sound Id> children (older files name one with a SoundId attribute instead).
static void frame_io(auto& io, auto& frame, ItemType type)
{
    auto isExtended = io.is_extended();
    auto isTrigger = type == ItemType::TRIGGER;
    auto isLayer = type == ItemType::LAYER;
    auto isRegion = isExtended && frame.regionId != -1;
    auto isRect = isLayer && (io.mode == SAVE_CLIPBOARD || !isRegion);
    auto isPurged = io.mode != SAVE_CLIPBOARD;
    auto isChild = isExtended && isLayer;
    auto eventId = frame.eventIds.empty() ? -1 : frame.eventIds.front();
    auto soundId = -1;

    io.attribute("EventId", eventId, isTrigger && !frame.eventIds.empty());
    io.attribute("SoundId", soundId, false);
    io.attribute("AtFrame", frame.atFrame, isTrigger);
    io.attribute("RegionId", frame.regionId, isLayer && isRegion);
    vec_io(io, "XPivot", "YPivot", frame.pivot, isRect);
    vec_io(io, "XCrop", "YCrop", frame.crop, isRect);
    vec_io(io, "Width", "Height", frame.size, isRect);
    vec_io(io, "XPosition", "YPosition", frame.position, !isTrigger);
    io.attribute("Delay", frame.duration, !isTrigger);
    io.attribute("Visible", frame.isVisible, !isTrigger);
    vec_io(io, "XScale", "YScale", frame.scale, !isTrigger);
    // Skew was once called shear.
    vec_io(io, "ShearX", "ShearY", frame.skew, false);
    vec_io(io, "SkewX", "SkewY", frame.skew, !isTrigger && isExtended);
    for (int i = 0; i < (int)std::size(TINT_ATTRIBUTES); ++i)
        io.color(TINT_ATTRIBUTES[i], frame.tint[i],
                 !isTrigger && (!isPurged || float_to_uint8(frame.tint[i]) != float_to_uint8(1.0f)));
    for (int i = 0; i < (int)std::size(OFFSET_ATTRIBUTES); ++i)
        io.color(OFFSET_ATTRIBUTES[i], frame.colorOffset[i],
                 !isTrigger && (!isPurged || float_to_uint8(frame.colorOffset[i]) != 0));
    io.color("AlphaOffset", frame.colorOffset.a, !isTrigger && isExtended && float_to_uint8(frame.colorOffset.a) != 0);
    io.attribute("Rotation", frame.rotation, !isTrigger);
    io.interpolation("Interpolated", frame.interpolation, !isTrigger);
    io.attribute("ShaderId", frame.shaderId, !isTrigger && isExtended && frame.shaderId != -1);
    io.attribute("Depth", frame.depth, isChild && frame.depth != 0);
    io.attribute("Blur", frame.blur, isChild && frame.blur != 0.0f);
    io.attribute("ChildAnimation", frame.childAnimation, isChild && !frame.childAnimation.empty());
    io.choice("ChildMode", frame.childMode, CHILD_MODE_VALUES, isChild && frame.childMode != ChildMode::LOOP);
    io.choice("ChildStart", frame.isChildStartRandom, CHILD_START_VALUES, isChild && frame.isChildStartRandom);
    io.attribute("ChildStart", frame.childStart, isChild && !frame.isChildStartRandom && frame.childStart != 0);
    io.attribute("Restart", frame.isChildRestart, isChild && frame.isChildRestart);
    io.extras(frame.extras);
    io.ids(ElementType::EVENT_ELEMENT, frame.eventIds, eventId, isTrigger && isExtended);
    io.ids(ElementType::SOUND_ELEMENT, frame.soundIds, soundId, isTrigger && isExtended);
}

// Layer and null tracks name their item, and a grouped one names its group in the clipboard.
static void track_io(auto& io, auto& track, int& groupId)
{
    auto isItem = track.type == ItemType::LAYER || track.type == ItemType::NULL_;
    if (isItem)
        io.id(track.type == ItemType::LAYER ? "LayerId" : "NullId", track.id);
    io.attribute("Visible", track.isVisible, isItem);
    io.attribute("GroupId", groupId, isItem && groupId != -1 && io.mode == SAVE_CLIPBOARD);
    io.extras(track.extras);
}

// `index` is the group's position, when groups are written apart from their container.
static void group_io(auto& io, auto& group, int& index)
{
    io.attribute("Id", group.id, io.mode != SAVE_EDITOR);
    io.attribute("Name", group.name);
    io.attribute("IsExpanded", group.isExpanded);
    io.attribute("Visible", group.isVisible);
    io.attribute("Index", index, index != -1);
    io.extras(group.extras);
}

// Markers are the editor's own, and a grouped animation names its group in the clipboard.
static void animation_io(auto& io, auto& animation, int& groupId)
{
    io.attribute("Name", animation.name);
    io.attribute("FrameNum", animation.frameNum);
    io.attribute("Loop", animation.isLoop);
    io.attribute("StartMarker", animation.startMarker, io.is_extended() && animation.startMarker != -1);
    io.attribute("EndMarker", animation.endMarker, io.is_extended() && animation.endMarker != -1);
    io.attribute("GroupId", groupId, groupId != -1 && io.mode == SAVE_CLIPBOARD);
    io.extras(animation.extras);
}

static void info_io(auto& io, auto& info)
{
    io.attribute("CreatedBy", info.createdBy);
    io.attribute("CreatedOn", info.createdOn);
    io.attribute("Fps", info.fps);
    io.attribute("Version", info.version);
    io.attribute("IsaacIgnored", info.isIsaacIgnored, io.is_extended() && info.isIsaacIgnored);
    io.extras(info.extras);
}

// A region with an origin writes it in place of its pivot.
static void item_io(auto& io, Is<Region> auto& region)
{
    io.id("Id", region.id);
    io.attribute("Name", region.name);
    vec_io(io, "XCrop", "YCrop", region.crop);
    vec_io(io, "Width", "Height", region.size);
    io.choice("Origin", region.origin, ORIGIN_VALUES, region.origin != Origin::CUSTOM);
    vec_io(io, "XPivot", "YPivot", region.pivot, region.origin == Origin::CUSTOM);
    io.extras(region.extras);
    if constexpr (std::remove_reference_t<decltype(io)>::IS_READING)
        region.pivot = region_pivot_get(region);
}

static void item_io(auto& io, Is<Spritesheet> auto& spritesheet)
{
    io.id("Id", spritesheet.id);
    io.attribute("Path", spritesheet.path);
    io.extras(spritesheet.extras);
    io.list(spritesheet.regions, io.is_extended());
}

static void item_io(auto& io, Is<Component> auto& component)
{
    io.attribute("Index", component.index);
    io.attribute("Binding", component.binding, !component.binding.empty());
    io.attribute("Value", component.value, !component.value.empty());
    io.extras(component.extras);
}

static void item_io(auto& io, Is<Uniform> auto& uniform)
{
    io.attribute("Name", uniform.name);
    io.attribute("Binding", uniform.binding, !uniform.binding.empty());
    io.attribute("Value", uniform.value, !uniform.value.empty());
    io.extras(uniform.extras);
    io.list(uniform.components);
}

static void item_io(auto& io, Is<Shader> auto& shader)
{
    io.id("Id", shader.id);
    io.attribute("Name", shader.name);
    io.attribute("Vertex", shader.vertex);
    io.attribute("Fragment", shader.fragment);
    io.extras(shader.extras);
    io.list(shader.uniforms);
}

// An animation layer's library and animation are the editor's own.
static void item_io(auto& io, Is<Layer> auto& layer)
{
    auto isAnimation = io.is_extended() && layer.is_animation();
    io.id("Id", layer.id);
    io.attribute("Name", layer.name);
    io.attribute("SpritesheetId", layer.spritesheetId);
    io.attribute("Library", layer.libraryId, isAnimation && layer.libraryId != -1);
    io.attribute("Animation", layer.animation, isAnimation);
    io.extras(layer.extras);
}

static void item_io(auto& io, Is<Null> auto& null)
{
    io.id("Id", null.id);
    io.attribute("Name", null.name);
    io.attribute("ShowRect", null.isShowRect, null.isShowRect);
    io.extras(null.extras);
}

static void item_io(auto& io, Is<Event> auto& event)
{
    io.id("Id", event.id);
    io.attribute("Name", event.name);
    io.extras(event.extras);
}

template <class Io, class Item>
    requires Is<Item, Sound> || Is<Item, Library>
static void item_io(Io& io, Item& item)
{
    io.id("Id", item.id);
    io.attribute("Path", item.path);
    io.extras(item.extras);
}

template <class Item> static Item element_read(pugi::xml_node element)
{
    Item item = {};
    XmlReader io{element};
    item_io(io, item);
    io.close();
    return item;
}

template <class Item> static void element_write(Sink& sink, const Writer& writer, const Item& item, int id)
{
    XmlWriter io(sink, writer, ITEM_TYPE<Item>, item.extras, id);
    item_io(io, item);
    io.close();
}

static Frame frame_read(pugi::xml_node element, ItemType type)
{
    Frame frame = {};
    XmlReader io{element};
    frame_io(io, frame, type);
    io.close();
    return frame;
}

static void frame_write(Sink& sink, const Writer& writer, const Frame& frame, ItemType type)
{
    XmlWriter io(sink, writer, type == ItemType::TRIGGER ? ElementType::TRIGGER : ElementType::FRAME, frame.extras);
    frame_io(io, frame, type);
    io.close();
}

static Track track_read(pugi::xml_node element, ItemType type)
{
    Track track = {.type = type};
    auto groupId = -1;
    XmlReader io{element, {}, {ElementType::FRAME, ElementType::TRIGGER}};
    track_io(io, track, groupId);
    auto frameType = type == ItemType::TRIGGER ? ElementType::TRIGGER : ElementType::FRAME;
    for (auto child = element_first(element); child;)
    {
        auto childType = type_get(child);
        // A frame written baked (BakeInterpolation/BakeDelay) is read back as the one interpolated frame it came from.
        if (auto delay = int_get(child, "BakeDelay", 0);
            childType == ElementType::FRAME && attribute_get(child, "BakeInterpolation") && delay >= FRAME_DURATION_MIN)
        {
            auto frame = frame_read(child, type);
            frame.interpolation = interpolation_read(child, "BakeInterpolation");
            frame.duration = delay;
            if (frameType == ElementType::FRAME)
                track.frames.push_back(std::move(frame));
            for (int i = 0; child && i < delay; ++i)
                child = element_next(child);
            continue;
        }
        if (childType == frameType)
            track.frames.push_back(frame_read(child, type));
        child = element_next(child);
    }
    io.close();
    return track;
}

// A container's children before groups are resolved: a child (with the group it names), a group (with any children
// nested in it), or unknown XML.
template <class Child, class GroupType> struct ContainerItem
{
    using Group = GroupType;

    std::optional<Child> child;
    std::optional<Group> group;
    std::optional<XmlNode> node;
    int groupId = -1;
    int index = -1;
    std::vector<Child> children;
    std::optional<Track> root;
    std::optional<Track> backupRoot;
    std::vector<Track> backupTracks;
    bool isBackup = false;
};

using TrackItem = ContainerItem<Track, TrackGroup>;
using AnimationItem = ContainerItem<std::shared_ptr<const Animation>, AnimationGroup>;

static auto& group_children_get(TrackGroup& group)
{
    return group.items;
}

static auto& group_children_get(AnimationGroup& group)
{
    return group.items;
}

// Groups become entries followed by their children: nested children move out after their group, and a child joins a
// group seen before it. Unknown XML becomes extras at its position.
template <class Entry, class Item> static std::vector<Entry> entries_build(std::vector<Item> items, Extras& extras)
{
    int nextGroupId = 0;
    for (auto& item : items)
        if (item.group)
            nextGroupId = std::max(nextGroupId, item.group->id + 1);

    std::vector<Item> flat;
    for (auto& item : items)
    {
        if (!item.group)
        {
            flat.push_back(std::move(item));
            continue;
        }
        if (item.group->id < 0)
            item.group->id = nextGroupId++;
        auto groupId = item.group->id;
        auto children = std::move(item.children);
        flat.push_back(std::move(item));
        for (auto& child : children)
            flat.push_back({.child = std::move(child), .groupId = groupId});
    }

    std::vector<Entry> entries;
    std::map<int, std::size_t> groupEntries;
    for (int i = 0; i < (int)flat.size(); ++i)
    {
        auto& item = flat[i];
        if (item.node)
            extras.children.emplace_back(i, std::move(*item.node));
        else if (item.group)
        {
            groupEntries[item.group->id] = entries.size();
            entries.emplace_back(std::move(*item.group));
        }
        else if (auto group = groupEntries.find(item.groupId); group != groupEntries.end())
            group_children_get(std::get<typename Item::Group>(entries[group->second]))
                .push_back(std::move(*item.child));
        else
            entries.emplace_back(std::move(*item.child));
    }
    return entries;
}

static TrackItem track_group_item_read(pugi::xml_node element, ItemType type)
{
    TrackItem item{.group = TrackGroup{}};
    XmlReader io{element};
    group_io(io, *item.group, item.index);
    auto trackType = track_element_type_get(type);
    children_each(element, [&](pugi::xml_node child, ElementType childType, int) {
        if (childType == ElementType::ROOT_ANIMATION && !item.root)
            item.root = track_read(child, ItemType::ROOT);
        else if (childType == trackType)
            item.children.push_back(track_read(child, type));
        else if (child.name() == FRAME_RESTORE_TAG && !item.isBackup)
        {
            item.isBackup = true;
            children_each(child, [&](pugi::xml_node backup, ElementType backupType, int) {
                if (backupType == ElementType::ROOT_ANIMATION && !item.backupRoot)
                    item.backupRoot = track_read(backup, ItemType::ROOT);
                else if (backupType == trackType)
                    item.backupTracks.push_back(track_read(backup, type));
            });
        }
    });
    return item;
}

static std::vector<TrackItem> track_items_read(pugi::xml_node container, ItemType type)
{
    std::vector<TrackItem> items;
    children_each(container, [&](pugi::xml_node child, ElementType childType, int) {
        if (childType == ElementType::GROUP)
            items.push_back(track_group_item_read(child, type));
        else if (childType == track_element_type_get(type))
            items.push_back({.child = track_read(child, type), .groupId = int_get(child, "GroupId")});
        else
            items.push_back({.node = node_read(child)});
    });
    return items;
}

// Track groups as any version wrote them: legacy *AnimationGroups metadata is placed back at its Index, and a group
// whose tracks were written baked (FrameRestore) gets its original root and tracks back.
static std::vector<TrackEntry> track_entries_build(std::vector<TrackItem> items, std::vector<TrackItem> legacyGroups,
                                                   Extras& extras)
{
    if (!legacyGroups.empty())
    {
        std::set<int> legacyIds;
        for (auto& item : legacyGroups)
            if (item.group->id >= 0)
                legacyIds.insert(item.group->id);
        std::erase_if(items, [&](const TrackItem& item) { return item.group && legacyIds.contains(item.group->id); });
        std::ranges::stable_sort(legacyGroups, [](const TrackItem& left, const TrackItem& right) {
            if (left.index == -1 || right.index == -1)
                return left.index != -1;
            return left.index < right.index;
        });
        for (auto& legacy : legacyGroups)
        {
            auto first = std::ranges::find_if(
                items, [&](const TrackItem& item) { return item.child && item.groupId == legacy.group->id; });
            auto position =
                legacy.index >= 0 ? std::min(legacy.index, (int)items.size()) : (int)(first - items.begin());
            items.insert(items.begin() + position, std::move(legacy));
        }
    }

    std::set<int> restoredIds;
    for (auto& item : items)
        if (item.group && item.group->id >= 0 && item.isBackup && (item.backupRoot || !item.backupTracks.empty()))
        {
            if (item.backupRoot)
                item.root = item.backupRoot;
            item.children = std::move(item.backupTracks);
            restoredIds.insert(item.group->id);
        }
    std::erase_if(items, [&](const TrackItem& item) { return item.child && restoredIds.contains(item.groupId); });

    for (auto& item : items)
        if (item.group)
            item.group->root = item.root.value_or(TrackGroup{}.root);
    return entries_build<TrackEntry>(std::move(items), extras);
}

static Animation animation_read(pugi::xml_node element)
{
    Animation animation{.root = {.type = ItemType::ROOT}, .layout = {}};
    auto groupId = -1;
    XmlReader io{element};
    animation_io(io, animation, groupId);
    std::map<ElementType, std::vector<TrackItem>> items;
    std::map<ElementType, std::vector<TrackItem>> legacyGroups;
    std::set<ElementType> legacyRead;
    int skipped = 0;
    children_each(element, [&](pugi::xml_node child, ElementType type, int position) {
        if (type == ElementType::LAYER_ANIMATION_GROUPS || type == ElementType::NULL_ANIMATION_GROUPS)
        {
            auto isLayers = type == ElementType::LAYER_ANIMATION_GROUPS;
            auto container = isLayers ? ElementType::LAYER_ANIMATIONS : ElementType::NULL_ANIMATIONS;
            if (legacyRead.insert(type).second)
                children_each(child, [&](pugi::xml_node group, ElementType groupType, int) {
                    if (groupType == ElementType::GROUP)
                        legacyGroups[container].push_back(
                            track_group_item_read(group, isLayers ? ItemType::LAYER : ItemType::NULL_));
                });
            ++skipped;
            return;
        }

        auto isKnown = type == ElementType::ROOT_ANIMATION || type == ElementType::LAYER_ANIMATIONS ||
                       type == ElementType::NULL_ANIMATIONS || type == ElementType::TRIGGERS;
        if (!isKnown || std::ranges::contains(animation.layout, type))
            return extras_child_add(animation.extras, child, position - skipped);

        animation.layout.push_back(type);
        if (type == ElementType::ROOT_ANIMATION)
            animation.root = track_read(child, ItemType::ROOT);
        if (type == ElementType::TRIGGERS)
            animation.triggers = track_read(child, ItemType::TRIGGER);
        if (type == ElementType::LAYER_ANIMATIONS)
        {
            items[type] = track_items_read(child, ItemType::LAYER);
            animation.layersExtras = extras_read(child);
        }
        if (type == ElementType::NULL_ANIMATIONS)
        {
            items[type] = track_items_read(child, ItemType::NULL_);
            animation.nullsExtras = extras_read(child);
        }
    });

    for (auto [type, entries, extras] :
         {std::tuple{ElementType::LAYER_ANIMATIONS, &animation.layers, &animation.layersExtras},
          std::tuple{ElementType::NULL_ANIMATIONS, &animation.nulls, &animation.nullsExtras}})
    {
        if (!legacyGroups[type].empty() && !std::ranges::contains(animation.layout, type))
            animation.layout.push_back(type);
        if (std::ranges::contains(animation.layout, type))
            *entries = track_entries_build(std::move(items[type]), std::move(legacyGroups[type]), *extras);
    }
    return animation;
}

static std::vector<AnimationEntry> animation_entries_read(pugi::xml_node parent, Extras& extras)
{
    std::vector<AnimationItem> items;
    children_each(parent, [&](pugi::xml_node child, ElementType type, int) {
        if (type == ElementType::GROUP)
        {
            AnimationItem item{.group = AnimationGroup{}};
            XmlReader io{child};
            group_io(io, *item.group, item.index);
            children_each(child, [&](pugi::xml_node animation, ElementType animationType, int) {
                if (animationType == ElementType::ANIMATION)
                    item.children.push_back(std::make_shared<const Animation>(animation_read(animation)));
            });
            items.push_back(std::move(item));
        }
        else if (type == ElementType::ANIMATION)
            items.push_back({.child = std::make_shared<const Animation>(animation_read(child)),
                             .groupId = int_get(child, "GroupId")});
        else
            items.push_back({.node = node_read(child)});
    });
    return entries_build<AnimationEntry>(std::move(items), extras);
}

template <class Item> static std::vector<Item> items_read(pugi::xml_node container, Content& content, ElementType type)
{
    std::vector<Item> items;
    Extras extras = {};
    XmlReader io{container};
    io.extras(extras);
    io.list(items);
    io.close();
    if (extras != Extras{})
        content.containerExtras.emplace_back(type, std::move(extras));
    return items;
}

static Content content_read(pugi::xml_node element)
{
    Content content = {.layout = {}, .extras = extras_read(element)};
    children_each(element, [&](pugi::xml_node child, ElementType type, int position) {
        if (child.name() == LEGACY_GROUPS_TAG)
            return;
        auto isKnown = type == ElementType::SPRITESHEETS || type == ElementType::SHADERS ||
                       type == ElementType::LAYERS || type == ElementType::NULLS || type == ElementType::EVENTS ||
                       type == ElementType::SOUNDS || type == ElementType::LIBRARIES;
        if (!isKnown || std::ranges::contains(content.layout, type))
            return extras_child_add(content.extras, child, position);
        content.layout.push_back(type);
        if (type == ElementType::SPRITESHEETS)
            content.spritesheets = items_read<Spritesheet>(child, content, type);
        if (type == ElementType::SHADERS)
            content.shaders = items_read<Shader>(child, content, type);
        if (type == ElementType::LAYERS)
            content.layers = items_read<Layer>(child, content, type);
        if (type == ElementType::NULLS)
            content.nulls = items_read<Null>(child, content, type);
        if (type == ElementType::EVENTS)
            content.events = items_read<Event>(child, content, type);
        if (type == ElementType::SOUNDS)
            content.sounds = items_read<Sound>(child, content, type);
        if (type == ElementType::LIBRARIES)
            content.libraries = items_read<Library>(child, content, type);
    });
    return content;
}

// Gives every item a unique, non-negative id. The first holder of a duplicated id keeps it.
template <class Items> static void ids_repair(Items& items)
{
    std::set<int> usedIds;
    for (auto& item : items)
        if (item.id >= 0)
            usedIds.insert(item.id);
    std::set<int> keptIds;
    int nextId = 0;
    for (auto& item : items)
    {
        if (item.id >= 0 && keptIds.insert(item.id).second)
            continue;
        while (usedIds.contains(nextId))
            ++nextId;
        item.id = nextId;
        usedIds.insert(nextId);
        keptIds.insert(nextId);
    }
}

// Loaded documents get unique content ids, frames lose dangling shader ids, and layer frames point at the region
// matching their rectangle and take that region's rectangle.
void model_repair(Model& model)
{
    auto& content = model.content;
    ids_repair(content.spritesheets);
    ids_repair(content.shaders);
    ids_repair(content.layers);
    ids_repair(content.nulls);
    ids_repair(content.events);
    ids_repair(content.sounds);
    ids_repair(content.libraries);
    for (auto& spritesheet : content.spritesheets)
        ids_repair(spritesheet.regions);
    for (auto& shader : content.shaders)
        if (shader.name.empty())
            shader.name = SHADER_NAME_DEFAULT;

    for (int i = 0; i < model.animations_count_get(); ++i)
    {
        auto& animation = *model.animation_edit(i);
        animation_tracks_each(animation, [&](Track& track) {
            if (track.type == ItemType::TRIGGER)
                return;
            for (auto& frame : track.frames)
                if (frame.shaderId != -1 && !item_get(content.shaders, frame.shaderId))
                    frame.shaderId = -1;
        });
        items_each(animation.layers, [&](Track& track, auto*) {
            auto layer = item_get(content.layers, track.id);
            auto spritesheet = layer ? item_get(content.spritesheets, layer->spritesheetId) : nullptr;
            if (!spritesheet)
                return;
            for (auto& frame : track.frames)
            {
                auto region = item_get(spritesheet->regions, frame.regionId);
                if (auto match = region_matched_get(spritesheet->regions, frame);
                    match && (!region || !is_region_matched(*region, frame)))
                    frame.regionId = match->id;
                frame_region_apply(frame, spritesheet->regions);
            }
        });
    }
}

static Model model_read(pugi::xml_node element)
{
    Model model = {.layout = {}, .extras = extras_read(element)};
    children_each(element, [&](pugi::xml_node child, ElementType type, int position) {
        auto isKnown = type == ElementType::INFO || type == ElementType::CONTENT || type == ElementType::ANIMATIONS;
        if (!isKnown || std::ranges::contains(model.layout, type))
            return extras_child_add(model.extras, child, position);
        model.layout.push_back(type);
        if (type == ElementType::INFO)
        {
            XmlReader io{child};
            info_io(io, model.info);
            io.close();
        }
        if (type == ElementType::CONTENT)
            model.content = content_read(child);
        if (type == ElementType::ANIMATIONS)
        {
            model.animations.defaultAnimation = string_get(child, "DefaultAnimation");
            model.animations.extras = extras_read(child);
            model.animations.entries = animation_entries_read(child, model.animations.extras);
        }
    });
    model_repair(model);
    return model;
}

// A whole document's frames use renumbered shader and region ids. A layer frame on a region takes its rectangle.
static Frame frame_normalize(const Writer& writer, Frame frame, const Track* layerTrack)
{
    if (!writer.model)
        return frame;
    if (frame.shaderId != -1)
        frame.shaderId = id_map_get(writer.shaderIds, frame.shaderId);
    if (!layerTrack || frame.regionId == -1)
        return frame;

    auto& content = writer.model->content;
    auto layer = item_get(content.layers, layerTrack->id);
    auto spritesheet = layer ? item_get(content.spritesheets, layer->spritesheetId) : nullptr;
    if (!spritesheet)
        return frame;
    auto regionIds = writer.regionIds.find(spritesheet->id);
    frame.regionId = regionIds == writer.regionIds.end() ? -1 : id_map_get(regionIds->second, frame.regionId);
    if (frame.regionId == -1)
        return frame;
    frame_region_rect_set(frame, spritesheet->regions[frame.regionId]);
    return frame;
}

// A backup (FrameRestore) track keeps its frames' region ids as they are.
static void track_write(Sink& sink, const Writer& writer, const Track& track, int groupId = -1, bool isBackup = false)
{
    auto id = track.id;
    if (track.type == ItemType::LAYER && writer.model)
        if (auto it = writer.layerIds.find(id); it != writer.layerIds.end())
            id = it->second;
    XmlWriter io(sink, writer, track_element_type_get(track.type), track.extras, id);
    track_io(io, track, groupId);

    auto layerTrack = track.type == ItemType::LAYER && !isBackup ? &track : nullptr;
    for (auto& frame : track.frames)
    {
        io.children.add();
        frame_write(sink, writer, frame_normalize(writer, frame, layerTrack), track.type);
    }
    io.close();
}

// A group. `index` records its position when groups are written apart from their container, and `members` writes
// the children nested into it.
template <class Group>
void group_write(Sink& sink, const Writer& writer, const Group& group, int index = -1,
                 const std::function<void()>& members = {})
{
    XmlWriter io(sink, writer, ElementType::GROUP, group.extras);
    group_io(io, group, index);
    if constexpr (requires { group.root; })
    {
        io.children.add();
        track_write(sink, writer, group.root);
    }
    io.children.flush();
    if (members)
        members();
    sink.close();
}

struct ContainerChild
{
    std::function<void(int, const std::function<void()>&)> write{};
    int groupId = -1;
    std::optional<int> ownGroupId;
};

template <class Group> static ContainerChild group_child_make(Sink& sink, const Writer& writer, const Group& group)
{
    return {.write = [&](int index,
                         const std::function<void()>& members) { group_write(sink, writer, group, index, members); },
            .ownGroupId = group.id};
}

// Children are laid out flat (a group, then its children) and reshaped for the mode: nested into their groups, or,
// in a track container, moved to a *AnimationGroups sibling (`groupsType`) that records each group's position.
static void container_write(Sink& sink, const Writer& writer, std::vector<ContainerChild> flat, const Extras& extras,
                            ElementType groupsType = ElementType::UNKNOWN)
{
    extras_write(sink, extras);
    for (auto& [index, node] : extras.children)
        flat.insert(flat.begin() + std::min(index, (int)flat.size()),
                    ContainerChild{.write = [&sink, &node](int, const std::function<void()>&) { sink.node(node); }});

    auto isGroups = writer.is_extended();
    std::map<int, std::vector<ContainerChild>> grouped;
    if (writer.mode == SAVE_EDITOR)
    {
        std::set<int> groupIds;
        for (auto& child : flat)
            if (child.ownGroupId && *child.ownGroupId >= 0)
                groupIds.insert(*child.ownGroupId);
        for (auto& child : flat)
            if (!child.ownGroupId && groupIds.contains(child.groupId))
                grouped[child.groupId].push_back(child);
        std::erase_if(
            flat, [&](const ContainerChild& child) { return !child.ownGroupId && groupIds.contains(child.groupId); });
    }

    auto isExtracted = groupsType != ElementType::UNKNOWN && writer.mode == SAVE_CLIPBOARD;
    std::vector<int> extracted;
    for (int i = 0; i < (int)flat.size(); ++i)
    {
        auto& child = flat[i];
        if (child.ownGroupId && !isGroups)
            continue;
        if (child.ownGroupId && isExtracted)
        {
            extracted.push_back(i);
            continue;
        }
        auto members = child.ownGroupId ? grouped.find(*child.ownGroupId) : grouped.end();
        child.write(-1, [&]() {
            if (members != grouped.end())
                for (auto& member : members->second)
                    member.write(-1, {});
        });
    }
    sink.close();

    if (extracted.empty())
        return;
    sink.open(tag_get(groupsType));
    for (auto i : extracted)
        flat[i].write(i, {});
    sink.close();
}

static void track_entries_write(Sink& sink, const Writer& writer, const std::vector<TrackEntry>& entries,
                                ElementType type, const Extras& extras)
{
    std::vector<ContainerChild> flat;
    auto track_child_make = [&](const Track& track, int groupId) {
        return ContainerChild{
            .write = [&, groupId](int, const std::function<void()>&) { track_write(sink, writer, track, groupId); },
            .groupId = groupId};
    };
    entries_walk(
        entries,
        [&](const Track& track, const TrackGroup* group) {
            flat.push_back(track_child_make(track, group ? group->id : -1));
        },
        [&](const TrackGroup& group) { flat.push_back(group_child_make(sink, writer, group)); });
    sink.open(tag_get(type));
    container_write(sink, writer, std::move(flat), extras,
                    type == ElementType::LAYER_ANIMATIONS ? ElementType::LAYER_ANIMATION_GROUPS
                                                          : ElementType::NULL_ANIMATION_GROUPS);
}

static bool is_frame_transform_default(const Frame& frame)
{
    Frame base = {};
    return frame.position == base.position && frame.scale == base.scale && frame.rotation == base.rotation &&
           frame.skew == base.skew && frame.tint == base.tint && frame.colorOffset == base.colorOffset;
}

IsaacIssues isaac_issues_get(const Model& model)
{
    IsaacIssues issues = {};
    issues.animationLayers = (int)std::ranges::count_if(model.content.layers, &Layer::is_animation);
    issues.vectorSpritesheets = (int)spritesheets_vector_used_get(model).size();
    for (auto [index, animation] : model.animations_get())
    {
        for (const auto* entries : {&animation->layers, &animation->nulls})
            groups_each(*entries, [&](const TrackGroup& group) {
                issues.groupTransforms += !std::ranges::all_of(group.root.frames, is_frame_transform_default);
            });
        animation_tracks_each(*animation, [&](const Track& track) {
            for (auto& frame : track.frames)
            {
                issues.easedFrames += track.type != ItemType::TRIGGER && is_special_interpolation(frame.interpolation);
                issues.skewFrames += frame.skew != Vec2();
                issues.shaderFrames += frame.shaderId != -1;
                issues.depthFrames += track.type == ItemType::LAYER && frame.depth != 0;
                issues.alphaOffsetFrames += float_to_uint8(frame.colorOffset.a) != 0;
                issues.blurFrames += track.type == ItemType::LAYER && frame.blur != 0.0f;
                issues.extraEvents += track.type == ItemType::TRIGGER ? std::max((int)frame.eventIds.size() - 1, 0) : 0;
                issues.triggerSounds += track.type == ItemType::TRIGGER &&
                                        std::ranges::any_of(frame.soundIds, [](int id) { return id != -1; });
            }
        });
    }
    return issues;
}

// The game's copy: group roots baked into their tracks (originals kept in a FrameRestore node the editor reads
// back), and interpolation flattened into single-step frames.
static Animation animation_bake(const Writer& writer, Animation animation)
{
    if (writer.mode == SAVE_GAME)
        for (auto* entries : {&animation.layers, &animation.nulls})
            groups_each(*entries, [&](TrackGroup& group) {
                if (std::ranges::all_of(group.root.frames, is_frame_transform_default))
                    return;
                Sink backup = {.mode = Sink::TREE};
                backup.open(FRAME_RESTORE_TAG);
                track_write(backup, writer, group.root);
                for (auto& track : group.items)
                {
                    track_write(backup, writer, track, group.id, true);
                    track_group_root_bake(track, group.root, animation.frameNum);
                }
                backup.close();
                group.root = TrackGroup{}.root;
                // Kept in order of position, where it reads back the same as appended at position 1.
                auto& children = group.extras.children;
                children.insert(std::ranges::lower_bound(children, 1, {}, &std::pair<int, XmlNode>::first),
                                {1, std::move(backup.root)});
            });

    if (writer.mode == SAVE_GAME)
        animation_tracks_each(animation, [](Track& track) {
            if (track.type == ItemType::TRIGGER)
                return;
            for (int index = (int)track.frames.size() - 1; index >= 0; --index)
                if (track.frames[index].interpolation != Interpolation::NONE)
                    frame_bake(track, index, FRAME_DURATION_MIN);
        });
    return animation;
}

// Known children in their recorded order. A part with content that was never in the file is appended.
static std::vector<ElementType> layout_get(std::vector<ElementType> layout,
                                           std::initializer_list<std::pair<ElementType, bool>> parts)
{
    for (auto [type, isContent] : parts)
        if (isContent && !std::ranges::contains(layout, type))
            layout.push_back(type);
    return layout;
}

static void animation_write(Sink& sink, const Writer& writer, const Animation& source, int groupId = -1)
{
    auto isBaked = writer.model && writer.mode == SAVE_GAME;
    auto baked = isBaked ? std::optional(animation_bake(writer, source)) : std::nullopt;
    auto& animation = baked ? *baked : source;

    XmlWriter io(sink, writer, ElementType::ANIMATION, animation.extras);
    animation_io(io, animation, groupId);
    for (auto type : layout_get(animation.layout, {{ElementType::ROOT_ANIMATION, !animation.root.frames.empty()},
                                                   {ElementType::LAYER_ANIMATIONS, !animation.layers.empty()},
                                                   {ElementType::NULL_ANIMATIONS, !animation.nulls.empty()},
                                                   {ElementType::TRIGGERS, !animation.triggers.frames.empty()}}))
    {
        io.children.add();
        if (type == ElementType::ROOT_ANIMATION)
            track_write(sink, writer, animation.root);
        if (type == ElementType::TRIGGERS)
            track_write(sink, writer, animation.triggers);
        if (type == ElementType::LAYER_ANIMATIONS)
            track_entries_write(sink, writer, animation.layers, type, animation.layersExtras);
        if (type == ElementType::NULL_ANIMATIONS)
            track_entries_write(sink, writer, animation.nulls, type, animation.nullsExtras);
    }
    io.close();
}

static void animations_write(Sink& sink, const Writer& writer, const Animations& animations)
{
    std::vector<ContainerChild> flat;
    auto animation_child_make = [&](const Animation& animation, int groupId) {
        return ContainerChild{
            .write = [&, groupId](int,
                                  const std::function<void()>&) { animation_write(sink, writer, animation, groupId); },
            .groupId = groupId};
    };
    entries_walk(
        animations.entries,
        [&](const std::shared_ptr<const Animation>& animation, const AnimationGroup* group) {
            flat.push_back(animation_child_make(*animation, group ? group->id : -1));
        },
        [&](const AnimationGroup& group) { flat.push_back(group_child_make(sink, writer, group)); });
    sink.open(tag_get(ElementType::ANIMATIONS));
    sink.attribute("DefaultAnimation", animations.defaultAnimation);
    container_write(sink, writer, std::move(flat), animations.extras);
}

// A content container's unknown XML (a file may repeat a container. Its parts are merged in order).
static Extras container_extras_get(const Content& content, ElementType type)
{
    Extras merged = {};
    for (auto& [extrasType, extras] : content.containerExtras)
    {
        if (extrasType != type)
            continue;
        merged.attributes.insert(merged.attributes.end(), extras.attributes.begin(), extras.attributes.end());
        if (!extras.text.empty())
            merged.text = extras.text;
        merged.children.insert(merged.children.end(), extras.children.begin(), extras.children.end());
    }
    return merged;
}

template <class Item>
void items_write(Sink& sink, const Writer& writer, const std::vector<Item>& items, ElementType type,
                 const Extras& extras)
{
    XmlWriter io(sink, writer, type, extras);
    io.extras(extras);
    io.list(items);
    io.close();
}

static void content_write(Sink& sink, const Writer& writer, const Content& content)
{
    sink.open(tag_get(ElementType::CONTENT));
    extras_write(sink, content.extras);
    Children children = {sink, content.extras};
    for (auto type : layout_get(content.layout, {{ElementType::SPRITESHEETS, !content.spritesheets.empty()},
                                                 {ElementType::SHADERS, !content.shaders.empty()},
                                                 {ElementType::LAYERS, !content.layers.empty()},
                                                 {ElementType::NULLS, !content.nulls.empty()},
                                                 {ElementType::EVENTS, !content.events.empty()},
                                                 {ElementType::SOUNDS, !content.sounds.empty()},
                                                 {ElementType::LIBRARIES, !content.libraries.empty()}}))
    {
        children.add();
        auto extras = container_extras_get(content, type);
        // Optional containers are left out when the mode drops them or they are empty (their place still counts for
        // where unknown children go).
        if (type == ElementType::SHADERS && writer.is_extended() &&
            (!content.shaders.empty() || !extras.children.empty()))
            items_write(sink, writer, content.shaders, type, extras);
        if (type == ElementType::SOUNDS && writer.is_extended() &&
            (!content.sounds.empty() || !extras.children.empty()))
            items_write(sink, writer, content.sounds, type, extras);
        if (type == ElementType::LIBRARIES && writer.is_extended() &&
            (!content.libraries.empty() || !extras.children.empty()))
            items_write(sink, writer, content.libraries, type, extras);
        if (type == ElementType::SPRITESHEETS)
            items_write(sink, writer, content.spritesheets, type, extras);
        if (type == ElementType::LAYERS)
            items_write(sink, writer, content.layers, type, extras);
        if (type == ElementType::NULLS)
            items_write(sink, writer, content.nulls, type, extras);
        if (type == ElementType::EVENTS)
            items_write(sink, writer, content.events, type, extras);
    }
    children.close();
}

// A whole document. `inner` writes anything after its children (the SourceDocument copy) and `tag` renames it.
static void model_write(Sink& sink, const Model& model, SaveMode mode, const std::function<void()>& inner = {},
                        std::string_view tag = {})
{
    Writer writer(mode, &model);
    sink.open(tag.empty() ? tag_get(ElementType::ANIMATED_ACTOR) : tag);
    extras_write(sink, model.extras);
    Children children = {sink, model.extras};
    auto isIgnored = writer.is_extended() && model.info.isIsaacIgnored;
    auto isInfo = model.info.fps != Info{}.fps || model.info.createdBy != Info{}.createdBy || isIgnored;
    for (auto type : layout_get(model.layout, {{ElementType::INFO, isInfo},
                                               {ElementType::CONTENT, true},
                                               {ElementType::ANIMATIONS, !model.animations.entries.empty()}}))
    {
        children.add();
        if (type == ElementType::CONTENT)
            content_write(sink, writer, model.content);
        if (type == ElementType::ANIMATIONS)
            animations_write(sink, writer, model.animations);
        if (type != ElementType::INFO)
            continue;
        XmlWriter io(sink, writer, ElementType::INFO, model.info.extras);
        info_io(io, model.info);
        io.close();
    }
    children.flush();
    if (inner)
        inner();
    sink.close();
}

// A save is the game's document with the editor's own copy embedded as SourceDocument. Neither writes tints of 255
// or color offsets of 0, which readers assume.
static void document_write(Sink& sink, const Model& model, Options options)
{
    model_write(sink, options.gameModel ? *options.gameModel : model, SAVE_GAME,
                [&]() { model_write(sink, model, SAVE_EDITOR, {}, SOURCE_DOCUMENT_TAG); });
}

// The text of whatever `write` sends to a sink.
template <class Write> static std::string text_get(Write&& write)
{
    Sink sink = {};
    write(sink);
    return std::move(sink.text);
}

static bool model_invalid_set(Model& model)
{
    model = model_make();
    model.isValid = false;
    return false;
}

// A parse error says where it is: "Error parsing element name (line 12)".
static bool document_parse(pugi::xml_document& document, std::string_view text, std::string* errorString)
{
    auto result = document.load_buffer(text.data(), text.size());
    if (result)
        return true;
    if (errorString)
        *errorString = std::format("{} (line {})", result.description(),
                                   std::ranges::count(text.substr(0, result.offset), '\n') + 1);
    return false;
}

static bool document_load(Model& model, std::string_view text, std::string* errorString)
{
    pugi::xml_document document;
    if (!document_parse(document, text, errorString))
        return model_invalid_set(model);
    auto root = document.document_element();
    if (!root)
    {
        if (errorString)
            *errorString = "No root element.";
        return model_invalid_set(model);
    }
    auto source = root.name() == SOURCE_DOCUMENT_TAG ? root : root.child(SOURCE_DOCUMENT_TAG.data());
    // A document without one (as the old extended format saved) is read as the editor's own.
    model = model_read(source ? source : root);
    return true;
}

Model model_make()
{
    Model model = {};
    model.info.createdOn = timestamp_get(CREATED_ON_FORMAT);
    return model;
}

bool model_load(Model& model, const std::filesystem::path& path, std::string* errorString)
{
    std::string text;
    if (file_text_load(path, &text))
        return model_load_string(model, text, errorString);
    if (errorString)
        *errorString = "File not found.";
    return model_invalid_set(model);
}

bool model_load_string(Model& model, std::string_view text, std::string* errorString)
{
    if (is_json(text))
        return model_from_json(model, text, errorString) || model_invalid_set(model);
    return document_load(model, text, errorString);
}

bool model_save(const Model& model, const std::filesystem::path& path, std::string* errorString, Options options)
{
    if (file_text_save(path, model_to_string(model, options)))
        return true;
    if (errorString)
        *errorString = "Could not write the file.";
    return false;
}

std::string model_to_string(const Model& model, Options options)
{
    if (options.isJson)
        return model_to_json(model);
    return text_get([&](Sink& sink) { document_write(sink, model, options); });
}

std::string model_serialize(const Model& model, SaveMode mode)
{
    return text_get([&](Sink& sink) { model_write(sink, model, mode); });
}

// The hash of what the editor would save, so it changes exactly when a save would.
std::uint64_t model_hash(const Model& model)
{
    Sink sink = {.mode = Sink::HASH};
    model_write(sink, model, SAVE_EDITOR);
    return sink.hash;
}

std::string frame_to_string(const Frame& frame, ItemType type)
{
    return text_get([&](Sink& sink) { frame_write(sink, Writer(SAVE_CLIPBOARD), frame, type); });
}

std::string animation_to_string(const Animation& animation, int groupId)
{
    return text_get([&](Sink& sink) { animation_write(sink, Writer(SAVE_CLIPBOARD), animation, groupId); });
}

std::string animation_group_to_string(const AnimationGroup& group)
{
    return text_get([&](Sink& sink) { group_write(sink, Writer(SAVE_CLIPBOARD), group); });
}

std::vector<Frame> frames_from_string(const std::string& text, ItemType type, std::string* errorString)
{
    pugi::xml_document document;
    if (!document_parse(document, text, errorString))
        return {};
    auto tag = tag_get(type == ItemType::TRIGGER ? ElementType::TRIGGER : ElementType::FRAME);
    std::vector<Frame> frames;
    for (auto element : document.children(tag))
        frames.push_back(frame_read(element, type));
    if (frames.empty() && errorString)
        *errorString = std::format("No valid {}(s).", tag);
    return frames;
}

std::vector<AnimationEntry> animations_from_string(const std::string& text, std::string* errorString)
{
    pugi::xml_document document;
    if (!document_parse(document, text, errorString))
        return {};
    Extras extras = {};
    auto entries = animation_entries_read(document, extras);
    if (entries.empty() && errorString)
        *errorString = "No valid animation(s).";
    return entries;
}

template <class Item> std::string item_to_string(const Item& item)
{
    return text_get([&](Sink& sink) { element_write(sink, Writer(SAVE_CLIPBOARD), item, item.id); });
}

template <class Item> std::vector<Item> items_from_string(const std::string& text, std::string* errorString)
{
    pugi::xml_document document;
    if (!document_parse(document, text, errorString))
        return {};
    auto tag = tag_get(ITEM_TYPE<Item>);
    std::vector<Item> items;
    for (auto element : document.children(tag))
    {
        auto item = element_read<Item>(element);
        if constexpr (requires { item.path; })
            item.path = path_backslash_handle(item.path);
        items.push_back(std::move(item));
    }
    if (items.empty() && errorString)
        *errorString = std::format("No valid {}(s).", tag);
    return items;
}

#define X(Item)                                                                                                        \
    template std::string item_to_string(const Item&);                                                                  \
    template std::vector<Item> items_from_string(const std::string&, std::string*);
X(Spritesheet)
X(Region)
X(Shader)
X(Layer)
X(Null)
X(Event)
X(Sound)
X(Library)
#undef X
