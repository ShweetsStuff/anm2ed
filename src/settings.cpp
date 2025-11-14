#include "settings.h"

#include "filesystem_.h"
#include "log.h"

using namespace anm2ed::util;
using namespace glm;

namespace anm2ed
{
  constexpr auto IMGUI_DEFAULT = R"(
# Dear ImGui
[Window][##DockSpace]
Pos=0,54
Size=1918,1010
Collapsed=0

[Window][Animation Preview]
Pos=60,62
Size=983,691
Collapsed=0
DockId=0x00000003,1

[Window][Animations]
Pos=1451,494
Size=459,259
Collapsed=0
DockId=0x0000000C,0

[Window][Events]
Pos=1045,463
Size=404,290
Collapsed=0
DockId=0x00000008,0

[Window][Frame Properties]
Pos=1045,62
Size=404,399
Collapsed=0
DockId=0x00000007,0

[Window][Layers]
Pos=1045,463
Size=404,290
Collapsed=0
DockId=0x00000008,1

[Window][Nulls]
Pos=1045,463
Size=404,290
Collapsed=0
DockId=0x00000008,2

[Window][Onionskin]
Pos=8,755
Size=1902,301
Collapsed=0
DockId=0x00000006,1

[Window][Spritesheet Editor]
Pos=60,62
Size=983,691
Collapsed=0
DockId=0x00000003,0

[Window][Spritesheets]
Pos=1451,62
Size=459,430
Collapsed=0
DockId=0x0000000B,0

[Window][Tools]
Pos=8,62
Size=50,691
Collapsed=0
DockId=0x00000001,0

[Window][Timeline]
Pos=8,755
Size=1902,301
Collapsed=0
DockId=0x00000006,0

[Window][Sounds]
Pos=1045,463
Size=404,290
Collapsed=0
DockId=0x00000008,3

[Docking][Data]
DockSpace           ID=0x123F8F08 Window=0x6D581B32 Pos=8,62 Size=1902,994 Split=Y Selected=0x4EFD0020
  DockNode          ID=0x00000005 Parent=0x123F8F08 SizeRef=1910,691 Split=X
    DockNode        ID=0x00000001 Parent=0x00000005 SizeRef=50,994 Selected=0x18A5FDB9
    DockNode        ID=0x00000002 Parent=0x00000005 SizeRef=1850,994 Split=X Selected=0x4EFD0020
      DockNode      ID=0x00000003 Parent=0x00000002 SizeRef=983,994 Selected=0x024430EF
      DockNode      ID=0x00000004 Parent=0x00000002 SizeRef=865,994 Split=X Selected=0x4EFD0020
        DockNode    ID=0x00000009 Parent=0x00000004 SizeRef=404,497 Split=Y Selected=0xCD8384B1
          DockNode  ID=0x00000007 Parent=0x00000009 SizeRef=181,399 Selected=0x754E368F
          DockNode  ID=0x00000008 Parent=0x00000009 SizeRef=181,290 Selected=0x8A65D963
        DockNode    ID=0x0000000A Parent=0x00000004 SizeRef=459,497 Split=Y Selected=0x4EFD0020
          DockNode  ID=0x0000000B Parent=0x0000000A SizeRef=710,430 CentralNode=1 Selected=0x4EFD0020
          DockNode  ID=0x0000000C Parent=0x0000000A SizeRef=710,259 Selected=0xC1986EE2
  DockNode          ID=0x00000006 Parent=0x123F8F08 SizeRef=1910,301 Selected=0x4F89F0DC
)";

