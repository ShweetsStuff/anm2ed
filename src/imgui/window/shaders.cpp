#include "shaders.hpp"

#include <filesystem>
#include <format>
#include <vector>

#include <imgui/imgui.h>

#include "path.hpp"
#include "strings.hpp"
#include "toast.hpp"
#include "util/imgui/input.hpp"
#include "util/imgui/layout.hpp"

using namespace anm2ed::resource;
using namespace anm2ed::util;

namespace anm2ed::imgui
{
#define SHADER_UNIFORM_BINDING_COMBO_VALUES                                                                            \
  X(resource::shader::UNIFORM_BINDING_IGNORE)                                                                          \
  X(resource::shader::UNIFORM_BINDING_MANUAL)                                                                          \
  X(resource::shader::UNIFORM_BINDING_COLOR_OFFSET)                                                                    \
  X(resource::shader::UNIFORM_BINDING_FRAME_TINT)                                                                      \
  X(resource::shader::UNIFORM_BINDING_MAIN_TEXTURE)                                                                    \
  X(resource::shader::UNIFORM_BINDING_COMPONENTS)                                                                      \
  X(resource::shader::UNIFORM_BINDING_PLAYBACK_TIME)                                                                   \
  X(resource::shader::UNIFORM_BINDING_TEXTURE_SIZE)                                                                    \
  X(resource::shader::UNIFORM_BINDING_TRANSFORM)

  inline constexpr resource::shader::UniformBinding SHADER_UNIFORM_BINDING_COMBO_ORDER[] = {
#define X(binding) binding,
      SHADER_UNIFORM_BINDING_COMBO_VALUES
#undef X
  };
#undef SHADER_UNIFORM_BINDING_COMBO_VALUES

  std::filesystem::path shader_absolute_path_get(Document& document, const std::filesystem::path& shaderPath)
  {
    auto loadPath = path::backslash_handle(shaderPath);
    if (loadPath.empty() || loadPath.is_absolute()) return loadPath;
    return document.directory_get() / loadPath;
  }

  bool is_shader_reload_needed(Document& document, Element* shader)
  {
    if (!shader || shader->fragment.empty()) return false;
    if (!document.shader_get(shader->id)) return true;
    if (!document.shaderVertexPaths.contains(shader->id) || !document.shaderFragmentPaths.contains(shader->id))
      return true;
    return document.shaderVertexPaths.at(shader->id) != shader->vertex ||
           document.shaderFragmentPaths.at(shader->id) != shader->fragment;
  }

  Element* shader_element_get(Document& document, int id)
  {
    auto shaders = document.anm2.element_get(ElementType::SHADERS);
    return shaders ? element_child_id_get(*shaders, ElementType::SHADER, id) : nullptr;
  }

  std::filesystem::path shader_asset_path_get(Document& document, const std::filesystem::path& path)
  {
    auto loadPath = path::backslash_handle(path);
    return path::backslash_replace(path::make_relative(loadPath, document.directory_get()));
  }

  void shader_path_edit(Document& document, int shaderId, std::filesystem::path Element::* member,
                        const std::filesystem::path& value)
  {
    auto shader = shader_element_get(document, shaderId);
    if (shader)
    {
      shader->*member = path::backslash_replace(value);
      document.change(Document::SHADERS);
    }
  }

  void shader_dialog_update(ShadersWindow& window, Manager& manager, Dialog& dialog, Dialog::Type type,
                            std::filesystem::path Element::* member)
  {
    if (!dialog.is_selected(type)) return;

    auto dialogPath = dialog.path;
    auto shaderId = window.dialogShaderId;
    manager.command_push(
        {manager.selected, [dialogPath, shaderId, member](Manager&, Document& document)
         { shader_path_edit(document, shaderId, member, shader_asset_path_get(document, dialogPath)); }});
    window.dialogShaderId = -1;
    dialog.reset();
  }

