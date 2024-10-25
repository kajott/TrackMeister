// SPDX-FileCopyrightText: 2024 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

#define _CRT_SECURE_NO_WARNINGS  // disable nonsense MSVC warnings

#include <string>

#include "imgui.h"

#include "version.h"
#include "app.h"
#include "config.h"
#include "config_item.h"

////////////////////////////////////////////////////////////////////////////////

static const char* helpText[] = {
    "F1",                  "show/hide help window",
    "F5",                  "reaload the current module and configuration",
    "F10 or Q",            "quit the application immediately",
    "F11",                 "toggle fullscreen mode",
    "Esc",                 "pause / cancel scanning / press twice to quit",
    "Space",               "pause / continue playback",
    "Tab",                 "show / hide the info and metadata bars",
    "Enter",               "show / hide the hake VU meters",
    "Cursor Left/Right",   "seek backward / forward one order",
    "PageUp / PageDown",   "load previous / next module in the current directory",
    "Ctrl+Home / Ctrl+End","load first / next module in the current directory",
    "file drag&drop",      "load another module",
    "Mouse Wheel",         "manually scroll metadata (stops auto-scrolling)",
    "A",                   "stop / resume metadata auto-scrolling",
    "F",                   "slowly fade out the song",
    "P",                   "show current position in seconds (hold to update)",
    "V",                   "show version number",
    "+ / -",               "increase / decrease volume temporarily",
    "Ctrl+L",              "start loudness scan for the current module",
    "Ctrl+Shift+L",        "start loudness scan for all modules in the directory",
    "Ctrl+Shift+S",        "save default configuration (tm_default.ini)",
    nullptr
};

void Application::uiHelpWindow() {
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(
        vp->WorkPos.x + 0.5f * vp->WorkSize.x,
        vp->WorkPos.y + 0.5f * vp->WorkSize.y
    ), ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
    static std::string windowTitle;
    if (windowTitle.empty()) {
        windowTitle.assign(g_ProductName);
        windowTitle.append(" Help");
    }
    if (ImGui::Begin(windowTitle.c_str(), &m_showHelp, ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize)) {
        if (ImGui::BeginTable("help", 2, ImGuiTableFlags_SizingFixedFit)) {
            for (const char** p_help = helpText;  *p_help;  ++p_help) {
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(*p_help);
            }
            ImGui::EndTable();
        }
    }
    ImGui::End();
}

////////////////////////////////////////////////////////////////////////////////

