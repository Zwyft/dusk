#include "dusk/ui/mods.hpp"

#include "dusk/config.hpp"
#include "dusk/mod_manager.hpp"
#include "m_Do/m_Do_main.h"

#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Element.h>
#include <algorithm>

namespace dusk::ui {

static Rml::String escape_rml(const Rml::String& text) {
    Rml::String out;
    out.reserve(text.size());
    for (char c : text) {
        switch (c) {
        case '<': out += "&lt;"; break;
        case '>': out += "&gt;"; break;
        case '&': out += "&amp;"; break;
        case '"': out += "&quot;"; break;
        default: out += c; break;
        }
    }
    return out;
}

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
<button id="back-btn">Back</button>
</pane>
</content>
</window>
</body>
</rml>
)";

ModsWindow::ModsWindow() : Document(kModsRml) {
    if (!mDocument) return;

    mRoot = mDocument->GetElementById("mod-list");
    mModList = mDocument->GetElementById("mod-list");
    mModCount = mDocument->GetElementById("mod-count");

    if (auto* openBtn = mDocument->GetElementById("open-folder-btn")) {
        listen(openBtn, Rml::EventId::Click,
            [](Rml::Event& ev) {
                mod_manager::open_mods_folder();
                ev.StopPropagation();
            });
    }

    if (auto* refreshBtn = mDocument->GetElementById("refresh-btn")) {
        listen(refreshBtn, Rml::EventId::Click,
            [this](Rml::Event& ev) {
                mod_manager::refresh_all();
                mNeedsRefresh = true;
                ev.StopPropagation();
            });
    }

    if (auto* backBtn = mDocument->GetElementById("back-btn")) {
        listen(backBtn, Rml::EventId::Click,
            [this](Rml::Event& ev) {
                pop();
                ev.StopPropagation();
            });
    }

    refresh_ui();
}

void ModsWindow::show() {
    Document::show();
    if (mRoot) {
        mRoot->SetAttribute("open", "");
    }
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
        auto* doc = parent ? parent->GetOwnerDocument() : nullptr;
        if (!doc) continue;
        auto row = doc->CreateElement("div");
        if (!row) continue;
        row->SetClass("mod-row", true);

        // Mod name and info
        auto info = doc->CreateElement("div");
        info->SetClass("mod-info", true);

        Rml::String title = mod.name;
        if (!mod.version.empty()) title += " v" + mod.version;
        auto nameEl = doc->CreateElement("span");
        nameEl->SetClass("mod-name", true);
        nameEl->SetInnerRML(escape_rml(title));
        info->AppendChild(std::move(nameEl));

        if (!mod.author.empty()) {
            auto authorEl = doc->CreateElement("span");
            authorEl->SetClass("mod-author detail", true);
            authorEl->SetInnerRML("by " + escape_rml(mod.author));
            info->AppendChild(std::move(authorEl));
        }

        if (!mod.description.empty()) {
            auto descEl = doc->CreateElement("span");
            descEl->SetClass("mod-description detail", true);
            descEl->SetInnerRML(escape_rml(mod.description));
            info->AppendChild(std::move(descEl));
        }

        row->AppendChild(std::move(info));

        // Enable/disable toggle
        auto toggle = doc->CreateElement("button");
        toggle->SetClass(mod.enabled ? "mod-toggle enabled" : "mod-toggle", true);
        toggle->SetInnerRML(mod.enabled ? "Enabled" : "Disabled");

        // Capture mod ID for callback
        std::string modId = mod.id;
        auto* togglePtr = toggle.get();
        row->AppendChild(std::move(toggle));
        parent->AppendChild(std::move(row));

        listen(togglePtr, Rml::EventId::Click,
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

    mModList->SetInnerRML("");

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
    if (!mDocument || !mDocument->IsVisible()) return false;

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
