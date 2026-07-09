#pragma once

#include <cstdint>
#include <filesystem>
#include <map>
#include <set>
#include <unordered_map>
#include <vector>

#include "snapshots.hpp"

#include <glm/glm.hpp>

#include "origin.hpp"
#include "shader.hpp"
#include "types.hpp"

namespace anm2ed
{
  class Manager;
  struct Command;

  class Document
  {
  public:
    enum ChangeType
    {
      INFO,
      LAYERS,
      NULLS,
      SPRITESHEETS,
      TEXTURES,
      EVENTS,
      ANIMATIONS,
      ITEMS,
      FRAMES,
      SOUNDS,
      ALL,
      COUNT
    };

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
      OVERLAY,
      SPRITESHEET
    };

    enum class RegionFrameMapping
    {
      PRESERVE,
      SET
    };

    enum FileMergePreset
    {
      FILE_MERGE_PRESET_MERGE_BY_NAME,
      FILE_MERGE_PRESET_APPEND_AS_NEW,
      FILE_MERGE_PRESET_REPLACE_MATCHING,
      FILE_MERGE_PRESET_COUNT
    };

    std::filesystem::path path{};
    uint64_t tabId{};

    Snapshots snapshots{};
    Snapshot& current = snapshots.current;

    Playback& playback = current.playback;
    Storage& animation = current.animation;
    Storage& event = current.event;
    Storage& frames = current.frames;
    Storage& items = current.items;
    Storage& layer = current.layer;
    Storage& merge = current.merge;
    Storage& null = current.null;
    Storage& overlay = current.overlay;
    Storage& region = current.region;
    Storage& sound = current.sound;
    Storage& spritesheet = current.spritesheet;
    std::map<int, resource::Texture>& textures = current.textures;
    std::map<int, resource::Texture>& overlayTextures = current.overlayTextures;
    std::map<int, resource::Audio>& sounds = current.sounds;
    Anm2& anm2 = current.anm2;
    Reference& reference = current.reference;
    std::set<Reference>& groupReferences = current.groupReferences;
    float& frameTime = current.frameTime;
    std::string& message = current.message;
    std::map<int, Storage> regionBySpritesheet{};
    int changeAllFramePropertiesRegionId{-1};

    float previewZoom{200};
    glm::vec2 previewPan{};
    glm::vec2 editorPan{};
    float editorZoom{200};
    int overlayIndex{-1};

    uint64_t hash{};
    uint64_t saveHash{};
    uint64_t autosaveHash{};
    double lastAutosaveTime{};
    bool isValid{true};
    bool isOpen{true};
    bool isForceDirty{false};
    std::unordered_map<int, uint64_t> spritesheetHashes{};
    std::unordered_map<int, uint64_t> spritesheetSaveHashes{};
    std::unordered_map<int, uint64_t> overlayHashes{};
    std::unordered_map<int, uint64_t> overlaySaveHashes{};
    std::unordered_map<int, std::filesystem::path> texturePaths{};
    std::unordered_map<int, std::filesystem::path> overlayTexturePaths{};
    std::unordered_map<int, std::filesystem::path> soundPaths{};
    std::unordered_map<int, std::filesystem::path> shaderVertexPaths{};
    std::unordered_map<int, std::filesystem::path> shaderFragmentPaths{};
    std::map<int, resource::Shader> shaders{};
    bool isAnimationPreviewSet{false};
    bool isSpritesheetEditorSet{false};
    EditTarget editTarget{EditTarget::NONE};

    Document(const std::filesystem::path&, bool = false, std::string* = nullptr);
    Document(const Document&) = delete;
    Document& operator=(const Document&) = delete;
    Document(Document&&) noexcept;
    Document& operator=(Document&&) noexcept;
    bool save(const std::filesystem::path& = {}, std::string* = nullptr, Options = {});
    void anm2_change(ChangeType);
    void assets_sync(ChangeType = ALL);
    void texture_change(int);
    void overlay_texture_change(int);
    bool texture_reload(int);
    bool overlay_texture_reload(int);
    bool sound_reload(int);
    bool shader_reload(int, std::string* = nullptr);
    resource::Texture* texture_get(int);
    const resource::Texture* texture_get(int) const;
    resource::Texture* overlay_texture_get(int);
    const resource::Texture* overlay_texture_get(int) const;
    resource::Audio* sound_get(int);
    const resource::Audio* sound_get(int) const;
    resource::Shader* shader_get(int);
    const resource::Shader* shader_get(int) const;
    bool regions_trim(int, const std::set<int>&);
    bool regions_generate_from_animations(const std::set<int>&, const std::string&, RegionFrameMapping);
    bool regions_generate_from_frames(const std::set<Reference>&, const std::string&, RegionFrameMapping);
    bool spritesheet_pack(int, int);
    bool spritesheets_merge(const std::set<int>&, bool, bool, bool, origin::Type);
    void scan_and_set_regions();
    bool file_merge(const std::filesystem::path&, FileMergePreset = FILE_MERGE_PRESET_MERGE_BY_NAME);
    void hash_set();
    void clean();
    void change(ChangeType);
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
    void overlay_hash_update(int);
    void overlay_hash_set_saved(int);
    bool overlay_is_dirty(int);
    bool overlay_any_dirty();
    void overlay_hashes_reset();
    void overlay_hashes_sync();
    bool is_frame_reference_valid(Reference) const;
    std::set<Reference> item_frame_references_get(Reference) const;
    std::set<Reference> selected_item_frame_references_get() const;
    std::set<Reference> frame_references_get(FrameReferenceFallback = FrameReferenceFallback::CURRENT) const;
    void frame_references_set(std::set<Reference>);
    void frame_references_clear();
    std::vector<Reference> layer_references_get();

    Element* frame_get();
    Element* item_get();
    Element* spritesheet_get();
    Element* overlay_get(int = -1);
    const Element* overlay_get(int = -1) const;
    int overlay_spritesheet_id_get(int) const;
    Element* animation_get();

    void spritesheet_add(const std::filesystem::path&);
    void spritesheets_add(const std::vector<std::filesystem::path>&);
    void overlay_add(int, const std::filesystem::path&);
    void overlays_add(int, const std::vector<std::filesystem::path>&);
    void sound_add(const std::filesystem::path&);
    void sounds_add(const std::vector<std::filesystem::path>&);

    bool autosave(std::string* = nullptr, Options = {});
    std::filesystem::path autosave_path_get();
    std::filesystem::path path_from_autosave_get(const std::filesystem::path&);

    void anm2_snapshot(const std::string& message);
    void frames_snapshot(const std::string& message, const std::set<Reference>&);
    void regions_snapshot(const std::string& message, int, const std::set<int>&);
    void textures_snapshot(const std::string& message);
    void anm2_textures_snapshot(const std::string& message);
    void undo();
    void redo();
    bool is_able_to_undo();
    bool is_able_to_redo();
  };

#define DOCUMENT_EDIT(document, message, changeType, body)                                                             \
  {                                                                                                                    \
    document.anm2_snapshot(message);                                                                                   \
    body;                                                                                                              \
    document.change(changeType);                                                                                       \
  }

#define DOCUMENT_EDIT_PTR(document, message, changeType, body)                                                         \
  {                                                                                                                    \
    document->anm2_snapshot(message);                                                                                  \
    body;                                                                                                              \
    document->change(changeType);                                                                                      \
  }

}
