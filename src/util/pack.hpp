#pragma once

#include <vector>

#include <glm/glm.hpp>

namespace anm2ed::util::pack
{
  bool rects_pack(const std::vector<glm::ivec2>&, glm::ivec2&, std::vector<glm::ivec2>&);
}
