#include "settings.h"

static void _settings_setting_load(Settings* self, const std::string& line)
{
    for (s32 i = 0; i < SETTINGS_COUNT; i++)
    {
        const auto& entry = SETTINGS_ENTRIES[i];
        const std::string& key = entry.key;

        void* target = (u8*)self + entry.offset;

        auto match_key = [&](const std::string& full) -> const char*
        {
            if (!line.starts_with(full)) 
                return nullptr;
            size_t p = full.size();
            while (p < line.size() && std::isspace((u8)line[p])) ++p;
            if (p < line.size() && line[p] == '=') return line.c_str() + p + 1;
            return nullptr;
        };

        auto* value = match_key(key);

        if (value)
        {
            switch (entry.type)
            {
                case TYPE_INT:    
                    *(s32*)target = std::atoi(value); 
                    return;
                case TYPE_BOOL:   
                    *(bool*)target = string_to_bool(value); 
                    return;
                case TYPE_FLOAT:  
                    *(f32*)target = std::atof(value); 
                    return;
                case TYPE_STRING: 
                    *(std::string*)target = value; 
                    return;
                default: 
                    break;
            }
        }

        if (entry.type == TYPE_VEC2)
        {
            vec2* v = (vec2*)target;
            if ((value = match_key(key + (entry.isWidthHeight ? "W" : "X")))) { v->x = std::atof(value); return; }
            if ((value = match_key(key + (entry.isWidthHeight ? "H" : "Y")))) { v->y = std::atof(value); return; }
        }
        else if (entry.type == TYPE_IVEC2)
        {
            ivec2* v = (ivec2*)target;
            if ((value = match_key(key + (entry.isWidthHeight ? "W" : "X")))) { v->x = std::atoi(value); return; }
            if ((value = match_key(key + (entry.isWidthHeight ? "H" : "Y")))) { v->y = std::atoi(value); return; }
        }
        else if (entry.type == TYPE_VEC3)
        {
            vec3* v = (vec3*)target;
            if ((value = match_key(key + "R"))) { v->x = std::atof(value); return; }
            if ((value = match_key(key + "G"))) { v->y = std::atof(value); return; }
            if ((value = match_key(key + "B"))) { v->z = std::atof(value); return; }
        }
        else if (entry.type == TYPE_VEC4)
        {
            vec4* v = (vec4*)target;
            if ((value = match_key(key + "R"))) { v->x = std::atof(value); return; }
            if ((value = match_key(key + "G"))) { v->y = std::atof(value); return; }
            if ((value = match_key(key + "B"))) { v->z = std::atof(value); return; }
            if ((value = match_key(key + "A"))) { v->w = std::atof(value); return; }
        }
    }
}

std::string settings_path_get(void)
{
    std::string filePath = preferences_path_get() + SETTINGS_PATH;
    return filePath;
}

static void _settings_setting_write(Settings* self, std::ostream& out, SettingsEntry entry)
{
    u8* selfPointer = (u8*)self;
    std::string value;

    switch (entry.type)
    {
        case TYPE_INT:
            value = std::format("{}", *(s32*)(selfPointer + entry.offset));
            out << entry.key << "=" << value << "\n";
            break;
        case TYPE_BOOL:
            value = std::format("{}", *(bool*)(selfPointer + entry.offset));
            out << entry.key << "=" << value << "\n";
            break;
        case TYPE_FLOAT:
            value = std::format("{:.3f}", *(f32*)(selfPointer + entry.offset));
            out << entry.key << "=" << value << "\n";
            break;
        case TYPE_STRING:
        {
            const std::string data = *reinterpret_cast<const std::string*>(selfPointer + entry.offset);
            if (!data.empty())
                out << entry.key << "=" << data.c_str() << "\n";
            break;
        }
        case TYPE_IVEC2:
        {
            ivec2* data = (ivec2*)(selfPointer + entry.offset);
            out << entry.key << (entry.isWidthHeight ? "W=" : "X=") << data->x << "\n";
            out << entry.key << (entry.isWidthHeight ? "H=" : "Y=") << data->y << "\n";
            break;
        }
        case TYPE_VEC2:
        {
            vec2* data = (vec2*)(selfPointer + entry.offset);
            out << entry.key << (entry.isWidthHeight ? "W=" : "X=") << std::format(SETTINGS_FLOAT_FORMAT, data->x) << "\n";
            out << entry.key << (entry.isWidthHeight ? "H=" : "Y=") << std::format(SETTINGS_FLOAT_FORMAT, data->y) << "\n";
            break;
        }
        case TYPE_VEC3:
        {
            vec3* data = (vec3*)(selfPointer + entry.offset);
            out << entry.key << "R=" << std::format(SETTINGS_FLOAT_FORMAT, data->r) << "\n";
            out << entry.key << "G=" << std::format(SETTINGS_FLOAT_FORMAT, data->g) << "\n";
            out << entry.key << "B=" << std::format(SETTINGS_FLOAT_FORMAT, data->b) << "\n";
            break;
        }
        case TYPE_VEC4:
        {
            vec4* data = (vec4*)(selfPointer + entry.offset);
            out << entry.key << "R=" << std::format(SETTINGS_FLOAT_FORMAT, data->r) << "\n";
            out << entry.key << "G=" << std::format(SETTINGS_FLOAT_FORMAT, data->g) << "\n";
            out << entry.key << "B=" << std::format(SETTINGS_FLOAT_FORMAT, data->b) << "\n";
            out << entry.key << "A=" << std::format(SETTINGS_FLOAT_FORMAT, data->a) << "\n";
            break;
        }
        default:
            break;
    }
}

