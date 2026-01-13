#include "storage.h"

#include <utility>

namespace anm2ed
{
  void Storage::clear() { *this = Storage(); }

  void Storage::labels_set(std::vector<std::string> labels) { labels_set(std::move(labels), {}); }

  void Storage::labels_set(std::vector<std::string> labels, std::vector<int> ids)
  {
    labelsString = std::move(labels);
    this->ids = std::move(ids);
    this->labels.clear();
    for (auto& label : labelsString)
      this->labels.emplace_back(label.c_str());
  }
}
