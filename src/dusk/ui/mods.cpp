#include "dusk/ui/mods.hpp"

#include "dusk/config.hpp"
#include "dusk/mod_manager.hpp"
#include "m_Do/m_Do_main.h"

#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Element.h>
#include <algorithm>

namespace dusk::ui {

static constexpr const char* kModsRml = R"(
<rml>
<head>
<link type="text/rcss" href="res/rml/window.rcss" />
<link type="text/rcss" href="res/rml/tabbing.rcss" />
</head>
<body>
<window>
<div class="tab-bar" />
<content class="content">
<pane>
<span class="section-heading">Installed Mods</span>
<span id="mod-count" class="detail">No mods found</span>
<div id="mod-list" />
<button id="open-folder-btn">Open Mods Folder</button>
<button id="refresh-btn">Refresh Mods</button>
</pane>
</content>
</window>
</body>
</rml>
)";

ModsWindow::ModsWindow() : Document(kModsRml) {
    mRoot = mDocument->GetElementById("mod-list");
    mModList = mDocument->GetElementById("mod-list");
    mModCount = mDocument->GetElementById("mod-count");

    if (auto* openBtn = mDocument->GetElementById("open-folder-btn")) {
        openBtn->AddEventListener(Rml::EventId::Click,
            [](Rml::Event& ev) {
                mod_manager::open_mods_folder();
                ev.StopPropagation();
            });
    }

    if (auto* refreshBtn = mDocument->GetElementById("refresh-btn")) {
        refreshBtn->AddEventListener(Rml::EventId::Click,
            [this](Rml::Event& ev) {
                mod_manager::refresh_all();
                mNeedsRefresh = true;
                ev.StopPropagation();
            });
    }

    refresh_ui();
}

void ModsWindow::build_mod_list(Rml::Element* parent) {
    auto mods = mod_manager::scan_mods();
    mFocusedIndex = 0;

    if (mods.empty()) {
        if (mModCount) mModCount->SetInnerRML("No mods found. Place mod folders in the mods directory.");
        return;
    }

    if (mModCount) {
        mModCount->SetInnerRML(Rml::String() + std::to_string(mods.size()) + " mod(s) found");
    }

    for (auto& mod : mods) {
        auto* row = parent->CreateChild("div");
        if (!row) continue;
        row->SetClass("mod-row", true);

        // Mod name and info
        auto* info = row->CreateChild("div");
        info->SetClass("mod-info", true);

        Rml::String title = mod.name;
        if (!mod.version.empty()) title += " v" + mod.version;
        auto* nameEl = info->CreateChild("span");
        nameEl->SetClass("mod-name", true);
        nameEl->SetInnerRML(title);

        if (!mod.author.empty()) {
            auto* authorEl = info->CreateChild("span");
            authorEl->SetClass("mod-author detail", true);
            authorEl->SetInnerRML("by " + mod.author);
        }

        if (!mod.description.empty()) {
            auto* descEl = info->CreateChild("span");
            descEl->SetClass("mod-description detail", true);
            descEl->SetInnerRML(mod.description);
        }

        // Enable/disable toggle
        auto* toggle = row->CreateChild("button");
        toggle->SetClass(mod.enabled ? "mod-toggle enabled" : "mod-toggle", true);
        toggle->SetInnerRML(mod.enabled ? "Enabled" : "Disabled");

        // Capture mod ID for callback
        std::string modId = mod.id;
        toggle->AddEventListener(Rml::EventId::Click,
            [this, modId](Rml::Event& ev) {
                auto mods = mod_manager::scan_mods();
                for (auto& m : mods) {
                    if (m.id == modId) {
                        mod_manager::toggle_mod(m);
                        break;
                    }
                }
                mNeedsRefresh = true;
                ev.StopPropagation();
            });
    }
}

void ModsWindow::refresh_ui() {
    if (!mModList) return;

    // Clear existing children
    while (mModList->GetFirstChild()) {
        mModList->RemoveChild(mModList->GetFirstChild());
    }

    build_mod_list(mModList);
    mNeedsRefresh = false;
}

void ModsWindow::update() {
    Document::update();
    if (mNeedsRefresh) {
        refresh_ui();
    }
}

bool ModsWindow::handle_nav_command(Rml::Event& event, NavCommand cmd) {
    if (!mDocument->IsVisible()) return false;

    if (cmd == NavCommand::Cancel) {
        pop();
        event.StopPropagation();
        return true;
    }

    if (cmd == NavCommand::Confirm) {
        if (auto* openBtn = mDocument->GetElementById("open-folder-btn")) {
            openBtn->Click();
            event.StopPropagation();
            return true;
        }
    }

    // Close on Menu command (same as Cancel)
    if (cmd == NavCommand::Menu) {
        pop();
        event.StopPropagation();
        return true;
    }

    return false;
}

} // namespace dusk::ui
