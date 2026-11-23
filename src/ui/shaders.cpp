#include "ui/shaders.h"

#include <filesystem>
#include <format>
#include <vector>

#include <imgui/imgui.h>

#include "path.h"
#include "localization.h"
#include "ui/toast.h"
#include "ui/input.h"
#include "ui/layout.h"

constexpr int SHADER_CARD_LINES = 2;

#define SHADER_UNIFORM_BINDING_COMBO_VALUES                                                                            \
    X(UNIFORM_BINDING_IGNORE)                                                                                          \
    X(UNIFORM_BINDING_MANUAL)                                                                                          \
    X(UNIFORM_BINDING_COLOR_OFFSET)                                                                                    \
    X(UNIFORM_BINDING_FRAME_TINT)                                                                                      \
    X(UNIFORM_BINDING_MAIN_TEXTURE)                                                                                    \
    X(UNIFORM_BINDING_COMPONENTS)                                                                                      \
    X(UNIFORM_BINDING_PLAYBACK_TIME)                                                                                   \
    X(UNIFORM_BINDING_TEXTURE_SIZE)                                                                                    \
    X(UNIFORM_BINDING_TRANSFORM)

inline constexpr UniformBinding SHADER_UNIFORM_BINDING_COMBO_ORDER[] = {
#define X(binding) binding,
    SHADER_UNIFORM_BINDING_COMBO_VALUES
#undef X
};
#undef SHADER_UNIFORM_BINDING_COMBO_VALUES

static bool is_shader_reload_needed(Document& document, Shader* shader)
{
    if (!shader || shader->fragment.empty())
        return false;
    if (!document.shader_get(shader->id))
        return true;
    if (!document.shaderVertexPaths.contains(shader->id) || !document.shaderFragmentPaths.contains(shader->id))
        return true;
    return document.shaderVertexPaths.at(shader->id) != shader->vertex ||
           document.shaderFragmentPaths.at(shader->id) != shader->fragment;
}

static Shader* shader_element_get(Document& document, int id)
{
    return item_get(document.model.content.shaders, id);
}

static void shader_path_edit(Document& document, int shaderId, std::filesystem::path Shader::* member,
                             const std::filesystem::path& value)
{
    auto shader = shader_element_get(document, shaderId);
    if (shader)
    {
        shader->*member = path_backslash_replace(value);
        document.change();
    }
}

static void shader_dialog_update(ShadersPanel& window, Manager& manager, Dialog& dialog, Dialog::Type type,
                                 std::filesystem::path Shader::* member)
{
    if (!dialog.is_selected(type))
        return;

    auto dialogPath = dialog.path;
    auto shaderId = window.dialogShaderId;
    manager.command_push({manager.selected, [dialogPath, shaderId, member](Manager&, Document& document) {
                              shader_path_edit(document, shaderId, member, asset_path_get(document, dialogPath));
                          }});
    window.dialogShaderId = -1;
    dialog.reset();
}

static void shader_path_row_update(ShadersPanel& window, Document& document, Resources& resources, Dialog& dialog,
                                   Shader* shader, std::filesystem::path Shader::* member, Dialog::Type dialogType,
                                   StringType label)
{
    auto shaderPath = shader ? shader->*member : std::filesystem::path{};
    auto isDefault = shaderPath.empty();
    auto displayPath = isDefault ? path_from_utf8(localize.get(BASIC_DEFAULT)) : shaderPath;

    if (ImGui::ImageButton(std::format("##{} Path Set", localize.get(label)).c_str(),
                           resources.icon_id_get(ICON_FOLDER), icon_size_get()))
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
    if (inputWidth < ImGui::GetFrameHeight())
        inputWidth = ImGui::GetFrameHeight();
    ImGui::SetNextItemWidth(inputWidth);
    ImGui::BeginDisabled(isDefault);
    auto isEdited = input_text_path(std::format("##{} Path", localize.get(label)).c_str(), &displayPath);
    ImGui::EndDisabled();
    if (isEdited && shader)
        shader_path_edit(document, shader->id, member, displayPath);

    ImGui::SameLine();
    ImGui::TextUnformatted(localize.get(label));
}

