#include "taskbar.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <ranges>

#include <imgui/imgui.h>

#include "math_.h"
#include "render.h"
#include "shader.h"
#include "toast.h"
#include "types.h"

#include "icon.h"

using namespace anm2ed::resource;
using namespace anm2ed::types;
using namespace anm2ed::canvas;
using namespace anm2ed::util;
using namespace glm;

namespace anm2ed::imgui
{
#ifdef __unix__

  namespace
  {
    constexpr std::array<int, 7> ICON_SIZES{16, 24, 32, 48, 64, 128, 256};

    bool ensure_parent_directory_exists(const std::filesystem::path& path)
    {
      std::error_code ec;
      std::filesystem::create_directories(path.parent_path(), ec);
      if (ec)
      {
        toasts.warning(std::format("Could not create directory for {} ({})", path.string(), ec.message()));
        return false;
      }
      return true;
    }

    bool write_binary_blob(const std::filesystem::path& path, const std::uint8_t* data, size_t size)
    {
      if (!ensure_parent_directory_exists(path)) return false;

      std::ofstream file(path, std::ios::binary | std::ios::trunc);
      if (!file.is_open())
      {
        toasts.warning(std::format("Could not open {} for writing", path.string()));
        return false;
      }

      file.write(reinterpret_cast<const char*>(data), static_cast<std::streamsize>(size));
      return true;
    }

    bool run_command_checked(const std::string& command, const std::string& description)
    {
      auto result = std::system(command.c_str());
      if (result != 0)
      {
        toasts.warning(std::format("{} failed (exit code {})", description, result));
        return false;
      }
      return true;
    }

    bool install_icon_set(const std::string& context, const std::string& iconName, const std::filesystem::path& path)
    {
      bool success = true;
      for (auto size : ICON_SIZES)
      {
        auto command = std::format("xdg-icon-resource install --noupdate --novendor --context {} --size {} \"{}\" {}",
                                   context, size, path.string(), iconName);
        success &= run_command_checked(command, std::format("Install {} icon ({}px)", iconName, size));
      }
      return success;
    }

    bool uninstall_icon_set(const std::string& context, const std::string& iconName)
    {
      bool success = true;
      for (auto size : ICON_SIZES)
      {
        auto command =
            std::format("xdg-icon-resource uninstall --noupdate --context {} --size {} {}", context, size, iconName);
        success &= run_command_checked(command, std::format("Remove {} icon ({}px)", iconName, size));
      }
      return success;
    }

    bool remove_file_if_exists(const std::filesystem::path& path)
    {
      std::error_code ec;
      if (!std::filesystem::exists(path, ec)) return true;
      std::filesystem::remove(path, ec);
      if (ec)
      {
        toasts.warning(std::format("Could not remove {} ({})", path.string(), ec.message()));
        return false;
      }
      return true;
    }
  }

  constexpr auto MIME_TYPE = R"(<?xml version="1.0" encoding="utf-8"?>
<mime-type xmlns="http://www.freedesktop.org/standards/shared-mime-info" type="application/x-anm2+xml">
  <!--Created automatically by update-mime-database. DO NOT EDIT!-->
  <comment>Anm2 Animation</comment>
  <glob pattern="*.anm2"/>
</mime-type>
)";

  constexpr auto DESKTOP_ENTRY_FORMAT = R"([Desktop Entry]
Type=Application
Name=Anm2Ed
Icon=anm2ed
Comment=Animation editor for .anm2 files
Exec={}
Terminal=false
Categories=Graphics;Development;
MimeType=application/x-anm2+xml;
)";

