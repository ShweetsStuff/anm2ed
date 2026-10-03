#include "pack.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

namespace anm2ed::util::pack
{
  constexpr int WIDTH_CANDIDATES_MAX = 512;

  struct Rect
  {
    int x{};
    int y{};
    int w{};
    int h{};
  };

  bool is_rect_intersecting(const Rect& a, const Rect& b)
  {
    return !(b.x >= a.x + a.w || b.x + b.w <= a.x || b.y >= a.y + a.h || b.y + b.h <= a.y);
  }

  bool is_rect_contained(const Rect& a, const Rect& b)
  {
    return b.x >= a.x && b.y >= a.y && b.x + b.w <= a.x + a.w && b.y + b.h <= a.y + a.h;
  }

  void free_rects_split(std::vector<Rect>& freeRects, const Rect& used)
  {
    std::vector<Rect> next{};
    next.reserve(freeRects.size() * 2);
    for (auto& free : freeRects)
    {
      if (!is_rect_intersecting(free, used))
      {
        next.push_back(free);
        continue;
      }
      if (used.x > free.x) next.push_back({free.x, free.y, used.x - free.x, free.h});
      if (used.x + used.w < free.x + free.w)
        next.push_back({used.x + used.w, free.y, free.x + free.w - (used.x + used.w), free.h});
      if (used.y > free.y) next.push_back({free.x, free.y, free.w, used.y - free.y});
      if (used.y + used.h < free.y + free.h)
        next.push_back({free.x, used.y + used.h, free.w, free.y + free.h - (used.y + used.h)});
    }
    freeRects = std::move(next);
  }

  void free_rects_prune(std::vector<Rect>& freeRects)
  {
    for (int i = 0; i < (int)freeRects.size(); ++i)
    {
      if (freeRects[i].w <= 0 || freeRects[i].h <= 0)
      {
        freeRects.erase(freeRects.begin() + i--);
        continue;
      }
      for (int j = i + 1; j < (int)freeRects.size();)
      {
        if (is_rect_contained(freeRects[i], freeRects[j]))
          freeRects.erase(freeRects.begin() + j);
        else if (is_rect_contained(freeRects[j], freeRects[i]))
        {
          freeRects.erase(freeRects.begin() + i--);
          break;
        }
        else
          ++j;
      }
    }
  }

  bool rect_insert(std::vector<Rect>& freeRects, glm::ivec2 size, Rect& result)
  {
    int bestShort = std::numeric_limits<int>::max();
    int bestLong = std::numeric_limits<int>::max();
    bool isFound{};
    for (auto& free : freeRects)
    {
      if (size.x > free.w || size.y > free.h) continue;
      int shortSide = std::min(free.w - size.x, free.h - size.y);
      int longSide = std::max(free.w - size.x, free.h - size.y);
      if (shortSide > bestShort || (shortSide == bestShort && longSide >= bestLong)) continue;
      bestShort = shortSide;
      bestLong = longSide;
      result = {free.x, free.y, size.x, size.y};
      isFound = true;
    }
    if (!isFound) return false;
    free_rects_split(freeRects, result);
    free_rects_prune(freeRects);
    return true;
  }

  bool rects_pack(const std::vector<glm::ivec2>& sizes, glm::ivec2& packedSize, std::vector<glm::ivec2>& positions)
  {
    if (sizes.empty()) return false;

    glm::ivec2 maxSize{};
    glm::ivec2 sumSize{};
    std::int64_t totalArea{};
    for (auto& size : sizes)
    {
      maxSize = glm::max(maxSize, size);
      sumSize += size;
      totalArea += (std::int64_t)size.x * size.y;
    }
    if (maxSize.x <= 0 || maxSize.y <= 0) return false;

    int bestSquareDelta = std::numeric_limits<int>::max();
    int bestArea = std::numeric_limits<int>::max();
    int endWidth = std::max(maxSize.x, sumSize.x);
    int step = std::max(1, (endWidth - maxSize.x) / WIDTH_CANDIDATES_MAX);

    for (int width = maxSize.x; width <= endWidth; width += step)
    {
      std::vector<glm::ivec2> candidate(sizes.size());
      glm::ivec2 used{};
      bool isValid{};
      for (int height = std::max(maxSize.y, (int)std::ceil((double)totalArea / width)); height <= sumSize.y && !isValid;
           ++height)
      {
        std::vector<Rect> freeRects{{0, 0, width, height}};
        used = {};
        isValid = true;
        for (int i = 0; i < (int)sizes.size() && isValid; ++i)
        {
          Rect rect{};
          isValid = rect_insert(freeRects, sizes[i], rect);
          candidate[i] = {rect.x, rect.y};
          used = glm::max(used, glm::ivec2(rect.x + rect.w, rect.y + rect.h));
        }
      }
      if (!isValid) continue;

      int area = used.x * used.y;
      int squareDelta = std::abs(used.x - used.y);
      if (squareDelta > bestSquareDelta || (squareDelta == bestSquareDelta && area >= bestArea)) continue;
      bestSquareDelta = squareDelta;
      bestArea = area;
      packedSize = used;
      positions = std::move(candidate);
      if (bestArea == totalArea && bestSquareDelta == 0) break;
    }

    return bestArea != std::numeric_limits<int>::max();
  }
}
