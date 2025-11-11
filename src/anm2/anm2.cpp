#include "anm2.h"

#include "filesystem_.h"
#include "time_.h"
#include "vector_.h"
#include "xml_.h"

using namespace tinyxml2;
using namespace anm2ed::types;
using namespace anm2ed::util;
using namespace glm;

namespace anm2ed::anm2
{
  Anm2::Anm2() { info.createdOn = time::get("%m/%d/%Y %I:%M:%S %p"); }

  Anm2::Anm2(const std::string& path, std::string* errorString)
  {
    XMLDocument document;

    if (document.LoadFile(path.c_str()) != XML_SUCCESS)
    {
      if (errorString) *errorString = document.ErrorStr();
      return;
    }

    filesystem::WorkingDirectory workingDirectory(path, true);

    const XMLElement* element = document.RootElement();

    if (auto infoElement = element->FirstChildElement("Info")) info = Info((XMLElement*)infoElement);
    if (auto contentElement = element->FirstChildElement("Content")) content = Content((XMLElement*)contentElement);
    if (auto animationsElement = element->FirstChildElement("Animations"))
      animations = Animations((XMLElement*)animationsElement);
  }

  XMLElement* Anm2::to_element(XMLDocument& document)
  {
    auto element = document.NewElement("AnimatedActor");
    document.InsertFirstChild(element);

    info.serialize(document, element);
    content.serialize(document, element);
    animations.serialize(document, element);

    return element;
  }

  bool Anm2::serialize(const std::string& path, std::string* errorString)
  {
    XMLDocument document;
    document.InsertFirstChild(to_element(document));

    if (document.SaveFile(path.c_str()) != XML_SUCCESS)
    {
      if (errorString) *errorString = document.ErrorStr();
      return false;
    }
    return true;
  }

  std::string Anm2::to_string()
  {
    XMLDocument document{};
    document.InsertEndChild(to_element(document));
    return xml::document_to_string(document);
  }

  uint64_t Anm2::hash() { return std::hash<std::string>{}(to_string()); }

  Frame* Anm2::frame_get(int animationIndex, Type itemType, int frameIndex, int itemID)
  {
    if (auto item = item_get(animationIndex, itemType, itemID); item)
      if (vector::in_bounds(item->frames, frameIndex)) return &item->frames[frameIndex];
    return nullptr;
  }
}