  void shader_path_row_update(ShadersWindow& window, Document& document, Resources& resources, Dialog& dialog,
                              Element* shader, std::filesystem::path Element::* member, Dialog::Type dialogType,
                              StringType label)
  {
    auto shaderPath = shader ? shader->*member : std::filesystem::path{};
    auto isDefault = shaderPath.empty();
    auto displayPath = isDefault ? path::from_utf8(localize.get(BASIC_DEFAULT)) : shaderPath;

    if (ImGui::ImageButton(std::format("##{} Path Set", localize.get(label)).c_str(), resources.icons[icon::FOLDER].id,
                           icon_size_get()))
    {
      if (shader)
      {
        window.dialogShaderId = shader->id;
        dialog.file_open(dialogType);
      }
    }

    ImGui::SameLine();

    auto labelWidth = ImGui::CalcTextSize(localize.get(label)).x;
    auto inputWidth = ImGui::GetContentRegionAvail().x - labelWidth - ImGui::GetStyle().ItemSpacing.x;
    if (inputWidth < ImGui::GetFrameHeight()) inputWidth = ImGui::GetFrameHeight();
    ImGui::SetNextItemWidth(inputWidth);
    ImGui::BeginDisabled(isDefault);
    auto isEdited = input_text_path(std::format("##{} Path", localize.get(label)).c_str(), &displayPath);
    ImGui::EndDisabled();
    if (isEdited && shader) shader_path_edit(document, shader->id, member, displayPath);

    ImGui::SameLine();
    ImGui::TextUnformatted(localize.get(label));
  }

  void shader_uniform_config_save(Document& document, Element& shaderElement, const resource::shader::Uniform& uniform)
  {
    auto config = shader_uniform_get(shaderElement, uniform.name, true);
    if (!config) return;

    config->binding = std::string(resource::shader::uniform_binding_value_get(uniform.binding));
    config->value = resource::shader::uniform_value_string_get(uniform);
    if (uniform.binding == resource::shader::UNIFORM_BINDING_COMPONENTS)
    {
      for (int index = 0; index < (int)uniform.components.size(); ++index)
      {
        auto component = shader_uniform_component_get(*config, index, true);
        if (!component) continue;
        component->binding =
            std::string(resource::shader::uniform_binding_value_get(uniform.components[index].binding));
        component->value = std::format("{:.6g}", uniform.components[index].value);
      }
    }
    else
      std::erase_if(config->children, [](const Element& child) { return child.type == ElementType::COMPONENT; });
    document.change(Document::SHADERS);
  }

  int shader_uniform_component_count_get(resource::shader::UniformValueType type)
  {
    if (type == resource::shader::UNIFORM_VALUE_VEC2) return 2;
    if (type == resource::shader::UNIFORM_VALUE_VEC3) return 3;
    if (type == resource::shader::UNIFORM_VALUE_VEC4) return 4;
    return 0;
  }

  void shader_uniform_components_from_value_set(resource::shader::Uniform& uniform)
  {
    uniform.components[0].value = uniform.value.x;
    uniform.components[1].value = uniform.value.y;
    uniform.components[2].value = uniform.value.z;
    uniform.components[3].value = uniform.value.w;
  }

  void shader_uniform_value_from_components_set(resource::shader::Uniform& uniform)
  {
    uniform.value.x = uniform.components[0].value;
    uniform.value.y = uniform.components[1].value;
    uniform.value.z = uniform.components[2].value;
    uniform.value.w = uniform.components[3].value;
  }

  bool shader_uniform_value_update(resource::shader::Uniform& uniform)
  {
    ImGui::SetNextItemWidth(-1.0f);
    switch (uniform.valueType)
    {
      case resource::shader::UNIFORM_VALUE_FLOAT:
        return ImGui::DragFloat("##Value", &uniform.value.x, 0.01f);
      case resource::shader::UNIFORM_VALUE_INT:
      case resource::shader::UNIFORM_VALUE_SAMPLER2D:
        return ImGui::InputInt("##Value", &uniform.intValue);
      case resource::shader::UNIFORM_VALUE_VEC2:
        return ImGui::DragFloat2("##Value", &uniform.value.x, 0.01f);
      case resource::shader::UNIFORM_VALUE_VEC3:
        return ImGui::DragFloat3("##Value", &uniform.value.x, 0.01f);
      case resource::shader::UNIFORM_VALUE_VEC4:
        return ImGui::DragFloat4("##Value", &uniform.value.x, 0.01f);
      default:
        ImGui::TextUnformatted(localize.get(BASIC_NONE));
        return false;
    }
  }

