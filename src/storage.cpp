#include "storage.h"

namespace anm2ed
{
  void Storage::labels_set(std::vector<std::string> labels)
  {
    labelsString = labels;
    this->labels.clear();
    for (auto& label : labelsString)
      this->labels.emplace_back(label.c_str());
  }
}