  Settings::Settings(const std::string& path)
  {
    if (filesystem::path_is_exist(path))
      logger.info(std::format("Using settings from: {}", path));
    else
    {
      logger.warning("Settings file does not exist; using default");
      save(path, IMGUI_DEFAULT);
      isDefault = true;
    }

    std::ifstream file(path);
    if (!file.is_open())
    {
      logger.error(std::format("Failed to open settings file: {}", path));
      return;
    }

    std::string line{};

    auto stream_assign = [](auto& dest, std::istringstream& ss) { ss >> dest; };

    auto value_set = [&](auto& dest, std::istringstream& ss)
    {
      using T = std::decay_t<decltype(dest)>;

      if constexpr (std::is_same_v<T, bool>)
      {
        std::string val;
        stream_assign(val, ss);
        dest = (val == "true" || val == "1");
      }
      else if constexpr (std::is_same_v<T, std::string>)
        std::getline(ss, dest);
      else
        stream_assign(dest, ss);
    };

    auto entry_load =
        [&](const std::string& key, std::istringstream& ss, const std::string& name, auto& value, std::string_view type)
    {
      using T = std::decay_t<decltype(value)>;

      auto is_match = [&](const char* suffix) { return key == name + suffix; };

      if constexpr (std::is_same_v<T, ivec2> || std::is_same_v<T, vec2>)
      {
        if (type.ends_with("_WH"))
        {
          if (is_match("W"))
          {
            stream_assign(value.x, ss);
            return true;
          }
          if (is_match("H"))
          {
            stream_assign(value.y, ss);
            return true;
          }
        }
        else
        {
          if (is_match("X"))
          {
            stream_assign(value.x, ss);
            return true;
          }
          if (is_match("Y"))
          {
            stream_assign(value.y, ss);
            return true;
          }
        }
      }
      else if constexpr (std::is_same_v<T, vec3>)
      {
        if (is_match("R"))
        {
          stream_assign(value.x, ss);
          return true;
        }
        if (is_match("G"))
        {
          stream_assign(value.y, ss);
          return true;
        }
        if (is_match("B"))
        {
          stream_assign(value.z, ss);
          return true;
        }
      }
      else if constexpr (std::is_same_v<T, vec4>)
      {
        if (is_match("R"))
        {
          stream_assign(value.x, ss);
          return true;
        }
        if (is_match("G"))
        {
          stream_assign(value.y, ss);
          return true;
        }
        if (is_match("B"))
        {
          stream_assign(value.z, ss);
          return true;
        }
        if (is_match("A"))
        {
          stream_assign(value.w, ss);
          return true;
        }
      }
      else
      {
        if (key == name)
        {
          value_set(value, ss);
          return true;
        }
      }

      return false;
    };

    while (std::getline(file, line))
    {
      if (line == "[Settings]" || line.empty()) continue;
      if (line == "# Dear ImGui") break;

      auto eq = line.find('=');
      if (eq == std::string::npos) continue;

      auto key = line.substr(0, eq);
      std::istringstream ss(line.substr(eq + 1));

#define X(symbol, name, string, type, ...)                                                                             \
  if (entry_load(key, ss, #name, name, #type)) continue;
      SETTINGS_MEMBERS SETTINGS_SHORTCUTS SETTINGS_WINDOWS
#undef X
    }

    file.close();
  }

  void Settings::save(const std::string& path, const std::string& imguiData)
  {
    std::ofstream file(path, std::ios::out | std::ios::binary);
    file << "[Settings]\n";

    auto value_save = [&](const std::string& key, const auto& value)
    {
      using T = std::decay_t<decltype(value)>;

      if constexpr (std::is_same_v<T, bool>)
        file << key << "=" << (value ? "true" : "false") << "\n";
      else
        file << key << "=" << value << "\n";
    };
    auto entry_save = [&](const std::string& name, const auto& value, const std::string_view type)
    {
      using T = std::decay_t<decltype(value)>;

      if constexpr (std::is_same_v<T, ivec2> || std::is_same_v<T, vec2>)
      {
        if (type.ends_with("_WH"))
        {
          value_save(name + "W", value.x);
          value_save(name + "H", value.y);
        }
        else
        {
          value_save(name + "X", value.x);
          value_save(name + "Y", value.y);
        }
      }
      else if constexpr (std::is_same_v<T, vec3>)
      {
        value_save(name + "R", value.x);
        value_save(name + "G", value.y);
        value_save(name + "B", value.z);
      }
      else if constexpr (std::is_same_v<T, vec4>)
      {
        value_save(name + "R", value.x);
        value_save(name + "G", value.y);
        value_save(name + "B", value.z);
        value_save(name + "A", value.w);
      }
      else
        value_save(name, value);
    };

#define X(symbol, name, string, type, ...) entry_save(#name, name, #type);
    SETTINGS_MEMBERS SETTINGS_SHORTCUTS SETTINGS_WINDOWS
#undef X

            file
        << "\n# Dear ImGui\n"
        << imguiData;

    file.flush();
    file.close();
  }
}