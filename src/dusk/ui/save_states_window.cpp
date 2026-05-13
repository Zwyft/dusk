#include "save_states_window.hpp"
#include "dusk/save_states.hpp"
#include "pane.hpp"
#include "button.hpp"
#include "dusk/main.h"
#include "dusk/config.hpp"
#include "m_Do/m_Do_main.h"
#include "dusk/file_select.hpp"
#include "aurora/lib/window.hpp"
#include <SDL3/SDL_clipboard.h>

#include <fmt/format.h>

namespace dusk::ui {

namespace {

Rml::String formatTimeAgo(std::chrono::system_clock::time_point timestamp) {
    auto now = std::chrono::system_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::seconds>(now - timestamp).count();
    if (diff < 60) return fmt::format("{}s ago", diff);
    if (diff < 3600) return fmt::format("{}m ago", diff / 60);
    if (diff < 86400) return fmt::format("{}h ago", diff / 3600);
    return fmt::format("{}d ago", diff / 86400);
}

} // namespace

SaveStatesWindow::SaveStatesWindow() : Window() {
    add_tab("Quick Saves", [this](Rml::Element* content) {
        build_quick_saves_tab(content);
    });
    add_tab("Named States", [this](Rml::Element* content) {
        build_named_states_tab(content);
    });
    set_active_tab(0);
}

void SaveStatesWindow::build_quick_saves_tab(Rml::Element* content) {
    auto& leftPane = add_child<Pane>(content, Pane::Type::Controlled);
    auto& rightPane = add_child<Pane>(content, Pane::Type::Uncontrolled);

    dusk::SaveStates& states = dusk::getSaveStates();

    leftPane.add_section("Quick Saves");
    rightPane.add_text("Quick save slots with stage reload. Use Full for instant load without stage transition.");

    static constexpr int kSlots = 4;

    for (int i = 0; i < kSlots; ++i) {
        leftPane.add_rml("<br/>");

        int slot = i;
        auto& slotInfo = states.getQuickSaves()[slot];
        Rml::String info;
        if (slotInfo.occupied) {
            info = fmt::format("{} ({})",
                states.quickSaveInfo(slot),
                slotInfo.isFullState ? "Full" : "Save");
        } else {
            info = "Empty";
        }

        leftPane.register_control(
            leftPane.add_button(fmt::format("Slot {}: {}", slot + 1, info)).on_pressed([this, slot] {
                mDoAud_seStartMenu(kSoundItemChange);
            }),
            rightPane, [this, slot](Pane& pane) {
                const auto& saves = dusk::getSaveStates().getQuickSaves();
                if (!saves[slot].occupied) {
                    pane.clear();
                    pane.add_text("Empty slot. Use Save or Full to create a save.");
                    return;
                }
                pane.clear();
                const auto& save = saves[slot];
                pane.add_rml(fmt::format("<b>Stage:</b> {}<br/><b>Room:</b> {}<br/><b>Type:</b> {}<br/><b>Saved:</b> {}",
                    save.stageName, (int)save.roomNo,
                    save.isFullState ? "Full State" : "Stage Reload",
                    formatTimeAgo(save.timestamp)));
            });

        bool gameRunning = dusk::IsGameLaunched && !dusk::getTransientSettings().stateShareLoadActive;

        if (gameRunning) {
            leftPane.register_control(
                leftPane.add_button("Save").on_pressed([slot]() {
                    mDoAud_seStartMenu(kSoundClick);
                    dusk::getSaveStates().quickSave(slot);
                }),
                rightPane, [](Pane& pane) {
                    pane.clear();
                    pane.add_text("Save current game state using stage reload method.");
                });

            leftPane.register_control(
                leftPane.add_button("Full").on_pressed([slot]() {
                    mDoAud_seStartMenu(kSoundClick);
                    dusk::getSaveStates().quickSaveFull(slot);
                }),
                rightPane, [](Pane& pane) {
                    pane.clear();
                    pane.add_text("Save full actor snapshot for instant load without stage transition.");
                });
        }

        if (states.hasQuickSave(slot)) {
            leftPane.register_control(
                leftPane.add_button("Load").on_pressed([slot]() {
                    mDoAud_seStartMenu(kSoundClick);
                    dusk::SaveStates& s = dusk::getSaveStates();
                    const auto& qs = s.getQuickSaves()[slot];
                    if (qs.isFullState) {
                        s.quickLoadFull(slot);
                    } else {
                        s.quickLoad(slot);
                    }
                }),
                rightPane, [](Pane& pane) {
                    pane.clear();
                    pane.add_text("Load this save state.");
                });

            leftPane.register_control(
                leftPane.add_button("Delete").on_pressed([slot]() {
                    mDoAud_seStartMenu(kSoundClick);
                    dusk::getSaveStates().deleteQuickSave(slot);
                }),
                rightPane, [](Pane& pane) {
                    pane.clear();
                    pane.add_text("Delete this save.");
                });
        }
    }
}

void SaveStatesWindow::build_named_states_tab(Rml::Element* content) {
    auto& leftPane = add_child<Pane>(content, Pane::Type::Controlled);
    auto& rightPane = add_child<Pane>(content, Pane::Type::Uncontrolled);

    dusk::SaveStates& states = dusk::getSaveStates();

    leftPane.add_section("Named States");
    rightPane.add_text("Save, load, and manage named save states. States persist across sessions.");

    const auto& namedStates = states.getNamedStates();
    bool gameRunning = dusk::IsGameLaunched && !dusk::getTransientSettings().stateShareLoadActive;

    for (size_t i = 0; i < namedStates.size(); ++i) {
        leftPane.add_rml("<br/>");

        int idx = i;
        const auto& state = namedStates[i];

        leftPane.register_control(
            leftPane.add_button(fmt::format("{}", state.name)).on_pressed([this, idx] {
                mDoAud_seStartMenu(kSoundItemChange);
            }),
            rightPane, [this, idx](Pane& pane) {
                pane.clear();
                pane.add_text("Select to view options.");
            });

        leftPane.register_control(
            leftPane.add_button("Load").on_pressed([idx]() {
                mDoAud_seStartMenu(kSoundClick);
                const auto& states = dusk::getSaveStates().getNamedStates();
                if (idx < (int)states.size()) {
                    dusk::getSaveStates().applyEncodedState(states[idx].encoded, states[idx].name);
                }
            }),
            rightPane, [](Pane& pane) {
                pane.clear();
                pane.add_text("Load this named state.");
            });

        leftPane.register_control(
            leftPane.add_button("Delete").on_pressed([idx]() {
                mDoAud_seStartMenu(kSoundClick);
                dusk::getSaveStates().deleteNamedState(idx);
            }),
            rightPane, [](Pane& pane) {
                pane.clear();
                pane.add_text("Delete this named state.");
            });
    }

    if (namedStates.empty()) {
        leftPane.add_rml("<br/><center><i>No named states. Create one below.</i></center>");
    }

    leftPane.add_rml("<br/>");
    leftPane.add_section("Actions");

    if (gameRunning) {
        leftPane.register_control(
            leftPane.add_button("Save Current State").on_pressed([] {
                mDoAud_seStartMenu(kSoundClick);
                dusk::SaveStates& ss = dusk::getSaveStates();
                const auto& states = ss.getNamedStates();
                int nextNum = 1;
                for (const auto& s : states) {
                    if (s.name.rfind("State ", 0) == 0) {
                        try {
                            int n = std::stoi(s.name.substr(7));
                            if (n >= nextNum) nextNum = n + 1;
                        } catch (...) {}
                    }
                }
                ss.saveNamedState(fmt::format("State {}", nextNum));
            }),
            rightPane, [](Pane& pane) {
                pane.clear();
                pane.add_text("Save the current game state with a new name.");
            });
    }

    leftPane.register_control(
        leftPane.add_button("Import from Clipboard").on_pressed([] {
            mDoAud_seStartMenu(kSoundClick);
            if (SDL_HasClipboardText()) {
                char* text = SDL_GetClipboardText();
                if (text && text[0] != '\0') {
                    dusk::SaveStates& ss = dusk::getSaveStates();
                    const auto& states = ss.getNamedStates();
                    int nextNum = 1;
                    for (const auto& s : states) {
                        if (s.name.rfind("State ", 0) == 0) {
                            try {
                                int n = std::stoi(s.name.substr(7));
                                if (n >= nextNum) nextNum = n + 1;
                            } catch (...) {}
                        }
                    }
                    ss.addNamedState(fmt::format("State {}", nextNum), text, false);
                }
                SDL_free(text);
            }
        }),
        rightPane, [](Pane& pane) {
            pane.clear();
            pane.add_text("Import a state from the clipboard.");
        });

    leftPane.register_control(
        leftPane.add_button("Load State Pack").on_pressed([] {
            mDoAud_seStartMenu(kSoundClick);
        }),
        rightPane, [](Pane& pane) {
            pane.clear();
            pane.add_text("Load a JSON file containing multiple states.");
        });

    if (!namedStates.empty()) {
        leftPane.register_control(
            leftPane.add_button("Clear All").on_pressed([] {
                mDoAud_seStartMenu(kSoundClick);
                dusk::getSaveStates().clearAllNamedStates();
            }),
            rightPane, [](Pane& pane) {
                pane.clear();
                pane.add_text("Delete all named states.");
            });
    }
}

} // namespace dusk::ui