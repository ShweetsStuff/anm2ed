#pragma once

#include <string>
#include <vector>

enum InstanceRole
{
    INSTANCE_FIRST,
    INSTANCE_FORWARDED,
    INSTANCE_ALONE
};

// One editor at a time. The first listens on a local port and pushes an SDL_EVENT_USER (data1, an SDL_strdup'd path)
// for each path a later one hands it, and that later one exits. When something else holds the port, it runs alone.
InstanceRole instance_start(const std::vector<std::string>& paths);
void instance_stop();
