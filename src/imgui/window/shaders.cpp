#include "shaders.hpp"

#include <algorithm>
#include <filesystem>
#include <format>

#include <imgui/imgui.h>

#include "path.hpp"
#include "strings.hpp"
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

  bool is_shader_reload_needed(Document& document, Element& spritesheet, Element* shader)
  {
    if (!shader || !shader->isEnabled || shader->fragment.empty()) return false;
    if (!document.shader_get(spritesheet.id)) return true;
    if (!document.shaderVertexPaths.contains(spritesheet.id) || !document.shaderFragmentPaths.contains(spritesheet.id))
      return true;
    return document.shaderVertexPaths.at(spritesheet.id) != shader->vertex ||
           document.shaderFragmentPaths.at(spritesheet.id) != shader->fragment;
  }

  Element* shader_spritesheet_get(Document& document)
  {
    if (document.spritesheet.selection.size() == 1)
      return document.anm2.element_get(ElementType::SPRITESHEET, *document.spritesheet.selection.begin());
    return nullptr;
  }

  Element* shader_element_get(Element& spritesheet, bool isCreate)
  {
    if (auto shader = element_child_first_get(spritesheet, ElementType::SHADER)) return shader;
    if (!isCreate) return nullptr;

    spritesheet.children.push_back(element_make(ElementType::SHADER));
    return &spritesheet.children.back();
  }

  void shader_enabled_set(Document& document, Element& spritesheet, bool isEnabled)
  {
    if (auto shader = shader_element_get(spritesheet, true))
    {
      shader->isEnabled = isEnabled;
      document.change(Document::SPRITESHEETS);
    }
  }

  std::filesystem::path shader_asset_path_get(Document& document, const std::filesystem::path& path)
  {
    auto loadPath = path::backslash_handle(path);
    return path::backslash_replace(path::make_relative(loadPath, document.directory_get()));
  }

  void shader_path_edit(Document& document, Element& spritesheet, std::filesystem::path Element::* member,
                        const std::filesystem::path& value)
  {
    if (auto shader = shader_element_get(spritesheet, true))
    {
      shader->*member = path::backslash_replace(value);
      document.change(Document::SPRITESHEETS);
    }
  }

  void shader_dialog_update(ShadersWindow& window, Manager& manager, Dialog& dialog, Dialog::Type type,
                            std::filesystem::path Element::* member)
  {
    if (!dialog.is_selected(type)) return;

    auto dialogPath = dialog.path;
    auto spritesheetId = window.dialogSpritesheetId;
    manager.command_push({manager.selected,
                          [dialogPath, spritesheetId, member](Manager&, Document& document)
                          {
                            auto spritesheet = document.anm2.element_get(ElementType::SPRITESHEET, spritesheetId);
                            if (!spritesheet) return;
                            shader_path_edit(document, *spritesheet, member,
                                             shader_asset_path_get(document, dialogPath));
                          }});
    window.dialogSpritesheetId = -1;
    dialog.reset();
  }

  void shader_path_row_update(ShadersWindow& window, Document& document, Resources& resources, Dialog& dialog,
                              Element* spritesheet, std::filesystem::path Element::* member, Dialog::Type dialogType,
                              StringType label)
  {
    auto shader = spritesheet ? shader_element_get(*spritesheet, false) : nullptr;
    auto shaderPath = shader ? shader->*member : std::filesystem::path{};
    auto isDefault = shaderPath.empty();
    auto displayPath = isDefault ? path::from_utf8(localize.get(BASIC_DEFAULT)) : shaderPath;

    if (ImGui::ImageButton(std::format("##{} Path Set", localize.get(label)).c_str(),
                           resources.icons[icon::FOLDER].id, icon_size_get()))
    {
      if (spritesheet)
      {
        window.dialogSpritesheetId = spritesheet->id;
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
    if (isEdited && spritesheet)
      shader_path_edit(document, *spritesheet, member, displayPath);

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
        component->binding = std::string(resource::shader::uniform_binding_value_get(uniform.components[index].binding));
        component->value = std::format("{:.6g}", uniform.components[index].value);
      }
    }
    else
      std::erase_if(config->children, [](const Element& child) { return child.type == ElementType::COMPONENT; });
    document.change(Document::SPRITESHEETS);
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

  void shader_uniforms_update(Document& document, Element* shaderElement, resource::Shader* runtime)
  {
    if (!shaderElement || !runtime) return;

    if (runtime->uniforms.empty())
    {
      ImGui::TextUnformatted(localize.get(BASIC_NONE));
      return;
    }

    auto flags = ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_RowBg |
                 ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollY;
    auto tableSize = ImGui::GetContentRegionAvail();
    if (tableSize.y < ImGui::GetFrameHeight()) tableSize.y = ImGui::GetFrameHeight();
    if (!ImGui::BeginTable("##Shader Uniforms", 4, flags, tableSize)) return;

    ImGui::TableSetupColumn(localize.get(BASIC_NAME));
    ImGui::TableSetupColumn(localize.get(LABEL_TYPE));
    ImGui::TableSetupColumn(localize.get(LABEL_BINDING));
    ImGui::TableSetupColumn(localize.get(LABEL_VALUE_COLUMN));
    ImGui::TableSetupScrollFreeze(0, 1);
    ImGui::TableHeadersRow();

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
      auto spritesheet = document ? shader_spritesheet_get(*document) : nullptr;
      auto shader = spritesheet ? shader_element_get(*spritesheet, false) : nullptr;
      auto isShaderEnabled = !shader || shader->isEnabled;
      auto isReloadNeeded = document && spritesheet && is_shader_reload_needed(*document, *spritesheet, shader);

      shader_dialog_update(*this, manager, dialog, Dialog::SHADER_VERTEX_PATH_SET, &Element::vertex);
      shader_dialog_update(*this, manager, dialog, Dialog::SHADER_FRAGMENT_PATH_SET, &Element::fragment);

      if (!spritesheet)
      {
        ImGui::TextUnformatted(localize.get(TEXT_SELECT_SPRITESHEET));
        ImGui::End();
        return;
      }

      auto contentSize = size_without_footer_get();
      if (contentSize.y < ImGui::GetFrameHeight()) contentSize.y = ImGui::GetFrameHeight();
      if (ImGui::BeginChild("##Shaders Child", contentSize, ImGuiChildFlags_Borders))
      {
        ImGui::BeginDisabled(!isShaderEnabled);
        if (ImGui::BeginTabBar("##Shaders Tabs"))
        {
          if (ImGui::BeginTabItem(localize.get(LABEL_FILES)))
          {
            if (document)
            {
              shader_path_row_update(*this, *document, resources, dialog, spritesheet, &Element::vertex,
                                     Dialog::SHADER_VERTEX_PATH_SET, LABEL_VERTEX);
              shader_path_row_update(*this, *document, resources, dialog, spritesheet, &Element::fragment,
                                     Dialog::SHADER_FRAGMENT_PATH_SET, LABEL_FRAGMENT);
            }

            if (isReloadNeeded) ImGui::TextWrapped("%s", localize.get(LABEL_SHADER_RELOAD_NEEDED));

            ImGui::Separator();

            auto consoleSize = ImGui::GetContentRegionAvail();
            if (consoleSize.y < ImGui::GetFrameHeight()) consoleSize.y = ImGui::GetFrameHeight();
            if (ImGui::BeginChild("##Shaders Console", consoleSize, ImGuiChildFlags_Borders))
              ImGui::TextWrapped("%s", status.c_str());
            ImGui::EndChild();

            ImGui::EndTabItem();
          }
          if (ImGui::BeginTabItem(localize.get(LABEL_UNIFORMS)))
          {
            shader_uniforms_update(*document, shader, document->shader_get(spritesheet->id));
            ImGui::EndTabItem();
          }
          ImGui::EndTabBar();
        }
        ImGui::EndDisabled();
      }
      ImGui::EndChild();

      auto widgetSize = widget_size_with_row_get(2);
      if (ImGui::Button(localize.get(isShaderEnabled ? BASIC_ENABLED : BASIC_DISABLED), widgetSize))
      {
        auto spritesheetId = spritesheet->id;
        auto isNextEnabled = !isShaderEnabled;
        manager.command_push({manager.selected,
                              [spritesheetId, isNextEnabled](Manager&, Document& document)
                              {
                                auto spritesheet = document.anm2.element_get(ElementType::SPRITESHEET, spritesheetId);
                                if (spritesheet) shader_enabled_set(document, *spritesheet, isNextEnabled);
                              }});
      }

      ImGui::SameLine();

      ImGui::BeginDisabled(!isShaderEnabled || !document);
      if (ImGui::Button(localize.get(BASIC_RELOAD), widgetSize) && document)
      {
        status.clear();
        document->shader_reload(spritesheet->id, &status);
      }
      ImGui::EndDisabled();
    }
    ImGui::End();
  }
}
