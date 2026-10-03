#include "internal.hpp"

using namespace anm2ed::util;
using namespace tinyxml2;

namespace anm2ed
{
  constexpr const char* CREATED_ON_FORMAT = "%m/%d/%Y %I:%M:%S %p";

  bool is_source_document_tag(std::string_view tag) { return tag == SOURCE_DOCUMENT_TAG; }

  const XMLElement* source_document_get(const XMLElement* rootElement)
  {
    if (!rootElement) return nullptr;
    if (is_source_document_tag(rootElement->Name() ? rootElement->Name() : "")) return rootElement;

    for (auto child = rootElement->FirstChildElement(); child; child = child->NextSiblingElement())
      if (is_source_document_tag(child->Name() ? child->Name() : "")) return child;

    return nullptr;
  }

  bool anm2_document_load(Anm2& anm2, XMLDocument& document, std::string* errorString)
  {
    auto rootElement = document.RootElement();
    if (!rootElement)
    {
      if (errorString) *errorString = "No root element.";
      anm2.isValid = false;
      return false;
    }

    auto sourceElement = source_document_get(rootElement);
    anm2.root = element_read(sourceElement ? sourceElement : rootElement);
    if (sourceElement)
    {
      anm2.root.type = ElementType::ANIMATED_ACTOR;
      anm2.root.tag = "AnimatedActor";
    }
    group_metadata_embed(anm2.root);
    group_frames_restore(anm2.root);
    groups_flatten(anm2.root);
    content_ids_repair(anm2.root);
    shader_frame_ids_repair(anm2.root);
    region_frame_ids_repair(anm2.root);
    anm2.region_frames_sync(true);
    anm2.isValid = true;
    return true;
  }

  Anm2::Anm2()
  {
    root = element_make(ElementType::ANIMATED_ACTOR);

    auto info = element_make(ElementType::INFO);
    info.createdOn = time::get(CREATED_ON_FORMAT);

    auto content = element_make(ElementType::CONTENT);
    for (auto type : {ElementType::SPRITESHEETS, ElementType::SHADERS, ElementType::LAYERS, ElementType::NULLS,
                      ElementType::EVENTS})
      content.children.push_back(element_make(type));

    root.children.push_back(std::move(info));
    root.children.push_back(std::move(content));
    root.children.push_back(element_make(ElementType::ANIMATIONS));
  }

  bool Anm2::load(const std::filesystem::path& path, std::string* errorString)
  {
    XMLDocument document{};
    File file(path, "rb");
    if (!file)
    {
      if (errorString) *errorString = "File not found.";
      isValid = false;
      return false;
    }

    if (document.LoadFile(file.get()) != XML_SUCCESS)
    {
      if (errorString) *errorString = document.ErrorStr();
      isValid = false;
      return false;
    }

    return anm2_document_load(*this, document, errorString);
  }

  bool Anm2::load_string(std::string_view string, std::string* errorString)
  {
    XMLDocument document{};
    if (document.Parse(string.data(), string.size()) != XML_SUCCESS)
    {
      if (errorString) *errorString = document.ErrorStr();
      isValid = false;
      return false;
    }

    return anm2_document_load(*this, document, errorString);
  }

  bool Anm2::save(const std::filesystem::path& path, std::string* errorString, Options options) const
  {
    XMLDocument document{};
    document.InsertFirstChild(to_element(document, options));

    File file(path, "wb");
    if (!file)
    {
      if (errorString) *errorString = "File permissions.";
      return false;
    }

    if (document.SaveFile(file.get()) != XML_SUCCESS)
    {
      if (errorString) *errorString = document.ErrorStr();
      return false;
    }

    return true;
  }

  std::string Anm2::to_string(Options options) const
  {
    XMLDocument document{};
    document.InsertEndChild(to_element(document, options));
    return xml::document_to_string(document);
  }

  XMLElement* Anm2::to_element(XMLDocument& document, Options options) const
  {
    auto serialize = [&](Flags flags)
    {
      auto normalized = normalized_for_serialize(flags);
      normalized.region_frames_sync(true);
      return element_to_xml(document, normalized.root, ElementType::UNKNOWN, flags);
    };

    auto editor = serialize(SERIALIZE_ANM2ED_DEFAULT);
    if (options.isExtendedFormat) return editor;

    auto out = serialize(SERIALIZE_ISAAC_DEFAULT);
    editor->SetName(SOURCE_DOCUMENT_TAG.data());
    out->InsertEndChild(editor);
    return out;
  }

}