static void shader_uniform_config_save(Document& document, Shader& shaderElement, const ShaderUniform& uniform)
{
    uniform_config_save(shaderElement, uniform);
    document.change();
}

static void shader_uniform_components_from_value_set(ShaderUniform& uniform)
{
    for (int i = 0; i < (int)uniform.components.size(); ++i)
        uniform.components[i].value = uniform.value[i];
}

static void shader_uniform_value_from_components_set(ShaderUniform& uniform)
{
    for (int i = 0; i < (int)uniform.components.size(); ++i)
        uniform.value[i] = uniform.components[i].value;
}

static bool shader_uniform_value_update(ShaderUniform& uniform)
{
    ImGui::SetNextItemWidth(-1.0f);
    switch (uniform.valueType)
    {
        case UNIFORM_VALUE_FLOAT:
            return ImGui::DragFloat("##Value", &uniform.value.x, 0.01f);
        case UNIFORM_VALUE_INT:
        case UNIFORM_VALUE_SAMPLER2D:
            return ImGui::InputInt("##Value", &uniform.intValue);
        case UNIFORM_VALUE_VEC2:
            return ImGui::DragFloat2("##Value", &uniform.value.x, 0.01f);
        case UNIFORM_VALUE_VEC3:
            return ImGui::DragFloat3("##Value", &uniform.value.x, 0.01f);
        case UNIFORM_VALUE_VEC4:
            return ImGui::DragFloat4("##Value", &uniform.value.x, 0.01f);
        default:
            ImGui::TextUnformatted(localize.get(BASIC_NONE));
            return false;
    }
}

static bool shader_uniform_component_binding_update(ShaderUniform::Component& component)
{
    auto bindingLabel = uniform_binding_label_get(component.binding);
    ImGui::SetNextItemWidth(ImGui::GetFrameHeight() * 5.0f);
    if (!ImGui::BeginCombo("##Component Binding", bindingLabel.data()))
        return false;

    bool isChanged = false;
    constexpr UniformBinding bindings[] = {UNIFORM_BINDING_MANUAL, UNIFORM_BINDING_PLAYBACK_TIME};
    for (auto binding : bindings)
    {
        auto isSelected = component.binding == binding;
        auto label = uniform_binding_label_get(binding);
        if (ImGui::Selectable(label.data(), isSelected))
        {
            component.binding = binding;
            isChanged = true;
        }
        if (isSelected)
            ImGui::SetItemDefaultFocus();
    }
    ImGui::EndCombo();
    return isChanged;
}

static bool shader_uniform_components_update(ShaderUniform& uniform)
{
    bool isChanged = false;
    constexpr const char* labels[] = {"X", "Y", "Z", "W"};
    auto componentCount = UNIFORM_VALUE_TYPE_INFOS[uniform.valueType].componentCount;

    for (int index = 0; index < componentCount; ++index)
    {
        auto& component = uniform.components[index];
        ImGui::PushID(index);
        if (index > 0)
            ImGui::Spacing();
        ImGui::TextUnformatted(labels[index]);
        ImGui::SameLine();
        if (shader_uniform_component_binding_update(component))
            isChanged = true;
        ImGui::SameLine();
        ImGui::BeginDisabled(component.binding != UNIFORM_BINDING_MANUAL);
        ImGui::SetNextItemWidth(-1.0f);
        if (ImGui::DragFloat("##Component Value", &component.value, 0.01f))
            isChanged = true;
        ImGui::EndDisabled();
        ImGui::PopID();
    }

    if (isChanged)
        shader_uniform_value_from_components_set(uniform);
    return isChanged;
}

