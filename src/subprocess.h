#pragma once

#include <string>

class Process
{
public:
    Process() = default;
    Process(const char*, const char*);
    Process(const Process&) = delete;
    Process& operator=(const Process&) = delete;
    ~Process();

    bool open(const char*, const char*);
    int close();
    FILE* get() const;
    explicit operator bool() const;

    std::string output_get_and_close();

private:
    FILE* pipe = nullptr;
};
