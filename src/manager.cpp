#include "manager.h"

#include "toast.h"

#include "util.h"

using namespace anm2ed::toast;
using namespace anm2ed::types;
using namespace anm2ed::util;

namespace anm2ed::manager
{
  Document* Manager::get(int index)
  {
    return vector::find(documents, index > -1 ? index : selected);
  }

  void Manager::open(const std::string& path, bool isNew)
  {
    std::string errorString{};
    Document document = Document(path, isNew, &errorString);
    if (document.is_valid())
    {
      documents.emplace_back(std::move(document));
      selected = documents.size() - 1;
      pendingSelected = selected;
      toasts.info(std::format("Initialized document: {}", path));
    }
    else
      toasts.error(std::format("Failed to initialize document: {} ({})", path, errorString));
  }

  void Manager::new_(const std::string& path)
  {
    open(path, true);
  }

  void Manager::save(int index, const std::string& path)
  {
    if (auto document = get(index); document)
    {
      std::string errorString{};
      document->path = !path.empty() ? path : document->path.string();
      document->save(document->path, &errorString);
    }
  }

  void Manager::save(const std::string& path)
  {
    save(selected, path);
  }

  void Manager::close(int index)
  {
    documents.erase(documents.begin() + index);
  }

  void Manager::layer_properties_open(int id)
  {
    if (auto document = get(); document)
    {
      if (id == -1)
        editLayer = anm2::Layer();
      else
        editLayer = document->anm2.content.layers.at(id);

      document->referenceLayer = id;

      layerPropertiesPopup.open();
    }
  }

  void Manager::layer_properties_trigger()
  {
    layerPropertiesPopup.trigger();
  }

  void Manager::layer_properties_end()
  {
    layerPropertiesPopup.end();
  }

  void Manager::layer_properties_close()
  {
    editLayer = anm2::Layer();
    layerPropertiesPopup.close();
  }

  void Manager::null_properties_open(int id)
  {
    if (auto document = get(); document)
    {
      if (id == -1)
        editNull = anm2::Null();
      else
        editNull = document->anm2.content.nulls.at(id);

      document->referenceNull = id;

      nullPropertiesPopup.open();
    }
  }

  void Manager::null_properties_trigger()
  {
    nullPropertiesPopup.trigger();
  }

  void Manager::null_properties_end()
  {
    nullPropertiesPopup.end();
  }

  void Manager::null_properties_close()
  {
    editNull = anm2::Null();
    nullPropertiesPopup.close();
  }

}
