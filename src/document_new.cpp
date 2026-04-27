#include "document_new.hpp"

namespace anm2ed
{
  DocumentNew::DocumentNew(const std::filesystem::path& path, std::string* errorString) : path(path)
  {
    anm2 = anm2_new::Anm2(path, errorString);
    isOpen = true;
    isValid = anm2.isValid;
  }

  DocumentNew::DocumentNew(DocumentNew&& other) noexcept
      : path(std::move(other.path)), snapshots(std::move(other.snapshots)), current(snapshots.current), anm2(current.anm2),
        message(current.message), selectedElements(std::move(other.selectedElements)), isOpen(other.isOpen),
        isValid(other.isValid)
  {
  }

  DocumentNew& DocumentNew::operator=(DocumentNew&& other) noexcept
  {
    if (this != &other)
      this->~DocumentNew();
    new (this) DocumentNew(std::move(other));
    return *this;
  }

  DocumentNew::ElementState* DocumentNew::element_state_get(anm2_new::Anm2::Element::Type type)
  {
    if (type == anm2_new::Anm2::Element::UNKNOWN) return nullptr;
    return &selectedElements[type];
  }

  const DocumentNew::ElementState* DocumentNew::element_state_get(anm2_new::Anm2::Element::Type type) const
  {
    if (type == anm2_new::Anm2::Element::UNKNOWN) return nullptr;
    if (auto it = selectedElements.find(type); it != selectedElements.end()) return &it->second;
    return nullptr;
  }

  bool DocumentNew::save(const std::filesystem::path& path, std::string* errorString,
                         anm2_new::Anm2::Compatibility compatibility)
  {
    if (!path.empty()) this->path = path;
    if (this->path.empty()) return false;

    auto isSaved = anm2.serialize(this->path, errorString, compatibility);
    if (isSaved)
    {
      isOpen = true;
      isValid = anm2.isValid;
    }

    return isSaved;
  }

  std::filesystem::path DocumentNew::filename_get() const { return path.filename(); }

  void DocumentNew::snapshot(const std::string& message)
  {
    current.message = message;
    snapshots.push(current);
  }

  void DocumentNew::undo()
  {
    snapshots.undo();
    isValid = anm2.isValid;
  }

  void DocumentNew::redo()
  {
    snapshots.redo();
    isValid = anm2.isValid;
  }

  bool DocumentNew::is_able_to_undo() { return !snapshots.undoStack.is_empty(); }

  bool DocumentNew::is_able_to_redo() { return !snapshots.redoStack.is_empty(); }
}
