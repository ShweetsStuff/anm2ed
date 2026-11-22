#pragma once

#include <filesystem>

class WorkingDirectory
{
public:
    enum Type
    {
        FILE,
        DIRECTORY
    };

    std::filesystem::path previous;

    WorkingDirectory(const std::filesystem::path&, Type type = DIRECTORY);
    ~WorkingDirectory();
};