void Application::uiConfigWindow() {
    float margin = float(m_screenSizeY >> 6);
    ImGui::SetNextWindowPos(ImVec2(
        float(m_metaStartX) - margin,
        0.5f * float(m_infoEndY + m_screenSizeY)
    ), ImGuiCond_FirstUseEver, ImVec2(1.0f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(640.0f, float(m_screenSizeY - m_infoEndY) - 2.0f * margin), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Configuration", &m_showConfig, ImGuiWindowFlags_NoCollapse)) {
        ImGui::End();
        return;
    }

    ImGui::PushStyleColor(ImGuiCol_Button, m_uiConfigShowGlobal ? ImGui::GetStyle().Colors[ImGuiCol_ButtonActive] : ImVec4(0.5f, 0.5f, 0.5f, 0.5f));
    if (ImGui::Button("Global Configuration")) { m_uiConfigShowGlobal = true; }
    ImGui::PopStyleColor(1);
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, m_uiConfigShowGlobal ? ImVec4(0.5f, 0.5f, 0.5f, 0.5f) : ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
    if (ImGui::Button("File-Specific Configuration")) { m_uiConfigShowGlobal = false; }
    ImGui::PopStyleColor(1);

    Config& cfg = m_uiConfigShowGlobal ? m_uiGlobalConfig : m_uiFileConfig;
    bool collapsed = false;
    bool cfgChanged = false;

    for (const ConfigItem *item = g_ConfigItems;  item->valid();  ++item) {
        // shortcuts / skips
        if (item->type == ConfigItem::DataType::SectionHeader) {
            collapsed = !ImGui::CollapsingHeader(item->description, ImGuiTreeNodeFlags_DefaultOpen);
            continue;
        }
        if (collapsed || (item->flags & ConfigItem::Flags::Hidden)
        || (!m_uiConfigShowGlobal && (item->flags & (ConfigItem::Flags::Global | ConfigItem::Flags::Startup)))) {
            continue;
        }

        // generic stuff (status bubble)
        bool isset = cfg.set.contains(item->ordinal);
        bool colored = false;
        std::string reason;
        if (isset && m_uiConfigShowGlobal && (m_fileConfig.set.contains(item->ordinal) || m_uiFileConfig.set.contains(item->ordinal))) {
            reason.append("setting is overridden by a file-specific setting\n");
            ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.8f, 0.2f, 0.0f, 1.0f)); colored = true;
        }
        if (isset && (item->flags & ConfigItem::Flags::Reload) && m_reloadPending) {
            reason.append("setting will become active after reloading (F5)\n");
            if (!colored) { ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.8f, 0.0f, 0.5f, 1.0f)); colored = true; }
        }
        if (isset && (item->flags & ConfigItem::Flags::Startup)) {
            reason.append("setting requires a restart\n");
            if (!colored) { ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.8f, 0.3f, 0.3f, 1.0f)); colored = true; }
        }
        if (isset) {
            if (!colored) { reason.append("setting modified\n"); }
            reason.append("click to revert");
        }
        if (ImGui::RadioButton((std::string("##RB") + item->name).c_str(), isset)) {
            if (isset) {
                cfg.set.remove(item->ordinal);
                item->copy((m_uiConfigShowGlobal || !m_fileConfig.set.contains(item->ordinal)) ? m_globalConfig : m_fileConfig, cfg);
                cfgChanged = true;
            }
        }
        if (colored) { ImGui::PopStyleColor(1); }
        if (!reason.empty()) { ImGui::SetItemTooltip("%s", reason.c_str()); }
        ImGui::SameLine();
        bool itemChanged = false;

        // type-dependent UI
        if (item->type == ConfigItem::DataType::Bool) {
            itemChanged = ImGui::Checkbox(item->name, static_cast<bool*>(item->ptr(cfg)));
        } else if (item->type == ConfigItem::DataType::Int) {
            itemChanged = ImGui::SliderInt(item->name, static_cast<int*>(item->ptr(cfg)), int(item->vmin), int(item->vmax));
        } else if (item->type == ConfigItem::DataType::Float) {
            itemChanged = ImGui::SliderFloat(item->name, static_cast<float*>(item->ptr(cfg)), item->vmin, item->vmax);
        } else if (item->type == ConfigItem::DataType::Enum) {
            itemChanged = ImGui::Combo(item->name, static_cast<int*>(item->ptr(cfg)), item->values);
        } else if (item->type == ConfigItem::DataType::Color) {
            uint32_t& value = *static_cast<uint32_t*>(item->ptr(cfg));
            float color[4] = {
                ( value        & 0xFF) * (1.f / 255.f),
                ((value >>  8) & 0xFF) * (1.f / 255.f),
                ((value >> 16) & 0xFF) * (1.f / 255.f),
                 (value >> 24)         * (1.f / 255.f)
            };
            itemChanged = ImGui::ColorEdit4(item->name, color);
            if (itemChanged) {
                value =  uint32_t(color[0] * 255.f + .5f)
                      | (uint32_t(color[1] * 255.f + .5f) <<  8)
                      | (uint32_t(color[2] * 255.f + .5f) << 16)
                      | (uint32_t(color[3] * 255.f + .5f) << 24);
            }
        } else if (item->type == ConfigItem::DataType::String) {
            std::string& value = *static_cast<std::string*>(item->ptr(cfg));
            if (item->values) {
                if (ImGui::BeginCombo(item->name, value.c_str())) {
                    const char* p = item->values;
                    while (*p) {
                        if (ImGui::Selectable(p, false)) {
                            value.assign(p);
                            itemChanged = true;
                        }
                        p += strlen(p) + 1;
                    }
                    ImGui::EndCombo();
                }
            } else {
                constexpr size_t BufSize = 1024;
                static char buf[BufSize];
                strncpy(buf, value.c_str(), BufSize);
                buf[BufSize - 1] = '\0';
                ImGui::InputText(item->name, buf, BufSize);
                if (ImGui::IsItemDeactivatedAfterEdit()) {
                    value.assign(buf);
                    itemChanged = true;
                }
            }
        } else {
            ImGui::Text("TODO: %s", item->name);
        }

        // generic stuff again (tooltip, change handling)
        if (item->description && item->description[0] && ImGui::BeginItemTooltip()) {
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 30.0f);
            ImGui::TextUnformatted(item->description);
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
        if (itemChanged) {
            if (item->flags & ConfigItem::Flags::Reload) {
                m_reloadPending = true;
            }
            cfg.set.add(item->ordinal);
            cfgChanged = true;
        }
    }

    if (cfgChanged) {
        updateConfig();
        updateImages();
        updateLayout();
    }
    ImGui::End();
}