static void shader_uniforms_update(Document& document, Shader* shaderElement, ShaderProgram* runtime, ImVec2 tableSize)
{
    if (!shaderElement)
        return;

    auto flags = ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_RowBg |
                 ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollY;
    if (tableSize.x <= 0.0f)
        tableSize.x = ImGui::GetContentRegionAvail().x;
    if (tableSize.y <= 0.0f)
        tableSize.y = ImGui::GetContentRegionAvail().y;
    if (tableSize.y < ImGui::GetFrameHeight())
        tableSize.y = ImGui::GetFrameHeight();
    if (!ImGui::BeginTable("##Shader Uniforms", 4, flags, tableSize))
        return;

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
            ImGui::TextUnformatted(uniform_value_type_label_get(uniform.valueType).data());

            ImGui::TableNextColumn();
            auto bindingLabel = uniform_binding_label_get(uniform.binding);
            ImGui::SetNextItemWidth(-1.0f);
            if (ImGui::BeginCombo("##Binding", bindingLabel.data()))
            {
                for (auto binding : SHADER_UNIFORM_BINDING_COMBO_ORDER)
                {
                    if (!is_uniform_binding_valid(binding, uniform.valueType))
                        continue;
                    auto isSelected = uniform.binding == binding;
                    auto label = uniform_binding_label_get(binding);
                    if (ImGui::Selectable(label.data(), isSelected))
                    {
                        uniform.binding = binding;
                        if (uniform.binding == UNIFORM_BINDING_COMPONENTS)
                            shader_uniform_components_from_value_set(uniform);
                        shader_uniform_config_save(document, *shaderElement, uniform);
                    }
                    if (isSelected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            ImGui::TableNextColumn();
            if (uniform.binding == UNIFORM_BINDING_COMPONENTS)
            {
                if (shader_uniform_components_update(uniform))
                    shader_uniform_config_save(document, *shaderElement, uniform);
            }
            else
            {
                ImGui::BeginDisabled(uniform.binding != UNIFORM_BINDING_MANUAL);
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

void shaders_update(Panel& panel, ShadersPanel& shaders)
{
    auto& [manager, settings, resources, dialog, clipboard, document, state] = panel;
    auto& items = document.model.content.shaders;
    auto& status = shaders.status;
    auto& propertiesPopup = shaders.propertiesPopup;
    auto& popupShaderId = shaders.popupShaderId;
    shader_dialog_update(shaders, manager, dialog, Dialog::SHADER_VERTEX_PATH_SET, &Shader::vertex);
    shader_dialog_update(shaders, manager, dialog, Dialog::SHADER_FRAGMENT_PATH_SET, &Shader::fragment);

    if (ImGui::Begin(localize.get(LABEL_SHADERS_WINDOW), &settings.windowIsShaders))
    {
        auto selection = document.selected_ids_get(SelectionKind::SHADERS);
        auto properties_open = [&](int id) {
            document.selected_ids_set(SelectionKind::SHADERS, {id});
            popupShaderId = id;
            propertiesPopup.open();
        };

        Actions actions = {};
        named_item_add_action_add(actions, panel, SelectionKind::SHADERS, &Content::shaders, EDIT_ADD_SHADER,
                                  TEXT_NEW_SHADER, TOOLTIP_ADD_SHADER);
        content_remove_action_add(actions, panel, ElementType::SHADER, SelectionKind::SHADERS);
        actions.add(
            ACTION_RELOAD, [selection]() { return !selection.empty(); },
            [&]() {
                status.clear();
                for (auto id : selection)
                    if (auto shader = shader_element_get(document, id))
                    {
                        document.shader_reload(id, &status);
                        toasts.push(localize_format(TOAST_RELOAD_SHADER, id, shader->name));
                    }
            },
            TOOLTIP_RELOAD_SHADERS);

        auto image_get = [&](int id) {
            auto isValid = document.shader_get(id) != nullptr;
            auto& image = resources.icons[isValid ? ICON_SHADER : ICON_NONE];
            return CardImage{.texture = &image, .size = Vec2(image.size), .isValid = isValid};
        };
        ListRows rows{.label_get =
                          [&](int id) {
                              auto name = shader_element_get(document, id)->name;
                              return localize_format(FORMAT_SHADER, id, name);
                          },
                      .tooltip_draw =
                          [&](int id) {
                              auto& shader = *shader_element_get(document, id);
                              tooltip_name_draw(resources, shader.name);
                              ImGui::Text("%s: %d", localize.get(BASIC_ID), id);
                              ImGui::Text("%s: %s", localize.get(LABEL_VERTEX), path_to_utf8(shader.vertex).c_str());
                              ImGui::Text("%s: %s", localize.get(LABEL_FRAGMENT),
                                          path_to_utf8(shader.fragment).c_str());
                              if (image_get(id).isValid)
                                  return;
                              ImGui::Spacing();
                              ImGui::TextWrapped("%s", localize.get(TOOLTIP_SHADER_INVALID));
                          },
                      .image_get = image_get,
                      .activate = properties_open,
                      .cardLines = SHADER_CARD_LINES};
        list_panel_draw(
            panel, actions, {{ACTION_ADD, ACTION_REMOVE, ACTION_RELOAD}},
            [&]() { content_list_draw(panel, SelectionKind::SHADERS, item_ids_get(items), rows); }, true);

        if (propertiesPopup.begin())
        {
            auto shader = shader_element_get(document, popupShaderId);
            if (!shader)
                propertiesPopup.close();
            if (!shader)
            {
                ImGui::EndPopup();
                propertiesPopup.end();
                ImGui::End();
                return;
            }

            auto popupSize = size_without_footer_get();
            if (popupSize.y < ImGui::GetFrameHeight())
                popupSize.y = ImGui::GetFrameHeight();
            if (ImGui::BeginChild("##Shader Properties Child", popupSize))
            {
                if (propertiesPopup.isJustOpened)
                    ImGui::SetKeyboardFocusHere();
                if (input_text_string(localize.get(BASIC_NAME), &shader->name))
                    document.change();

                ImGui::SeparatorText(localize.get(LABEL_FILES));

                shader_path_row_update(shaders, document, resources, dialog, shader, &Shader::vertex,
                                       Dialog::SHADER_VERTEX_PATH_SET, LABEL_VERTEX);
                shader_path_row_update(shaders, document, resources, dialog, shader, &Shader::fragment,
                                       Dialog::SHADER_FRAGMENT_PATH_SET, LABEL_FRAGMENT);

                if (is_shader_reload_needed(document, shader))
                    ImGui::TextWrapped("%s", localize.get(LABEL_SHADER_RELOAD_NEEDED));

                ImGui::SeparatorText(localize.get(LABEL_UNIFORMS));

                auto runtime = document.shader_get(shader->id);
                auto tableSize = ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetTextLineHeightWithSpacing() * 8.0f);
                auto outputReserve = ImGui::GetTextLineHeightWithSpacing() * 6.0f;
                auto availableHeight = ImGui::GetContentRegionAvail().y;
                if (availableHeight - outputReserve < tableSize.y)
                    tableSize.y = availableHeight - outputReserve;
                if (tableSize.y < ImGui::GetFrameHeight())
                    tableSize.y = ImGui::GetFrameHeight();
                shader_uniforms_update(document, shader, runtime, tableSize);

                ImGui::SeparatorText(localize.get(LABEL_OUTPUT));

                auto consoleSize = ImGui::GetContentRegionAvail();
                if (consoleSize.y < ImGui::GetFrameHeight())
                    consoleSize.y = ImGui::GetFrameHeight();
                if (ImGui::BeginChild("##Shaders Output", consoleSize, ImGuiChildFlags_Borders))
                    ImGui::TextWrapped("%s", status.c_str());
                ImGui::EndChild();
            }
            ImGui::EndChild();

            auto popupWidgetSize = widget_size_with_row_get(2);
            if (ImGui::Button(localize.get(BASIC_CONFIRM), popupWidgetSize))
            {
                status.clear();
                document.shader_reload(shader->id, &status);
                propertiesPopup.close();
            }
            tooltip_set(TOOLTIP_RELOAD_SHADERS);
            ImGui::SameLine();
            if (ImGui::Button(localize.get(BASIC_CANCEL), popupWidgetSize))
                propertiesPopup.close();

            ImGui::EndPopup();
        }
        propertiesPopup.end();
    }
    ImGui::End();
}
