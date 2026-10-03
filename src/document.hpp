#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <map>
#include <set>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "asset_store.hpp"
#include "edit/edit.hpp"
#include "selection.hpp"
#include "snapshots.hpp"
#include "strings.hpp"

#include <glm/glm.hpp>

#include "origin.hpp"
#include "shader.hpp"
#include "types.hpp"

namespace anm2ed
{
  class Manager;
  struct Command;

  struct DocumentData
  {
    std::filesystem::path path{};
    uint64_t tabId{};
    Snapshots snapshots{};
    std::map<int, Storage> regionBySpritesheet{};
    Storage animation{};
    resource::AssetStore assets{};
    std::map<int, resource::Image> textureDrafts{};
    int changeAllFramePropertiesRegionId{-1};
    int changeAllFramePropertiesShaderId{-1};

    float previewZoom{200};
    glm::vec2 previewPan{};
    glm::vec2 editorPan{};
    float editorZoom{200};
    int overlayIndex{-1};
    uint64_t overlayDocumentId{};

    uint64_t hash{};
    uint64_t saveHash{};
    uint64_t autosaveHash{};
    double lastAutosaveTime{};
    bool isValid{true};
    bool isOpen{true};
    bool isForceDirty{false};
    std::unordered_map<int, uint64_t> spritesheetHashes{};
    std::unordered_map<int, uint64_t> spritesheetSaveHashes{};
    std::unordered_map<int, std::filesystem::path> texturePaths{};
    std::unordered_map<int, std::filesystem::path> soundPaths{};
    std::map<int, std::filesystem::path> shaderVertexPaths{};
    std::map<int, std::filesystem::path> shaderFragmentPaths{};
    std::map<int, resource::Shader> shaders{};
    bool isAnimationPreviewSet{false};
    bool isSpritesheetEditorSet{false};
  };

  class Document : public DocumentData
  {
  public:
    enum class FrameReferenceFallback
    {
      NONE,
      CURRENT,
    };

    enum class EditTarget
    {
      NONE,
      FRAME,
      REGION,
      SPRITESHEET
    };

    Snapshot& current = snapshots.current;
#define X(type, name) type& name = current.name;
    SNAPSHOT_STEP_STATE_FIELDS
#undef X
    Anm2& anm2 = current.anm2;
    std::string& message = current.message;
    EditTarget editTarget{EditTarget::NONE};
    UidIndex index{};

    Document(const std::filesystem::path&, bool = false, std::string* = nullptr);
    Document(const Document&) = delete;
    Document& operator=(const Document&) = delete;
    Document(Document&&) noexcept;
    Document& operator=(Document&&) noexcept;
    bool save(const std::filesystem::path& = {}, std::string* = nullptr, Options = {});
    void assets_sync();
    void texture_change(int);
    bool texture_reload(int);
    bool sound_reload(int);
    bool shader_reload(int, std::string* = nullptr);
    const resource::Image* texture_get(int) const;
    resource::Image* texture_edit(int);
    void texture_set(int, resource::Image);
    const resource::AudioData* sound_get(int) const;
    void sound_set(int, resource::AudioData);
    resource::Shader* shader_get(int);
    bool regions_trim(int, const std::set<int>&);
    bool spritesheet_pack(int, int);
    bool spritesheets_merge(const std::set<int>&, bool, bool, bool, origin::Type);
    void hash_set();
    void clean();
    void change();
    void edit_begin(StringType);
    edit::Uids edit_run(StringType, const std::function<edit::Uids(Anm2&)>&);

    // Snapshots for undo, runs the operation on the model, then commits; void operations select nothing.
    template <class Operation> edit::Uids edit_apply(StringType label, Operation&& operation)
    {
      return edit_run(label,
                      [&](Anm2& anm2) -> edit::Uids
                      {
                        if constexpr (std::is_void_v<std::invoke_result_t<Operation&, Anm2&>>)
                        {
                          operation(anm2);
                          return {};
                        }
                        else
                          return operation(anm2);
                      });
    }
    std::vector<Reference> references_get(const edit::Uids&) const;
    bool is_dirty() const;
    bool is_autosave_dirty() const;
    std::filesystem::path directory_get() const;
    std::filesystem::path filename_get() const;
    bool is_valid() const;
    void command_run(Manager&, Command&);
    void spritesheet_hash_update(int);
    void spritesheet_hash_set_saved(int);
    bool spritesheet_is_dirty(int);
    bool spritesheet_any_dirty();
    void spritesheet_hashes_reset();
    void spritesheet_hashes_sync();
    Reference reference_get() const;
    void reference_set(Reference);
    std::set<Reference> selected_get(SelectionKind) const;
    void selected_set(SelectionKind, const std::set<Reference>&);
    std::set<int> animations_selected_get() const;
    void animations_selected_set(const std::set<int>&);
    std::set<Reference> item_frame_references_get(Reference) const;
    std::set<Reference> selected_item_frame_references_get() const;
    std::set<Reference> frame_references_get(FrameReferenceFallback = FrameReferenceFallback::CURRENT) const;
    void frame_references_set(std::set<Reference>);
    void frame_references_clear();
    std::vector<Reference> layer_references_get();

    Storage* layer_regions_get(int);
    Element* frame_get();
    Element* item_get();
    Element* spritesheet_get();

    void spritesheets_add(const std::vector<std::filesystem::path>&);
    void sounds_add(const std::vector<std::filesystem::path>&);

    bool autosave(std::string* = nullptr, Options = {});
    std::filesystem::path autosave_path_get();
    std::filesystem::path path_from_autosave_get(const std::filesystem::path&);

    void undo();
    void redo();
    bool is_able_to_undo();
    bool is_able_to_redo();
  };
}