void settings_save(Settings* self)
{
    const std::string path = settings_path_get();
    const std::filesystem::path filesystemPath(path);
    const std::filesystem::path directory = filesystemPath.parent_path();

    if (!directory.empty()) 
    {
        std::error_code errorCode;
        std::filesystem::create_directories(directory, errorCode);
        if (errorCode) 
        {
            log_error(std::format(SETTINGS_DIRECTORY_ERROR, directory.string(), errorCode.message()));
            return;
        }
    }

    std::string data;
    if (std::filesystem::exists(filesystemPath)) 
    {
        if (std::ifstream in(path, std::ios::binary); in)
            data.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    }

    std::filesystem::path temp = filesystemPath;
    temp += SETTINGS_TEMPORARY_EXTENSION;

    std::ofstream out(temp, std::ios::binary | std::ios::trunc);
    if (!out) 
    {
        log_error(std::format(SETTINGS_INIT_ERROR, temp.string()));
        return;
    }

    out << SETTINGS_SECTION << "\n";
    for (s32 i = 0; i < SETTINGS_COUNT; i++)
        _settings_setting_write(self, out, SETTINGS_ENTRIES[i]);

    out << "\n" << SETTINGS_SECTION_IMGUI << "\n";
    out << data;

    out.flush();

    if (!out.good()) 
    {
        log_error(std::format(SETTINGS_SAVE_ERROR, temp.string()));
        return;
    }

    out.close();

    std::error_code errorCode;
    std::filesystem::rename(temp, filesystemPath, errorCode);
    if (errorCode) 
    {
        // Windows can block rename if target exists; try remove+rename
        std::filesystem::remove(filesystemPath, errorCode);
        errorCode = {};
        std::filesystem::rename(temp, filesystemPath, errorCode);
        if (errorCode) 
        {
            log_error(std::format(SETTINGS_SAVE_FINALIZE_ERROR, filesystemPath.string(), errorCode.message()));
            std::filesystem::remove(temp);
            return;
        }
    }

    log_info(std::format(SETTINGS_SAVE_INFO, path));
}

void settings_init(Settings* self)
{
    const std::string path = settings_path_get();
    std::ifstream file(path, std::ios::binary);
    std::istream* in = nullptr;
    std::istringstream defaultSettings;
    
    if (file)
    {
        log_info(std::format(SETTINGS_INIT_INFO, path));
        in = &file; 
    }
    else
    {
        log_error(std::format(SETTINGS_INIT_ERROR, path));
        log_info(SETTINGS_DEFAULT_INFO);
        defaultSettings.str(SETTINGS_DEFAULT);
        in = &defaultSettings;
    }

    std::string line;
    bool inSettingsSection = false;

    while (std::getline(*in, line)) 
    { 
        if (line == SETTINGS_SECTION)
        { 
            inSettingsSection = true; 
            continue; 
        } 
        if (line == SETTINGS_SECTION_IMGUI) break; 
        if (inSettingsSection) _settings_setting_load(self, line); 
    }

    // Save default settings
    if (!file) 
    {
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        if (out) 
        {
            out << SETTINGS_DEFAULT;
            out.flush();
            log_info(std::format(SETTINGS_SAVE_INFO, path));
        } 
        else
            log_error(std::format(SETTINGS_DEFAULT_ERROR, path));
    }
}