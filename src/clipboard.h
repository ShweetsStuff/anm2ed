#pragma once

#include <string>

class Clipboard
{
public:
    Clipboard();
    bool is_empty();
    std::string get();
    void set(const std::string&);
};