  bool shader_uniform_component_binding_update(resource::shader::Uniform::Component& component)
  {
    auto bindingLabel = resource::shader::uniform_binding_label_get(component.binding);
    ImGui::SetNextItemWidth(ImGui::GetFrameHeight() * 5.0f);
    if (!ImGui::BeginCombo("##Component Binding", bindingLabel.data())) return false;

    bool isChanged{};
    constexpr resource::shader::UniformBinding bindings[] = {resource::shader::UNIFORM_BINDING_MANUAL,
                                                             resource::shader::UNIFORM_BINDING_PLAYBACK_TIME};
    for (auto binding : bindings)
    {
      auto isSelected = component.binding == binding;
      auto label = resource::shader::uniform_binding_label_get(binding);
      if (ImGui::Selectable(label.data(), isSelected))
      {
        component.binding = binding;
        isChanged = true;
      }
      if (isSelected) ImGui::SetItemDefaultFocus();
    }
    ImGui::EndCombo();
    return isChanged;
  }

  bool shader_uniform_components_update(resource::shader::Uniform& uniform)
  {
    bool isChanged{};
    constexpr const char* labels[] = {"X", "Y", "Z", "W"};
    auto componentCount = shader_uniform_component_count_get(uniform.valueType);

    for (int index = 0; index < componentCount; ++index)
    {
      auto& component = uniform.components[index];
      ImGui::PushID(index);
      if (index > 0) ImGui::Spacing();
      ImGui::TextUnformatted(labels[index]);
      ImGui::SameLine();
      if (shader_uniform_component_binding_update(component)) isChanged = true;
      ImGui::SameLine();
      ImGui::BeginDisabled(component.binding != resource::shader::UNIFORM_BINDING_MANUAL);
      ImGui::SetNextItemWidth(-1.0f);
      if (ImGui::DragFloat("##Component Value", &component.value, 0.01f)) isChanged = true;
      ImGui::EndDisabled();
      ImGui::PopID();
    }

    if (isChanged) shader_uniform_value_from_components_set(uniform);
    return isChanged;
  }

  void shader_uniforms_update(Document& document, Element* shaderElement, resource::Shader* runtime, ImVec2 tableSize)
  {
    if (!shaderElement) return;

    auto flags = ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_RowBg |
                 ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollY;
    if (tableSize.x <= 0.0f) tableSize.x = ImGui::GetContentRegionAvail().x;
    if (tableSize.y <= 0.0f) tableSize.y = ImGui::GetContentRegionAvail().y;
    if (tableSize.y < ImGui::GetFrameHeight()) tableSize.y = ImGui::GetFrameHeight();
    if (!ImGui::BeginTable("##Shader Uniforms", 4, flags, tableSize)) return;

    ImGui::TableSetupColumn(localize.get(BASIC_NAME));
    ImGui::TableSetupColumn(localize.get(LABEL_TYPE));
    ImGui::TableSetupColumn(localize.get(LABEL_BINDING));
    ImGui::TableSetupColumn(localize.get(LABEL_VALUE_COLUMN));
    ImGui::TableSetupScrollFreeze(0, 1);
    ImGui::TableHeadersRow();

    if (runtime)
      for (auto& uniform : runtime->uniforms)
      {
        ImGui::PushID(uniform.name.c_str());
        ImGui::TableNextRow();

        ImGui::TableNextColumn();
        ImGui::TextUnformatted(uniform.name.c_str());

        ImGui::TableNextColumn();
        ImGui::TextUnformatted(resource::shader::uniform_value_type_label_get(uniform.valueType).data());

        ImGui::TableNextColumn();
        auto bindingLabel = resource::shader::uniform_binding_label_get(uniform.binding);
        ImGui::SetNextItemWidth(-1.0f);
        if (ImGui::BeginCombo("##Binding", bindingLabel.data()))
        {
          for (auto binding : SHADER_UNIFORM_BINDING_COMBO_ORDER)
          {
            if (!resource::shader::is_uniform_binding_valid(binding, uniform.valueType)) continue;
            auto isSelected = uniform.binding == binding;
            auto label = resource::shader::uniform_binding_label_get(binding);
            if (ImGui::Selectable(label.data(), isSelected))
            {
              uniform.binding = binding;
              if (uniform.binding == resource::shader::UNIFORM_BINDING_COMPONENTS)
                shader_uniform_components_from_value_set(uniform);
              shader_uniform_config_save(document, *shaderElement, uniform);
            }
            if (isSelected) ImGui::SetItemDefaultFocus();
          }
          ImGui::EndCombo();
        }

        ImGui::TableNextColumn();
        if (uniform.binding == resource::shader::UNIFORM_BINDING_COMPONENTS)
        {
          if (shader_uniform_components_update(uniform)) shader_uniform_config_save(document, *shaderElement, uniform);
        }
        else
        {
          ImGui::BeginDisabled(uniform.binding != resource::shader::UNIFORM_BINDING_MANUAL);
          if (shader_uniform_value_update(uniform))
          {
            shader_uniform_components_from_value_set(uniform);
            shader_uniform_config_save(document, *shaderElement, uniform);
          }
          ImGui::EndDisabled();
        }

        ImGui::PopID();
      }

    ImGui::EndTable();
  }

