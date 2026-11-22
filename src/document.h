#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <map>
#include <set>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "asset_store.h"
#include "edit/edit.h"
#include "model/draw.h"
#include "selection.h"
#include "snapshots.h"
#include "localization.h"

#include <glm/glm.hpp>

#include "shader.h"

  class Manager;
  struct Command;

  // Ids and labels for a combo box of content items.
  struct Choices
  {
    std::vector<int> ids;
    std::vector<std::string> labels;
  };

  // A linked .anm2, loaded read-only: its document, its spritesheets' images and its sounds.
  struct LibraryData
  {
    Model model = {};
    std::map<int, Image> images;
    std::map<int, AudioData> sounds;
    std::filesystem::file_time_type writeTime;
  };

  // A step into an animation layer's child: the parent's tab, animation, time and layer track it was taken from,
  // and the animation it led to (in this tab, or a library's).
  struct ChildStep
  {
    std::uint64_t tabId = 0;
    int animationIndex = -1;
    float frameTime = 0;
    Reference layer = {};
    std::string animationName;
    std::string layerName;
    int childAnimationIndex = -1;
  };

  struct DocumentData
  {
    std::filesystem::path path;
    uint64_t tabId = 0;
    Snapshots snapshots = {};
    AssetStore assets = {};
    std::map<int, Image> textureDrafts;
    int changeAllFramePropertiesRegionId = -1;
    int changeAllFramePropertiesShaderId = -1;

    float previewZoom = 200;
    glm::vec2 previewPan{};
    glm::vec2 editorPan{};
    float editorZoom = 200;
    int overlayIndex = -1;
    uint64_t overlayDocumentId = 0;

    uint64_t hash = 0;
    uint64_t saveHash = 0;
    uint64_t autosaveHash = 0;
    double lastAutosaveTime = 0;
    bool isValid = true;
    bool isOpen = true;
    bool isForceDirty = false;
    bool isVectorKept = false;
    std::vector<ChildStep>
        childSteps{}; // the steps that led to the animation being edited (the breadcrumb) // asked this session to keep SVG spritesheets in a Rebirth document
    std::unordered_map<int, uint64_t> spritesheetHashes;
    std::unordered_map<int, uint64_t> spritesheetSaveHashes;
    std::unordered_map<int, uint64_t> spritesheetAutosaveHashes;
    std::unordered_map<int, std::filesystem::path> texturePaths;
    std::unordered_map<int, std::filesystem::file_time_type> textureWriteTimes;
    std::unordered_map<int, std::filesystem::path> soundPaths;
    std::map<int, std::filesystem::path> shaderVertexPaths;
    std::map<int, std::filesystem::path> shaderFragmentPaths;
    std::map<int, ShaderProgram> shaders;
    // Libraries by absolute path, and what each (document, library id) points at (nullptr is this document).
    std::map<std::filesystem::path, LibraryData> libraries;
    std::map<std::pair<const Model*, int>, const LibraryData*> libraryLinks;
    std::uint64_t librariesRevision = 0; // counts library loads, for anything drawn from them
    // Vector spritesheets drawn again for zooms past 100%, by (image uid, power-of-two level).
    mutable std::map<std::pair<std::uint64_t, int>, Image> vectorImages;
    bool isAnimationPreviewSet = false;
    bool isSpritesheetEditorSet = false;
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
    std::string& message = current.message;
    EditTarget editTarget = EditTarget::NONE;
    UidIndex index = {};

    Document(const std::filesystem::path&, bool = false, std::string* = nullptr);
    Document(const Document&) = delete;
    Document& operator=(const Document&) = delete;
    Document(Document&&) noexcept;
    Document& operator=(Document&&) noexcept;
    bool save(const std::filesystem::path& = {}, std::string* = nullptr, Options = {});
    bool file_write(const std::filesystem::path&, std::string* = nullptr, Options = {});
    void assets_sync();
    void texture_change(int);
    bool texture_reload(int);
    bool sound_reload(int);
    bool shader_reload(int, std::string* = nullptr);
    const Image* texture_get(int) const;
    Image* texture_edit(int);
    void texture_set(int, Image);
    const AudioData* sound_get(int) const;
    std::vector<const AudioData*> sounds_get() const;
    std::vector<const AudioData*> trigger_sounds_get(const Animation&, int, int = -1) const;
    void sound_set(int, AudioData);
    ShaderProgram* shader_get(int);
    std::filesystem::path library_path_get(int) const;
    const LibraryData* library_get(const Model&, int) const;
    LibraryGet library_resolver_get() const;
    SpritesheetPathGet spritesheet_path_resolver_get() const;
    std::vector<std::string> animation_names_get(int) const;
    const Image* draw_image_get(const Draw&, float = 1.0f) const;
    const Image* vector_image_get(const Image*, float) const;
    void libraries_sync();
    bool libraries_reload();
    void library_reload(int);
    bool regions_trim(int, const std::set<int>&);
    bool spritesheet_pack(int, int);
    bool spritesheets_merge(const std::set<int>&, bool, bool, bool, Origin);
    void hash_set();
    void clean();
    void change();
    void edit_begin(StringType);
    Uids edit_run(StringType, const std::function<Uids(Model&)>&);

    // Snapshots for undo, runs the operation on the model, then commits; void operations select nothing.
    template <class Operation> Uids edit_apply(StringType label, Operation&& operation)
    {
      return edit_run(label,
                      [&](Model& model) -> Uids
                      {
                        if constexpr (std::is_void_v<std::invoke_result_t<Operation&, Model&>>)
                        {
                          operation(model);
                          return {};
                        }
                        else
                          return operation(model);
                      });
    }
    std::vector<Reference> references_get(const Uids&) const;
    bool is_dirty() const;
    bool is_layer_animation_missing(const Layer&) const;
    bool is_autosave_dirty();
    std::filesystem::path directory_get() const;
    std::filesystem::path filename_get() const;
    bool is_valid() const;
    void command_run(Manager&, Command&);
    void spritesheet_hash_update(int);
    void spritesheet_hash_set_saved(int);
    bool spritesheet_is_dirty(int);
    bool spritesheet_any_dirty();
    void spritesheet_hashes_sync();
    std::set<int> spritesheets_changed_get();
    void spritesheets_reload(const std::set<int>&);
    void spritesheets_vector_convert(const std::set<int>&);
    std::filesystem::path spritesheet_autosave_path_get(const Spritesheet&);
    std::set<int> spritesheets_autosave_dirty_get();
    void spritesheets_autosave_restore();
    void spritesheets_autosave_clear();
    Reference reference_get() const;
    void reference_set(Reference);
    std::set<Reference> selected_get(SelectionKind) const;
    void selected_set(SelectionKind, const std::set<Reference>&);
    std::set<int> selected_ids_get(SelectionKind) const;
    void selected_ids_set(SelectionKind, const std::set<int>&);
    int focused_id_get(SelectionKind) const;
    void focused_id_set(SelectionKind, int);
    void selected_clear(SelectionKind);
    Choices choices_get(SelectionKind, int = -1) const;
    void frames_select(const Uids&);
    void frame_select(Reference);
    void frame_toggle(Reference);
    bool frames_range_select(Reference, Reference, bool);
    void frame_focus_select();
    void focus_clear();
    void track_select(Reference);
    std::set<int> animations_selected_get() const;
    void animations_selected_set(const std::set<int>&);
    std::set<Reference> item_frame_references_get(Reference) const;
    std::set<Reference> selected_item_frame_references_get() const;
    std::set<Reference> frame_references_get(FrameReferenceFallback = FrameReferenceFallback::CURRENT) const;
    void frame_references_set(std::set<Reference>);
    void frame_references_clear();
    std::vector<Reference> layer_references_get();

    void spritesheets_add(const std::vector<std::filesystem::path>&);
    void sounds_add(const std::vector<std::filesystem::path>&);
    void libraries_add(const std::vector<std::filesystem::path>&);

    bool autosave(std::string* = nullptr, Options = {});
    std::filesystem::path autosave_path_get();
    std::filesystem::path path_from_autosave_get(const std::filesystem::path&);

    void undo();
    void redo();
    bool is_able_to_undo();
    bool is_able_to_redo();
  };
