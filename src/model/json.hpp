#pragma once

#include <string>
#include <string_view>

#include "model.hpp"

// The editor's own JSON document: content items in arrays (an item's index is its id), animations keyed by their
// unique name, and every value left out when it is the default. Unknown XML (extras) is not kept.
namespace anm2ed::model
{
  bool is_json(std::string_view);
  std::string model_to_json(const Model&);
  bool model_from_json(Model&, std::string_view, std::string* = nullptr);
}