  void ShadersWindow::update(Manager& manager, Settings& settings, Resources& resources, Dialog& dialog)
  {
    if (ImGui::Begin(localize.get(LABEL_SHADERS_WINDOW), &settings.windowIsShaders))
    {
      auto document = manager.get();
      if (!document)
      {
        ImGui::End();
        return;
      }
      auto shaders = document->anm2.element_get(ElementType::SHADERS);
      if (!shaders)
      {
        ImGui::End();
        return;
      }

      shader_dialog_update(*this, manager, dialog, Dialog::SHADER_VERTEX_PATH_SET, &Element::vertex);
      shader_dialog_update(*this, manager, dialog, Dialog::SHADER_FRAGMENT_PATH_SET, &Element::fragment);

      auto shader_add = [&]()
      {
        manager.command_push({manager.selected, [this](Manager&, Document& document)
                              {
                                auto shaders = document.anm2.element_get(ElementType::SHADERS);
                                if (!shaders) return;

                                auto shader = element_make(ElementType::SHADER);
                                shader.id = element_child_next_id_get(*shaders, ElementType::SHADER);
                                shader.name = localize.get(TEXT_NEW_SHADER);

                                document.anm2_snapshot(localize.get(EDIT_ADD_SHADER));
                                shaders->children.push_back(shader);
                                document.shader.selection = {shader.id};
                                document.shader.reference = shader.id;
                                newElementId = shader.id;
                                document.anm2_change(Document::SHADERS);
                              }});
      };

      auto unused_shaders_remove = [&]()
      {
        manager.command_push({manager.selected, [this](Manager&, Document& document)
                              {
                                auto shaders = document.anm2.element_get(ElementType::SHADERS);
                                if (!shaders) return;
                                auto unused = document.anm2.element_unused(ElementType::SHADER);
                                if (unused.empty()) return;

                                document.anm2_snapshot(localize.get(EDIT_REMOVE_UNUSED_SHADERS));
                                for (auto id : unused)
                                {
                                  element_child_id_erase(*shaders, ElementType::SHADER, id);
                                  document.shader.selection.erase(id);
                                  if (document.shader.reference == id) document.shader.reference = -1;
                                  if (popupShaderId == id) propertiesPopup.close();
                                }
                                document.anm2_change(Document::SHADERS);
                              }});
      };

      auto& selection = document->shader.selection;
      auto& reference = document->shader.reference;
      std::vector<int> ids{};
      for (auto& shader : shaders->children)
        if (shader.type == ElementType::SHADER) ids.push_back(shader.id);

      if (reference != -1 && !element_child_id_get(*shaders, ElementType::SHADER, reference)) reference = -1;
      for (auto it = selection.begin(); it != selection.end();)
      {
        if (!element_child_id_get(*shaders, ElementType::SHADER, *it))
          it = selection.erase(it);
        else
          ++it;
      }

      auto contentSize = size_without_footer_get();
      if (contentSize.y < ImGui::GetFrameHeight()) contentSize.y = ImGui::GetFrameHeight();
      auto tooltipWindowPadding = ImGui::GetStyle().WindowPadding;
      auto tooltipItemSpacing = ImGui::GetStyle().ItemSpacing;
      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2());
      if (ImGui::BeginChild("##Shaders Child", contentSize, ImGuiChildFlags_Borders))
      {
        auto style = ImGui::GetStyle();
        auto shaderChildSize = ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetTextLineHeightWithSpacing() * 2);

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2());
        selection.start(ids.size());
        if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_A, ImGuiInputFlags_RouteFocused))
        {
          selection.clear();
          for (auto id : ids)
            selection.insert(id);
        }
        if (ImGui::Shortcut(ImGuiKey_Escape, ImGuiInputFlags_RouteFocused))
        {
          selection.clear();
          reference = -1;
        }

        for (auto& shader : shaders->children)
        {
          if (shader.type != ElementType::SHADER) continue;
          auto id = shader.id;
          auto isNewShader = newElementId == id;
          ImGui::PushID(id);

          if (ImGui::BeginChild("##Shader Child", shaderChildSize, ImGuiChildFlags_Borders))
          {
            auto isSelected = selection.contains(id);
            auto cursorPos = ImGui::GetCursorPos();
            auto runtime = document->shader_get(id);
            bool isValid = runtime != nullptr;
            auto& shaderIcon = isValid ? resources.icons[icon::SHADER] : resources.icons[icon::NONE];
            auto tintColor = !isValid ? ImVec4(1.0f, 0.25f, 0.25f, 1.0f) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

            ImGui::SetNextItemSelectionUserData(id);
            ImGui::SetNextItemStorageID(id);
            auto isActivated = ImGui::Selectable("##Shader Selectable", isSelected, 0, shaderChildSize);
            auto isClicked =
                ImGui::IsItemClicked(ImGuiMouseButton_Left) || ImGui::IsItemClicked(ImGuiMouseButton_Right);
            if (isActivated || isClicked) reference = id;
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            {
              selection = {id};
              reference = id;
              popupShaderId = id;
              propertiesPopup.open();
            }

            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, tooltipItemSpacing);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, tooltipWindowPadding);
            if (ImGui::BeginItemTooltip())
            {
              ImGui::PushFont(resources.fonts[font::BOLD].get(), font::SIZE);
              ImGui::TextUnformatted(shader.name.c_str());
              ImGui::PopFont();
              ImGui::Text("%s: %d", localize.get(BASIC_ID), id);
              ImGui::Text("%s: %s", localize.get(LABEL_VERTEX), path::to_utf8(shader.vertex).c_str());
              ImGui::Text("%s: %s", localize.get(LABEL_FRAGMENT), path::to_utf8(shader.fragment).c_str());
              if (!isValid)
              {
                ImGui::Spacing();
                ImGui::TextWrapped("%s", localize.get(TOOLTIP_SHADER_INVALID));
              }
              ImGui::EndTooltip();
            }
            ImGui::PopStyleVar(2);

            ImGui::SetCursorPos(cursorPos);
            auto imageSize = ImVec2(shaderChildSize.y, shaderChildSize.y);
            ImGui::ImageWithBg(shaderIcon.id, imageSize, ImVec2(), ImVec2(1, 1), ImVec4(), tintColor);

            ImGui::SetCursorPos(ImVec2(shaderChildSize.y + style.ItemSpacing.x,
                                       shaderChildSize.y - shaderChildSize.y / 2 - ImGui::GetTextLineHeight() / 2));
            auto label = std::vformat(localize.get(FORMAT_SHADER), std::make_format_args(id, shader.name));
            ImGui::TextUnformatted(label.c_str());
          }
          ImGui::EndChild();

          if (isNewShader)
          {
            ImGui::SetScrollHereY(0.5f);
            newElementId = -1;
          }

          ImGui::PopID();
        }

        selection.finish();
        ImGui::PopStyleVar();
      }
      ImGui::EndChild();
      ImGui::PopStyleVar();

      auto widgetSize = widget_size_with_row_get(3);
      if (ImGui::Button(localize.get(BASIC_ADD), widgetSize)) shader_add();
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_ADD_SHADER));

      ImGui::SameLine();

      if (ImGui::Button(localize.get(BASIC_REMOVE_UNUSED), widgetSize)) unused_shaders_remove();
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_REMOVE_UNUSED_SHADERS));

      ImGui::SameLine();

      ImGui::BeginDisabled(selection.empty());
      if (ImGui::Button(localize.get(BASIC_RELOAD), widgetSize))
      {
        status.clear();
        for (auto id : selection)
        {
          auto shader = shader_element_get(*document, id);
          if (!shader) continue;
          document->shader_reload(id, &status);
          toasts.push(std::vformat(localize.get(TOAST_RELOAD_SHADER), std::make_format_args(id, shader->name)));
        }
      }
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_RELOAD_SHADERS));
      ImGui::EndDisabled();

      propertiesPopup.trigger();
      if (ImGui::BeginPopupModal(propertiesPopup.label(), &propertiesPopup.isOpen, ImGuiWindowFlags_NoResize))
      {
        auto shader = shader_element_get(*document, popupShaderId);
        if (!shader)
        {
          propertiesPopup.close();
          ImGui::EndPopup();
          propertiesPopup.end();
          ImGui::End();
          return;
        }

        auto popupSize = size_without_footer_get();
        if (popupSize.y < ImGui::GetFrameHeight()) popupSize.y = ImGui::GetFrameHeight();
        if (ImGui::BeginChild("##Shader Properties Child", popupSize))
        {
          if (propertiesPopup.isJustOpened) ImGui::SetKeyboardFocusHere();
          if (input_text_string(localize.get(BASIC_NAME), &shader->name)) document->change(Document::SHADERS);

          ImGui::SeparatorText(localize.get(LABEL_FILES));

          shader_path_row_update(*this, *document, resources, dialog, shader, &Element::vertex,
                                 Dialog::SHADER_VERTEX_PATH_SET, LABEL_VERTEX);
          shader_path_row_update(*this, *document, resources, dialog, shader, &Element::fragment,
                                 Dialog::SHADER_FRAGMENT_PATH_SET, LABEL_FRAGMENT);

          if (is_shader_reload_needed(*document, shader))
            ImGui::TextWrapped("%s", localize.get(LABEL_SHADER_RELOAD_NEEDED));

          ImGui::SeparatorText(localize.get(LABEL_UNIFORMS));

          auto runtime = document->shader_get(shader->id);
          auto tableSize = ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetTextLineHeightWithSpacing() * 8.0f);
          auto outputReserve = ImGui::GetTextLineHeightWithSpacing() * 6.0f;
          auto availableHeight = ImGui::GetContentRegionAvail().y;
          if (availableHeight - outputReserve < tableSize.y) tableSize.y = availableHeight - outputReserve;
          if (tableSize.y < ImGui::GetFrameHeight()) tableSize.y = ImGui::GetFrameHeight();
          shader_uniforms_update(*document, shader, runtime, tableSize);

          ImGui::SeparatorText(localize.get(LABEL_OUTPUT));

          auto consoleSize = ImGui::GetContentRegionAvail();
          if (consoleSize.y < ImGui::GetFrameHeight()) consoleSize.y = ImGui::GetFrameHeight();
          if (ImGui::BeginChild("##Shaders Output", consoleSize, ImGuiChildFlags_Borders))
            ImGui::TextWrapped("%s", status.c_str());
          ImGui::EndChild();
        }
        ImGui::EndChild();

        auto popupWidgetSize = widget_size_with_row_get(2);
        if (ImGui::Button(localize.get(BASIC_CONFIRM), popupWidgetSize))
        {
          status.clear();
          document->shader_reload(shader->id, &status);
          propertiesPopup.close();
        }
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_RELOAD_SHADERS));
        ImGui::SameLine();
        if (ImGui::Button(localize.get(BASIC_CANCEL), popupWidgetSize)) propertiesPopup.close();

        ImGui::EndPopup();
      }
      propertiesPopup.end();
    }
    ImGui::End();
  }
}
