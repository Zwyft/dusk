#include "dusk/ui/mods.hpp"

#include "dusk/config.hpp"
#include "dusk/mod_manager.hpp"
#include "fmt/format.h"
#include "m_Do/m_Do_main.h"
#include "pane.hpp"

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

ModsWindow::ModsWindow() : Window() {
    add_tab("Mods", [this](Rml::Element* content) {
        auto& leftPane = add_child<Pane>(content, Pane::Type::Uncontrolled);
        auto& rightPane = add_child<Pane>(content, Pane::Type::Uncontrolled);

        refresh_mod_list(leftPane, rightPane);

        leftPane.add_text("");
        leftPane.register_control(
            leftPane.add_button("Refresh Mods"),
            rightPane, [](Pane& pane) {
                pane.clear();
                pane.add_text("Refresh the mod list and re-link all enabled mod textures.");
            });
    });
}

void ModsWindow::update() {
    Window::update();
    if (mNeedsRefresh) {
        refresh_active_tab();
        mNeedsRefresh = false;
    }
}

void ModsWindow::refresh_mod_list(Pane& leftPane, Pane& rightPane) {
    auto mods = mod_manager::scan_mods();

    if (mods.empty()) {
        leftPane.add_text("No mods found. Place mod folders in the mods directory.");
        return;
    }

    leftPane.add_section(fmt::format("Installed Mods ({})", mods.size()), true);

    for (const auto& mod : mods) {
        std::string id = mod.id;
        bool enabled = mod.enabled;
        uint32_t textureCount = mod.textureCount;
        Rml::String displayName = escape_rml(mod.name);
        if (!mod.version.empty()) displayName += " v" + escape_rml(mod.version);
        Rml::String author = escape_rml(mod.author);
        Rml::String desc = escape_rml(mod.description);

        leftPane.register_control(
            leftPane.add_button(displayName),
            rightPane, [id, enabled, textureCount, displayName, author, desc, this](Pane& pane) {
                pane.clear();
                auto rml = fmt::format("<b>{}</b>", displayName);
                if (!author.empty()) rml += fmt::format("<br/><br/><i>by {}</i>", author);
                if (!desc.empty()) rml += fmt::format("<br/><br/>{}", desc);
                rml += fmt::format("<br/><br/>Textures: {}<br/>Status: {}",
                    textureCount, enabled ? "Enabled" : "Disabled");
                pane.add_rml(rml);
                pane.add_button(enabled ? "Disable" : "Enable").on_pressed([this, id] {
                    mDoAud_seStartMenu(kSoundItemChange);
                    auto mods = mod_manager::scan_mods();
                    for (auto& m : mods) {
                        if (m.id == id) {
                            mod_manager::toggle_mod(m);
                            mNeedsRefresh = true;
                            break;
                        }
                    }
                });
            });
    }
}

} // namespace dusk::ui
