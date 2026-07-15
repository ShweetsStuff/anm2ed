#include "settings.hpp"

#include <fstream>
#include <sstream>

#include "log.hpp"
#include "path.hpp"

using namespace anm2ed::util;
using namespace glm;

namespace anm2ed
{
  constexpr auto IMGUI_DEFAULT = R"(
[Window][##DockSpace]
Pos=0,54
Size=1918,1007
Collapsed=0

[Window][##Documents]
Pos=0,22
Size=1918,32
Collapsed=0

[Window][###Onionskin]
Pos=8,724
Size=1902,329
Collapsed=0
DockId=0x00000016,1

[Window][###Animation Preview]
Pos=60,62
Size=898,660
Collapsed=0
DockId=0x00000013,0

[Window][###Sounds]
Pos=1166,62
Size=744,305
Collapsed=0
DockId=0x0000000F,1

[Window][###Animations]
Pos=1166,369
Size=179,353
Collapsed=0
DockId=0x00000011,0

[Window][###Events]
Pos=960,413
Size=204,309
Collapsed=0
DockId=0x00000006,1

[Window][###Frame Properties]
Pos=960,62
Size=204,349
Collapsed=0
DockId=0x00000005,0

[Window][###Layers]
Pos=960,413
Size=204,309
Collapsed=0
DockId=0x00000006,0

[Window][###Nulls]
Pos=960,413
Size=204,309
Collapsed=0
DockId=0x00000006,2

[Window][###Spritesheet Editor]
Pos=60,62
Size=898,660
Collapsed=0
DockId=0x00000013,1

[Window][###Spritesheets]
Pos=1166,62
Size=744,305
Collapsed=0
DockId=0x0000000F,0

[Window][###Tools]
Pos=8,62
Size=50,660
Collapsed=0
DockId=0x00000004,0

[Window][###Welcome]
Pos=0,54
Size=1918,1007
Collapsed=0

[Window][###Timeline]
Pos=8,724
Size=1902,329
Collapsed=0
DockId=0x00000016,0

[Window][###Regions]
Pos=1347,369
Size=563,353
Collapsed=0
DockId=0x00000014,0

[Window][###Shaders]
Pos=1347,369
Size=563,353
Collapsed=0
DockId=0x00000014,1

[Window][###Overlays]
Pos=1347,369
Size=563,353
Collapsed=0
DockId=0x00000014,2

[Table][0xE5891919,5]
Column 0  Weight=1.3608
Column 1  Weight=0.8522
Column 2  Weight=0.7973
Column 3  Weight=0.9897
Column 4  Weight=0.5671

[Table][0xB03AD869,4]
Column 0  Weight=0.9635
Column 1  Weight=0.6034
Column 2  Weight=0.5742
Column 3  Weight=1.8589

[Table][0x6BD17CEE,4]
Column 0  Weight=0.9451
Column 1  Weight=0.5919
Column 2  Weight=0.5823
Column 3  Weight=1.8807

[Docking][Data]
DockSpace                 ID=0x123F8F08 Window=0x6D581B32 Pos=8,62 Size=1902,991 Split=Y Selected=0x4EFD0020
  DockNode                ID=0x00000015 Parent=0x123F8F08 SizeRef=1902,660 Split=X
    DockNode              ID=0x00000004 Parent=0x00000015 SizeRef=50,991 Selected=0xA82FD886
    DockNode              ID=0x00000012 Parent=0x00000015 SizeRef=1850,991 Split=X
      DockNode            ID=0x00000013 Parent=0x00000012 SizeRef=898,991 Selected=0xDDB7ACD5
      DockNode            ID=0x00000003 Parent=0x00000012 SizeRef=950,991 Split=X
        DockNode          ID=0x00000001 Parent=0x00000003 SizeRef=50,994 Selected=0x18A5FDB9
        DockNode          ID=0x00000002 Parent=0x00000003 SizeRef=1850,994 Split=X Selected=0x4EFD0020
          DockNode        ID=0x00000009 Parent=0x00000002 SizeRef=204,497 Split=Y Selected=0xCD8384B1
            DockNode      ID=0x00000007 Parent=0x00000009 SizeRef=181,275 Selected=0x754E368F
            DockNode      ID=0x00000008 Parent=0x00000009 SizeRef=181,278 Split=Y Selected=0xE42526D3
              DockNode    ID=0x00000005 Parent=0x00000008 SizeRef=204,349 Selected=0x6DE1CB13
              DockNode    ID=0x00000006 Parent=0x00000008 SizeRef=204,309 Selected=0xE42526D3
          DockNode        ID=0x0000000A Parent=0x00000002 SizeRef=744,497 Split=Y Selected=0x4EFD0020
            DockNode      ID=0x0000000B Parent=0x0000000A SizeRef=459,303 Split=Y Selected=0xD302131E
              DockNode    ID=0x0000000F Parent=0x0000000B SizeRef=586,305 CentralNode=1 Selected=0xD302131E
              DockNode    ID=0x00000010 Parent=0x0000000B SizeRef=586,353 Split=X Selected=0x9B32B74A
                DockNode  ID=0x00000011 Parent=0x00000010 SizeRef=179,293 Selected=0x0ED5C382
                DockNode  ID=0x00000014 Parent=0x00000010 SizeRef=563,293 Selected=0x3732FDF0
            DockNode      ID=0x0000000C Parent=0x0000000A SizeRef=459,250 Selected=0xC1986EE2
  DockNode                ID=0x00000016 Parent=0x123F8F08 SizeRef=1902,329 Selected=0x597925B7



  
)";

  Settings::Settings(const std::filesystem::path& path)
  {
    auto pathUtf8 = path::to_utf8(path);
    if (path::is_exist(path))
      logger.info(std::format("Using settings from: {}", pathUtf8));
    else
    {
      logger.warning("Settings file does not exist; using default");
      save(path, IMGUI_DEFAULT);
      isDefault = true;
    }

    std::ifstream file(path);
    if (!file.is_open())
    {
      logger.error(std::format("Failed to open settings file: {}", pathUtf8));
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
      {
        std::getline(ss, dest);
      }
      else if constexpr (std::is_same_v<T, std::filesystem::path>)
      {
        std::string value;
        std::getline(ss, value);
        dest = path::from_utf8(value);
      }
      else
      {
        stream_assign(dest, ss);
      }
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

  Options Settings::anm2_options_get() const
  {
    return {.flags = SERIALIZE_DEFAULT, .isExtendedFormat = isFileExtendedFormat};
  }

  std::string Settings::imgui_data_load(const std::filesystem::path& path)
  {
    auto pathUtf8 = path::to_utf8(path);
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file.is_open())
    {
      logger.error(
          std::format("Failed to open settings file for Dear ImGui data: {}; using Dear ImGui defaults", pathUtf8));
      return {};
    }

    std::string line{};
    std::ostringstream dataStream;
    bool isImGuiSection = false;
    bool isContent = false;

    while (std::getline(file, line))
    {
      if (!isImGuiSection)
      {
        if (line == "# Dear ImGui") isImGuiSection = true;
        continue;
      }

      if (isContent) dataStream << "\n";
      dataStream << line;
      isContent = true;
    }

    if (!isImGuiSection)
    {
      logger.warning(
          std::format("Dear ImGui section missing from settings file: {}; using Dear ImGui defaults", pathUtf8));
      return {};
    }

    if (!isContent)
    {
      logger.warning(std::format("Dear ImGui section empty in settings file: {}; using Dear ImGui defaults", pathUtf8));
      return {};
    }

    return dataStream.str();
  }

  void Settings::save(const std::filesystem::path& path, const std::string& imguiData)
  {
    auto pathUtf8 = path::to_utf8(path);
    logger.info(std::format("Saving settings to: {}", pathUtf8));
    std::ofstream file(path, std::ios::out | std::ios::binary);
    if (!file.is_open())
    {
      logger.error(std::format("Failed to save settings file: {}", pathUtf8));
      return;
    }

    file << "[Settings]\n";

    auto value_save = [&](const std::string& key, const auto& value)
    {
      using T = std::decay_t<decltype(value)>;

      if constexpr (std::is_same_v<T, bool>)
        file << key << "=" << (value ? "true" : "false") << "\n";
      else if constexpr (std::is_same_v<T, std::filesystem::path>)
        file << key << "=" << path::to_utf8(value) << "\n";
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
    if (file.fail())
    {
      logger.error(std::format("Failed while writing settings file: {}", pathUtf8));
      return;
    }
    file.close();
    logger.info(std::format("Saved settings to: {}", pathUtf8));
  }
}