#endif

  Taskbar::Taskbar() : generate(vec2()) {}

  void Taskbar::update(Manager& manager, Settings& settings, Resources& resources, Dialog& dialog, bool& isQuitting)
  {
    auto document = manager.get();
    auto reference = document ? &document->reference : nullptr;
    auto animation = document ? document->animation_get() : nullptr;
    auto item = document ? document->item_get() : nullptr;

    if (ImGui::BeginMainMenuBar())
    {
      height = ImGui::GetWindowSize().y;

      if (ImGui::BeginMenu("File"))
      {
        if (ImGui::MenuItem("New", settings.shortcutNew.c_str())) dialog.file_save(dialog::ANM2_NEW);
        if (ImGui::MenuItem("Open", settings.shortcutOpen.c_str())) dialog.file_open(dialog::ANM2_NEW);

        if (ImGui::BeginMenu("Open Recent", !manager.recentFiles.empty()))
        {
          for (auto [i, file] : std::views::enumerate(manager.recentFiles))
          {
            auto label = std::format(FILE_LABEL_FORMAT, file.filename().string(), file.string());

            ImGui::PushID(i);
            if (ImGui::MenuItem(label.c_str())) manager.open(file);
            ImGui::PopID();
          }

          if (!manager.recentFiles.empty())
            if (ImGui::MenuItem("Clear List")) manager.recent_files_clear();

          ImGui::EndMenu();
        }

        if (ImGui::MenuItem("Save", settings.shortcutSave.c_str(), false, document)) manager.save();
        if (ImGui::MenuItem("Save As", settings.shortcutSaveAs.c_str(), false, document))
          dialog.file_save(dialog::ANM2_SAVE);
        if (ImGui::MenuItem("Explore XML Location", nullptr, false, document))
          dialog.file_explorer_open(document->directory_get());

        ImGui::Separator();
        if (ImGui::MenuItem("Exit", settings.shortcutExit.c_str())) isQuitting = true;
        ImGui::EndMenu();
      }
      if (dialog.is_selected(dialog::ANM2_NEW))
      {
        manager.new_(dialog.path);
        dialog.reset();
      }

      if (dialog.is_selected(dialog::ANM2_OPEN))
      {
        manager.open(dialog.path);
        dialog.reset();
      }

      if (dialog.is_selected(dialog::ANM2_SAVE))
      {
        manager.save(dialog.path);
        dialog.reset();
      }

      if (ImGui::BeginMenu("Wizard"))
      {
        ImGui::BeginDisabled(!item || document->reference.itemType != anm2::LAYER);
        if (ImGui::MenuItem("Generate Animation From Grid")) generatePopup.open();
        if (ImGui::MenuItem("Change All Frame Properties")) changePopup.open();
        ImGui::EndDisabled();
        ImGui::Separator();
        if (ImGui::MenuItem("Render Animation", nullptr, false, animation)) renderPopup.open();
        ImGui::EndMenu();
      }

      if (ImGui::BeginMenu("Playback"))
      {
        ImGui::MenuItem("Always Loop", nullptr, &settings.playbackIsLoop);
        ImGui::SetItemTooltip("%s", "Animations will always loop during playback, even if looping isn't set.");

        ImGui::MenuItem("Clamp Playhead", nullptr, &settings.playbackIsClampPlayhead);
        ImGui::SetItemTooltip("%s", "The playhead will always clamp to the animation's length.");

        ImGui::EndMenu();
      }

      if (ImGui::BeginMenu("Window"))
      {
        for (auto [i, member] : std::views::enumerate(WINDOW_MEMBERS))
          ImGui::MenuItem(WINDOW_STRINGS[i], nullptr, &(settings.*member));

        ImGui::EndMenu();
      }

      if (ImGui::BeginMenu("Settings"))
      {
        if (ImGui::MenuItem("Configure"))
        {
          editSettings = settings;
          configurePopup.open();
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Associate .anm2 Files with Editor", nullptr, false,
                            !isAnm2Association || !isAbleToAssociateAnm2))
        {
#ifdef _WIN32

#elif __unix__
          auto cache_icons = []()
          {
            auto programIconPath = std::filesystem::path(filesystem::path_icon_get());
            auto fileIconPath = std::filesystem::path(filesystem::path_icon_file_get());
            auto iconBytes = std::size(resource::icon::PROGRAM);

            bool isSuccess = write_binary_blob(programIconPath, resource::icon::PROGRAM, iconBytes) &&
                             write_binary_blob(fileIconPath, resource::icon::PROGRAM, iconBytes);

            if (isSuccess)
            {
              isSuccess = install_icon_set("apps", "anm2ed", programIconPath) &&
                          install_icon_set("mimetypes", "application-x-anm2+xml", fileIconPath) &&
                          run_command_checked("xdg-icon-resource forceupdate --theme hicolor", "Refresh icon cache");
            }

            remove_file_if_exists(programIconPath);
            remove_file_if_exists(fileIconPath);

            if (isSuccess) toasts.info("Cached program and file icons.");
            return isSuccess;
          };

          auto register_mime = []()
          {
            auto path = std::filesystem::path(filesystem::path_mime_get());
            if (!ensure_parent_directory_exists(path)) return false;

            std::ofstream file(path, std::ofstream::out | std::ofstream::trunc);
            if (!file.is_open())
            {
              toasts.warning(std::format("Could not write .anm2 MIME type: {}", path.string()));
              return false;
            }

            file << MIME_TYPE;
            file.close();
            toasts.info(std::format("Wrote .anm2 MIME type to: {}", path.string()));

            auto mimeRoot = path.parent_path().parent_path();
            auto command = std::format("update-mime-database \"{}\"", mimeRoot.string());
            return run_command_checked(command, "Update MIME database");
          };

          auto register_desktop_entry = []()
          {
            auto path = std::filesystem::path(filesystem::path_application_get());
            if (!ensure_parent_directory_exists(path)) return false;

            std::ofstream file(path, std::ofstream::out | std::ofstream::trunc);
            if (!file.is_open())
            {
              toasts.warning(std::format("Could not write desktop entry: {}", path.string()));
              return false;
            }

            auto desktopEntry = std::format(DESKTOP_ENTRY_FORMAT, filesystem::path_executable_get());
            file << desktopEntry;
            file.close();
            toasts.info(std::format("Wrote desktop entry to: {}", path.string()));

            auto desktopDir = path.parent_path();
            auto desktopUpdate =
                std::format("update-desktop-database \"{}\"", desktopDir.empty() ? "." : desktopDir.string());
            auto desktopFileName = path.filename().string();
            auto setDefault = std::format("xdg-mime default {} application/x-anm2+xml",
                                          desktopFileName.empty() ? path.string() : desktopFileName);

            auto databaseUpdated = run_command_checked(desktopUpdate, "Update desktop database");
            auto defaultRegistered = run_command_checked(setDefault, "Set default handler for .anm2");
            return databaseUpdated && defaultRegistered;
          };

          auto iconsCached = cache_icons();
          auto mimeRegistered = register_mime();
          auto desktopRegistered = register_desktop_entry();

          isAnm2Association = iconsCached && mimeRegistered && desktopRegistered;
          if (isAnm2Association)
            toasts.info("Associated .anm2 files with the editor.");
          else
            toasts.warning("Association incomplete. Please review the warnings above.");
#endif
        }
        ImGui::SetItemTooltip(
            "Associate .anm2 files with the application (i.e., clicking on them in a file explorer will "
            "open the application).");

        if (ImGui::MenuItem("Remove .anm2 File Association", nullptr, false,
                            isAnm2Association || !isAbleToAssociateAnm2))
        {
#ifdef _WIN32

#elif __unix__
          {
            auto iconsRemoved =
                uninstall_icon_set("apps", "anm2ed") && uninstall_icon_set("mimetypes", "application-x-anm2+xml") &&
                run_command_checked("xdg-icon-resource forceupdate --theme hicolor", "Refresh icon cache");
            if (iconsRemoved)
              toasts.info("Removed cached icons.");
            else
              toasts.warning("Could not remove all cached icons.");
          }

          {
            auto path = std::filesystem::path(filesystem::path_mime_get());
            auto removed = remove_file_if_exists(path);
            if (removed) toasts.info(std::format("Removed .anm2 MIME type: {}", path.string()));

            auto mimeRoot = path.parent_path().parent_path();
            run_command_checked(std::format("update-mime-database \"{}\"", mimeRoot.string()), "Update MIME database");
          }

          {
            auto path = std::filesystem::path(filesystem::path_application_get());
            if (remove_file_if_exists(path)) toasts.info(std::format("Removed desktop entry: {}", path.string()));

            auto desktopDir = path.parent_path();
            run_command_checked(
                std::format("update-desktop-database \"{}\"", desktopDir.empty() ? "." : desktopDir.string()),
                "Update desktop database");
          }
#endif
          isAnm2Association = false;
        }
        ImGui::SetItemTooltip("Unassociate .anm2 files with the application.");

        ImGui::EndMenu();
      }

      if (ImGui::BeginMenu("Help"))
      {
        if (ImGui::MenuItem("About")) aboutPopup.open();
        ImGui::EndMenu();
      }

      ImGui::EndMainMenuBar();
    }

    generatePopup.trigger();

    if (ImGui::BeginPopupModal(generatePopup.label, &generatePopup.isOpen, ImGuiWindowFlags_NoResize))
    {
      auto& startPosition = settings.generateStartPosition;
      auto& size = settings.generateSize;
      auto& pivot = settings.generatePivot;
      auto& rows = settings.generateRows;
      auto& columns = settings.generateColumns;
      auto& count = settings.generateCount;
      auto& delay = settings.generateDelay;
      auto& zoom = settings.generateZoom;
      auto& zoomStep = settings.viewZoomStep;

      auto childSize = ImVec2(row_widget_width_get(2), size_without_footer_get().y);

      if (ImGui::BeginChild("##Options Child", childSize, ImGuiChildFlags_Borders))
      {
        ImGui::InputInt2("Start Position", value_ptr(startPosition));
        ImGui::InputInt2("Frame Size", value_ptr(size));
        ImGui::InputInt2("Pivot", value_ptr(pivot));
        ImGui::InputInt("Rows", &rows, STEP, STEP_FAST);
        ImGui::InputInt("Columns", &columns, STEP, STEP_FAST);

        input_int_range("Count", count, anm2::FRAME_NUM_MIN, rows * columns);

        ImGui::InputInt("Delay", &delay, STEP, STEP_FAST);
      }
      ImGui::EndChild();

      ImGui::SameLine();

      if (ImGui::BeginChild("##Preview Child", childSize, ImGuiChildFlags_Borders))
      {
        auto& backgroundColor = settings.previewBackgroundColor;
        auto& time = generateTime;
        auto& shaderTexture = resources.shaders[resource::shader::TEXTURE];

        auto previewSize = ImVec2(ImGui::GetContentRegionAvail().x, size_without_footer_get(2).y);

        generate.size_set(to_vec2(previewSize));
        generate.bind();
        generate.viewport_set();
        generate.clear(backgroundColor);

        if (document && document->reference.itemType == anm2::LAYER)
        {
          auto& texture = document->anm2.content
                              .spritesheets[document->anm2.content.layers[document->reference.itemID].spritesheetID]
                              .texture;

          auto index = std::clamp((int)(time * (count - 1)), 0, (count - 1));
          auto row = index / columns;
          auto column = index % columns;
          auto crop = startPosition + ivec2(size.x * column, size.y * row);
          auto uvMin = (vec2(crop) + vec2(0.5f)) / vec2(texture.size);
          auto uvMax = (vec2(crop) + vec2(size) - vec2(0.5f)) / vec2(texture.size);

          mat4 transform = generate.transform_get(zoom) * math::quad_model_get(size, {}, pivot);

          generate.texture_render(shaderTexture, texture.id, transform, vec4(1.0f), {},
                                  math::uv_vertices_get(uvMin, uvMax).data());
        }
        generate.unbind();

        ImGui::Image(generate.texture, previewSize);

        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::SliderFloat("##Time", &time, 0.0f, 1.0f, "");

        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::InputFloat("##Zoom", &zoom, zoomStep, zoomStep, "%.0f%%");
        zoom = glm::clamp(zoom, ZOOM_MIN, ZOOM_MAX);
      }

      ImGui::EndChild();

      auto widgetSize = widget_size_with_row_get(2);

      if (ImGui::Button("Generate", widgetSize))
      {
        auto generate_from_grid = [&]()
        {
          item->frames_generate_from_grid(startPosition, size, pivot, columns, count, delay);
          animation->frameNum = animation->length();
        };

        DOCUMENT_EDIT_PTR(document, "Generate Animation from Grid", Document::FRAMES, generate_from_grid());

        generatePopup.close();
      }

      ImGui::SameLine();

      if (ImGui::Button("Cancel", widgetSize)) generatePopup.close();

      ImGui::EndPopup();
    }

    changePopup.trigger();

    if (ImGui::BeginPopupModal(changePopup.label, &changePopup.isOpen, ImGuiWindowFlags_NoResize))
    {
      auto& isCrop = settings.changeIsCrop;
      auto& isSize = settings.changeIsSize;
      auto& isPosition = settings.changeIsPosition;
      auto& isPivot = settings.changeIsPivot;
      auto& isScale = settings.changeIsScale;
      auto& isRotation = settings.changeIsRotation;
      auto& isDelay = settings.changeIsDelay;
      auto& isTint = settings.changeIsTint;
      auto& isColorOffset = settings.changeIsColorOffset;
      auto& isVisibleSet = settings.changeIsVisibleSet;
      auto& isInterpolatedSet = settings.changeIsInterpolatedSet;
      auto& crop = settings.changeCrop;
      auto& size = settings.changeSize;
      auto& position = settings.changePosition;
      auto& pivot = settings.changePivot;
      auto& scale = settings.changeScale;
      auto& rotation = settings.changeRotation;
      auto& delay = settings.changeDelay;
      auto& tint = settings.changeTint;
      auto& colorOffset = settings.changeColorOffset;
      auto& isVisible = settings.changeIsVisible;
      auto& isInterpolated = settings.changeIsInterpolated;

      auto& isFromSelectedFrame = settings.changeIsFromSelectedFrame;
      auto& numberFrames = settings.changeNumberFrames;

      auto propertiesSize = child_size_get(10);

      if (ImGui::BeginChild("##Properties", propertiesSize, ImGuiChildFlags_Borders))
      {
#define PROPERTIES_WIDGET(body)                                                                                        \
  ImGui::Checkbox(checkboxLabel, &isEnabled);                                                                          \
  ImGui::SameLine();                                                                                                   \
  ImGui::BeginDisabled(!isEnabled);                                                                                    \
  body;                                                                                                                \
  ImGui::EndDisabled();

        auto bool_value = [&](const char* checkboxLabel, const char* valueLabel, bool& isEnabled, bool& value)
        { PROPERTIES_WIDGET(ImGui::Checkbox(valueLabel, &value)); };

        auto color3_value = [&](const char* checkboxLabel, const char* valueLabel, bool& isEnabled, vec3& value)
        { PROPERTIES_WIDGET(ImGui::ColorEdit3(valueLabel, value_ptr(value))); };

        auto color4_value = [&](const char* checkboxLabel, const char* valueLabel, bool& isEnabled, vec4& value)
        { PROPERTIES_WIDGET(ImGui::ColorEdit4(valueLabel, value_ptr(value))); };

        auto float2_value = [&](const char* checkboxLabel, const char* valueLabel, bool& isEnabled, vec2& value)
        { PROPERTIES_WIDGET(ImGui::InputFloat2(valueLabel, value_ptr(value), math::vec2_format_get(value))); };

        auto float_value = [&](const char* checkboxLabel, const char* valueLabel, bool& isEnabled, float& value)
        { PROPERTIES_WIDGET(ImGui::InputFloat(valueLabel, &value, STEP, STEP_FAST, math::float_format_get(value))); };

        auto int_value = [&](const char* checkboxLabel, const char* valueLabel, bool& isEnabled, int& value)
        { PROPERTIES_WIDGET(ImGui::InputInt(valueLabel, &value, STEP, STEP_FAST)); };

#undef PROPERTIES_WIDGET

        float2_value("##Is Crop", "Crop", isCrop, crop);
        float2_value("##Is Size", "Size", isSize, size);
        float2_value("##Is Position", "Position", isPosition, position);
        float2_value("##Is Pivot", "Pivot", isPivot, pivot);
        float2_value("##Is Scale", "Scale", isScale, scale);
        float_value("##Is Rotation", "Rotation", isRotation, rotation);
        int_value("##Is Delay", "Delay", isDelay, delay);
        color4_value("##Is Tint", "Tint", isTint, tint);
        color3_value("##Is Color Offset", "Color Offset", isColorOffset, colorOffset);
        bool_value("##Is Visible", "Visible", isVisibleSet, isVisible);
        ImGui::SameLine();
        bool_value("##Is Interpolated", "Interpolated", isInterpolatedSet, isInterpolated);
      }
      ImGui::EndChild();

      auto settingsSize = child_size_get(2);

      if (ImGui::BeginChild("##Settings", settingsSize, ImGuiChildFlags_Borders))
      {
        ImGui::Checkbox("From Selected Frame", &isFromSelectedFrame);
        ImGui::SetItemTooltip("The frames after the currently referenced frame will be changed with these values.\nIf"
                              " off, will use all frames.");

        ImGui::BeginDisabled(!isFromSelectedFrame);
        input_int_range("Number of Frames", numberFrames, anm2::FRAME_NUM_MIN,
                        item->frames.size() - reference->frameIndex);
        ImGui::SetItemTooltip("Set the number of frames that will be changed.");
        ImGui::EndDisabled();
      }
      ImGui::EndChild();

      auto widgetSize = widget_size_with_row_get(4);

      auto frame_change = [&](anm2::ChangeType type)
      {
        anm2::FrameChange frameChange;
        if (isCrop) frameChange.crop = std::make_optional(crop);
        if (isSize) frameChange.size = std::make_optional(size);
        if (isPosition) frameChange.position = std::make_optional(position);
        if (isPivot) frameChange.pivot = std::make_optional(pivot);
        if (isScale) frameChange.scale = std::make_optional(scale);
        if (isRotation) frameChange.rotation = std::make_optional(rotation);
        if (isDelay) frameChange.delay = std::make_optional(delay);
        if (isTint) frameChange.tint = std::make_optional(tint);
        if (isColorOffset) frameChange.colorOffset = std::make_optional(colorOffset);
        if (isVisibleSet) frameChange.isVisible = std::make_optional(isVisible);
        if (isInterpolatedSet) frameChange.isInterpolated = std::make_optional(isInterpolated);

        DOCUMENT_EDIT_PTR(document, "Change Frame Properties", Document::FRAMES,
                          item->frames_change(frameChange, type,
                                              isFromSelectedFrame && document->frame_get() ? reference->frameIndex : 0,
                                              isFromSelectedFrame ? numberFrames : -1));

        changePopup.close();
      };

      if (ImGui::Button("Add", widgetSize)) frame_change(anm2::ADD);
      ImGui::SameLine();
      if (ImGui::Button("Subtract", widgetSize)) frame_change(anm2::SUBTRACT);
      ImGui::SameLine();
      if (ImGui::Button("Adjust", widgetSize)) frame_change(anm2::ADJUST);
      ImGui::SameLine();
      if (ImGui::Button("Cancel", widgetSize)) changePopup.close();

      ImGui::EndPopup();
    }

    configurePopup.trigger();

    if (ImGui::BeginPopupModal(configurePopup.label, &configurePopup.isOpen, ImGuiWindowFlags_NoResize))
    {
      auto childSize = size_without_footer_get(2);

      if (ImGui::BeginTabBar("##Configure Tabs"))
      {
        if (ImGui::BeginTabItem("General"))
        {
          if (ImGui::BeginChild("##Tab Child", childSize, true))
          {
            ImGui::SeparatorText("File");

            ImGui::Checkbox("Autosaving", &editSettings.fileIsAutosave);
            ImGui::SetItemTooltip("Enables autosaving of documents.");

            ImGui::BeginDisabled(!editSettings.fileIsAutosave);
            input_int_range("Autosave Time (minutes)", editSettings.fileAutosaveTime, 0, 10);
            ImGui::SetItemTooltip("If changed, will autosave documents using this interval.");
            ImGui::EndDisabled();

            ImGui::SeparatorText("Keyboard");

            input_float_range("Repeat Delay (seconds)", editSettings.keyboardRepeatDelay, 0.05f, 1.0f, 0.05f, 0.05f,
                              "%.2f");
            ImGui::SetItemTooltip("Set how often, after repeating begins, key inputs will be fired.");

            input_float_range("Repeat Rate (seconds)", editSettings.keyboardRepeatRate, 0.005f, 1.0f, 0.005f, 0.005f,
                              "%.3f");
            ImGui::SetItemTooltip("Set how often, after repeating begins, key inputs will be fired.");

            ImGui::SeparatorText("UI");

            input_float_range("UI Scale", editSettings.uiScale, 0.5f, 2.0f, 0.25f, 0.25f, "%.2f");
            ImGui::SetItemTooltip("Change the scale of the UI.");

            ImGui::Checkbox("Vsync", &editSettings.isVsync);
            ImGui::SetItemTooltip("Toggle vertical sync; synchronizes program update rate with monitor refresh rate.");

            ImGui::SeparatorText("View");

            input_float_range("Zoom Step", editSettings.viewZoomStep, 10.0f, 250.0f, 10.0f, 10.0f, "%.0f");
            ImGui::SetItemTooltip("When zooming in/out with mouse or shortcut, this value will be used.");
          }
          ImGui::EndChild();

          ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Shortcuts"))
        {
          ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2());

          if (ImGui::BeginChild("##Tab Child", childSize, true))
          {
            if (ImGui::BeginTable("Shortcuts", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY))
            {
              ImGui::TableSetupScrollFreeze(0, 1);
              ImGui::TableSetupColumn("Shortcut");
              ImGui::TableSetupColumn("Value");
              ImGui::TableHeadersRow();

              for (int i = 0; i < SHORTCUT_COUNT; ++i)
              {
                bool isSelected = selectedShortcut == i;

                ShortcutMember member = SHORTCUT_MEMBERS[i];
                std::string* settingString = &(editSettings.*member);
                std::string chordString = isSelected ? "" : *settingString;

                ImGui::PushID(i);
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(SHORTCUT_STRINGS[i]);
                ImGui::TableSetColumnIndex(1);

                if (ImGui::Selectable(chordString.c_str(), isSelected)) selectedShortcut = i;

                ImGui::PopID();

                if (isSelected)
                {
                  ImGuiKeyChord chord{ImGuiKey_None};

                  if (ImGui::IsKeyDown(ImGuiMod_Ctrl)) chord |= ImGuiMod_Ctrl;
                  if (ImGui::IsKeyDown(ImGuiMod_Shift)) chord |= ImGuiMod_Shift;
                  if (ImGui::IsKeyDown(ImGuiMod_Alt)) chord |= ImGuiMod_Alt;
                  if (ImGui::IsKeyDown(ImGuiMod_Super)) chord |= ImGuiMod_Super;

                  for (auto& key : KEY_MAP | std::views::values)
                  {
                    if (ImGui::IsKeyPressed(key))
                    {
                      chord |= key;
                      *settingString = chord_to_string(chord);
                      selectedShortcut = -1;
                      break;
                    }
                  }
                }
              }
              ImGui::EndTable();
            }
            ImGui::EndChild();

            ImGui::PopStyleVar();

            ImGui::EndTabItem();
          }
        }

        ImGui::EndTabBar();
      }

      auto widgetSize = widget_size_with_row_get(3);

      if (ImGui::Button("Save", widgetSize))
      {
        settings = editSettings;
        configurePopup.close();
      }
      ImGui::SetItemTooltip("Use the configured settings.");

      ImGui::SameLine();

      if (ImGui::Button("Use Default Settings", widgetSize)) editSettings = Settings();
      ImGui::SetItemTooltip("Reset the settings to their defaults.");

      ImGui::SameLine();

      if (ImGui::Button("Close", widgetSize)) configurePopup.close();
      ImGui::SetItemTooltip("Close without updating settings.");

      ImGui::EndPopup();
    }

    renderPopup.trigger();

    if (ImGui::BeginPopupModal(renderPopup.label, &renderPopup.isOpen, ImGuiWindowFlags_NoResize))
    {
      auto animation = document ? document->animation_get() : nullptr;
      if (!animation) renderPopup.close();

      auto& playback = document->playback;
      auto& ffmpegPath = settings.renderFFmpegPath;
      auto& path = settings.renderPath;
      auto& format = settings.renderFormat;
      auto& scale = settings.renderScale;
      auto& isRaw = settings.renderIsRawAnimation;
      auto& type = settings.renderType;
      auto& start = manager.recordingStart;
      auto& end = manager.recordingEnd;
      auto& isRange = manager.isRecordingRange;
      auto widgetSize = widget_size_with_row_get(2);
      auto dialogType = type == render::PNGS   ? dialog::PNG_DIRECTORY_SET
                        : type == render::GIF  ? dialog::GIF_PATH_SET
                        : type == render::WEBM ? dialog::WEBM_PATH_SET
                                               : dialog::NONE;

      auto replace_extension = [&]()
      { path = std::filesystem::path(path).replace_extension(render::EXTENSIONS[type]); };

      auto range_to_length = [&]()
      {
        start = 0;
        end = animation->frameNum;
      };

      if (renderPopup.isJustOpened)
      {
        replace_extension();
        if (!isRange) range_to_length();
      }

      if (ImGui::ImageButton("##FFmpeg Path Set", resources.icons[icon::FOLDER].id, icon_size_get()))
        dialog.file_open(dialog::FFMPEG_PATH_SET);
      ImGui::SameLine();
      input_text_string("FFmpeg Path", &ffmpegPath);
      ImGui::SetItemTooltip("Set the path where the FFmpeg installation is located.\nFFmpeg is required to render "
                            "animations.\nhttps://ffmpeg.org");
      dialog.set_string_to_selected_path(ffmpegPath, dialog::FFMPEG_PATH_SET);

      if (ImGui::ImageButton("##Path Set", resources.icons[icon::FOLDER].id, icon_size_get()))
      {
        if (dialogType == dialog::PNG_DIRECTORY_SET)
          dialog.folder_open(dialogType);
        else
          dialog.file_save(dialogType);
      }
      ImGui::SameLine();
      input_text_string(type == render::PNGS ? "Directory" : "Path", &path);
      ImGui::SetItemTooltip("Set the output path or directory for the animation.");
      dialog.set_string_to_selected_path(path, dialogType);

      if (ImGui::Combo("Type", &type, render::STRINGS, render::COUNT)) replace_extension();
      ImGui::SetItemTooltip("Set the type of the output.");

      ImGui::BeginDisabled(type != render::PNGS);
      input_text_string("Format", &format);
      ImGui::SetItemTooltip(
          "For outputted images, each image will use this format.\n{} represents the index of each image.");
      ImGui::EndDisabled();

      ImGui::BeginDisabled(!isRange);
      input_int_range("Start", start, 0, animation->frameNum - 1);
      ImGui::SetItemTooltip("Set the starting time  of the animation.");
      input_int_range("End", end, start + 1, animation->frameNum);
      ImGui::SetItemTooltip("Set the ending time  of the animation.");
      ImGui::EndDisabled();

      ImGui::BeginDisabled(!isRaw);
      input_float_range("Scale", scale, 1.0f, 100.0f, STEP, STEP_FAST, "%.1fx");
      ImGui::SetItemTooltip("Set the output scale of the animation.");
      ImGui::EndDisabled();

      if (ImGui::Checkbox("Custom Range", &isRange))
        if (!isRange) range_to_length();
      ImGui::SetItemTooltip("Toggle using a custom range for the animation.");

      ImGui::SameLine();

      ImGui::Checkbox("Raw", &isRaw);
      ImGui::SetItemTooltip("Record only the raw animation; i.e., only its layers, to its bounds.");

      ImGui::SameLine();

      ImGui::Checkbox("Sound", &settings.timelineIsSound);
      ImGui::SetItemTooltip("Toggle sounds playing with triggers.\nBind sounds to events in the Events window.\nThe "
                            "output animation will use the played sounds.");

      if (ImGui::Button("Render", widgetSize))
      {
        manager.isRecordingStart = true;
        playback.time = start;
        playback.isPlaying = true;
        renderPopup.close();
        manager.progressPopup.open();
      }

      ImGui::SameLine();

      if (ImGui::Button("Cancel", widgetSize)) renderPopup.close();

      ImGui::EndPopup();
    }

    renderPopup.end();

    aboutPopup.trigger();

    if (ImGui::BeginPopupModal(aboutPopup.label, &aboutPopup.isOpen, ImGuiWindowFlags_NoResize))
    {
      if (ImGui::Button("Close")) aboutPopup.close();
      ImGui::EndPopup();
    }

    if (shortcut(settings.shortcutNew, shortcut::GLOBAL)) dialog.file_save(dialog::ANM2_NEW);
    if (shortcut(settings.shortcutOpen, shortcut::GLOBAL)) dialog.file_open(dialog::ANM2_OPEN);
    if (shortcut(settings.shortcutSave, shortcut::GLOBAL)) document->save();
    if (shortcut(settings.shortcutSaveAs, shortcut::GLOBAL)) dialog.file_save(dialog::ANM2_SAVE);
    if (shortcut(settings.shortcutExit, shortcut::GLOBAL)) isQuitting = true;
  }
}
