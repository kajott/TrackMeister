// SPDX-FileCopyrightText: 2024 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

#include <string>

#include "imgui.h"

#include "version.h"
#include "app.h"

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
