#pragma once

#include <optional>
#include <string>
#include <vector>

enum InstanceRole
{
    INSTANCE_FIRST,
    INSTANCE_FORWARDED,
    INSTANCE_ALONE
};

// One editor at a time. The first listens on a local port for the paths a later one hands it, and that later one
// exits. Each hand-off pushes an SDL_EVENT_USER to wake the event loop. When something else holds the port, it runs
// alone.
InstanceRole instance_start(const std::vector<std::string>& paths);
// The paths handed over since the last call (empty for a launch without any), or nothing without a hand-off.
std::optional<std::vector<std::string>> instance_paths_take();
void instance_stop();
