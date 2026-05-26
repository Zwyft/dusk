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
    add_tab("Practice Mode", [this](Rml::Element* content) {
        build_practice_mode_tab(content);
    });
    add_tab("Session Snapshots", [this](Rml::Element* content) {
        build_session_snapshots_tab(content);
    });
    set_active_tab(0);
}

void SaveStatesWindow::build_quick_saves_tab(Rml::Element* content) {
    auto& leftPane = add_child<Pane>(content, Pane::Type::Controlled);
    auto& rightPane = add_child<Pane>(content, Pane::Type::Uncontrolled);

    dusk::SaveStates& states = dusk::getSaveStates();

    leftPane.add_section("Quick Saves");
    rightPane.add_text("Each slot can store either Stage Reload (safe/default) or Full Snapshot (instant but less stable). Open a slot to see details before loading.");

    static constexpr int kSlots = 4;

    for (int i = 0; i < kSlots; ++i) {
        leftPane.add_rml("<br/>");

        int slot = i;
        auto& slotInfo = states.getQuickSaves()[slot];
        Rml::String info;
        if (slotInfo.occupied) {
            info = fmt::format("{} ({})",
                states.quickSaveInfo(slot),
                slotInfo.isFullState ? "Full Snapshot" : "Stage Reload");
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
                    pane.add_text("Empty slot. Use Stage Save or Full Snapshot to create one.");
                    return;
                }
                pane.clear();
                const auto& save = saves[slot];
                pane.add_rml(fmt::format("<b>Slot:</b> {}<br/><b>Stage:</b> {}<br/><b>Room:</b> {}<br/><b>Type:</b> {}<br/><b>Saved:</b> {}",
                    slot + 1,
                    save.stageName, (int)save.roomNo,
                    save.isFullState ? "Full Snapshot (instant load)" : "Stage Reload (safe/default)",
                    formatTimeAgo(save.timestamp)));
            });

        bool gameRunning = dusk::IsGameLaunched && !dusk::getTransientSettings().stateShareLoadActive;

        if (gameRunning) {
            leftPane.register_control(
                leftPane.add_button("Save (Stage)").on_pressed([slot]() {
                    mDoAud_seStartMenu(kSoundClick);
                    dusk::getSaveStates().quickSave(slot);
                }),
                rightPane, [](Pane& pane) {
                    pane.clear();
                    pane.add_text("Save current game state using stage reload method (recommended).\nMore stable across rooms and script events.");
                });

            leftPane.register_control(
                leftPane.add_button("Save (Full Snapshot)").on_pressed([slot]() {
                    mDoAud_seStartMenu(kSoundClick);
                    dusk::getSaveStates().quickSaveFull(slot);
                }),
                rightPane, [](Pane& pane) {
                    pane.clear();
                    pane.add_text("Capture full actor snapshot for instant load.\nUseful for practice, but may be less stable in complex scenes.");
                });
        }

        if (states.hasQuickSave(slot)) {
            leftPane.register_control(
                leftPane.add_button("Load").on_pressed([slot]() {
                    mDoAud_seStartMenu(kSoundClick);
                    dusk::SaveStates& s = dusk::getSaveStates();
                    s.quickLoad(slot);
                }),
                rightPane, [](Pane& pane) {
                    pane.clear();
                    pane.add_text("Load this slot. Dusk will choose the correct load mode automatically.");
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
    rightPane.add_text("Save, load, and manage named save states. Tip: use descriptive names for routes, boss setups, or practice checkpoints.");

    const auto& namedStates = states.getNamedStates();
    bool gameRunning = dusk::IsGameLaunched && !dusk::getTransientSettings().stateShareLoadActive;

    for (size_t i = 0; i < namedStates.size(); ++i) {
        leftPane.add_rml("<br/>");

        int idx = i;
        const auto& state = namedStates[i];

        leftPane.register_control(
            leftPane.add_button(fmt::format("{}: {}", idx + 1, state.name)).on_pressed([this, idx] {
                mDoAud_seStartMenu(kSoundItemChange);
            }),
            rightPane, [idx](Pane& pane) {
                const auto& states = dusk::getSaveStates().getNamedStates();
                if (idx < 0 || idx >= (int)states.size()) {
                    pane.clear();
                    pane.add_text("State not found.");
                    return;
                }

                const auto& state = states[idx];
                pane.clear();
                pane.add_rml(fmt::format("<b>Name:</b> {}<br/><b>Type:</b> {}",
                    state.name,
                    state.isFullState ? "Full Snapshot" : "Stage Reload"));
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

void SaveStatesWindow::build_practice_mode_tab(Rml::Element* content) {
    auto& leftPane = add_child<Pane>(content, Pane::Type::Controlled);
    auto& rightPane = add_child<Pane>(content, Pane::Type::Uncontrolled);

    leftPane.add_section("Practice Scenario Launcher");
    rightPane.add_text("Create premade place snapshots from your current inventory/progression. These act like route-practice checkpoints.");

    struct ScenarioPreset {
        const char* name;
        const char* stage;
        int8_t room;
        int8_t layer;
        int16_t spawn;
        const char* description;
    };

    static constexpr ScenarioPreset kPresets[] = {
        {"Practice: Ordon Ranch", "F_SP103", 0, 0, -1, "Early movement and wolf routing drills."},
        {"Practice: Kakariko Village", "R_SP109", 0, 0, -1, "Mid-game movement, climb, and menuing routes."},
        {"Practice: Forest Temple Entrance", "D_MN05", 0, 0, -1, "Dungeon opener setups."},
        {"Practice: Goron Mines Entrance", "D_MN04", 0, 0, -1, "Bomb bag / mine route practice."},
        {"Practice: Lakebed Temple Entrance", "D_MN07", 0, 0, -1, "Water movement and routing practice."},
    };

    bool gameRunning = dusk::IsGameLaunched && !dusk::getTransientSettings().stateShareLoadActive;
    if (!gameRunning) {
        leftPane.add_text("Launch game to create practice snapshots.");
        return;
    }

    for (const auto& preset : kPresets) {
        leftPane.register_control(
            leftPane.add_button(fmt::format("Create {}", preset.name)).on_pressed([preset]() {
                mDoAud_seStartMenu(kSoundClick);
                dusk::getSaveStates().createPracticePresetState(
                    preset.name,
                    preset.stage,
                    preset.room,
                    preset.layer,
                    preset.spawn);
            }),
            rightPane, [preset](Pane& pane) {
                pane.clear();
                pane.add_rml(fmt::format("<b>{}</b><br/>{}<br/>Stage: {} / Room {} / Layer {}",
                    preset.name,
                    preset.description,
                    preset.stage,
                    (int)preset.room,
                    (int)preset.layer));
            });
    }

    leftPane.add_rml("<br/>");
    leftPane.add_section("Scenario Snapshot Helpers");

    leftPane.register_control(
        leftPane.add_button("Save Full Practice Snapshot").on_pressed([] {
            mDoAud_seStartMenu(kSoundClick);
            auto now = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            dusk::getSaveStates().saveNamedStateFull(fmt::format("Practice Full {}", now));
        }),
        rightPane, [](Pane& pane) {
            pane.clear();
            pane.add_text("Capture a full snapshot for instant retry practice.");
        });
}

void SaveStatesWindow::build_session_snapshots_tab(Rml::Element* content) {
    auto& leftPane = add_child<Pane>(content, Pane::Type::Controlled);
    auto& rightPane = add_child<Pane>(content, Pane::Type::Uncontrolled);

    dusk::SaveStates& states = dusk::getSaveStates();
    leftPane.add_section("Session Snapshots + Rollback");
    rightPane.add_text("Timeline-like snapshots for this session. Capture, load any point, or rollback to latest.");

    bool gameRunning = dusk::IsGameLaunched && !dusk::getTransientSettings().stateShareLoadActive;
    if (gameRunning) {
        leftPane.register_control(
            leftPane.add_button("Capture Snapshot (Stage Reload)").on_pressed([] {
                mDoAud_seStartMenu(kSoundClick);
                dusk::getSaveStates().captureSessionSnapshot({}, false);
            }),
            rightPane, [](Pane& pane) {
                pane.clear();
                pane.add_text("Capture a lightweight rollback point.");
            });

        leftPane.register_control(
            leftPane.add_button("Capture Snapshot (Full)").on_pressed([] {
                mDoAud_seStartMenu(kSoundClick);
                dusk::getSaveStates().captureSessionSnapshot({}, true);
            }),
            rightPane, [](Pane& pane) {
                pane.clear();
                pane.add_text("Capture a full actor snapshot (more exact, potentially less stable). ");
            });

        leftPane.register_control(
            leftPane.add_button("Rollback to Latest Snapshot").on_pressed([] {
                mDoAud_seStartMenu(kSoundClick);
                dusk::getSaveStates().rollbackLastSessionSnapshot();
            }),
            rightPane, [](Pane& pane) {
                pane.clear();
                pane.add_text("Load the newest session snapshot.");
            });
    }

    const auto& snaps = states.getSessionSnapshots();
    if (snaps.empty()) {
        leftPane.add_rml("<br/><center><i>No session snapshots yet.</i></center>");
    } else {
        for (int i = (int)snaps.size() - 1; i >= 0; --i) {
            const auto& snap = snaps[i];
            const auto label = fmt::format("{}{}: {}{}",
                snap.favorite ? "★ " : "",
                i + 1,
                snap.name,
                snap.tag.empty() ? "" : fmt::format(" [{}]", snap.tag));
            leftPane.register_control(
                leftPane.add_button(label).on_pressed([i]() {
                    mDoAud_seStartMenu(kSoundClick);
                    dusk::getSaveStates().loadSessionSnapshot(i);
                }),
                rightPane, [i](Pane& pane) {
                    auto& stateMgr = dusk::getSaveStates();
                    const auto& local = stateMgr.getSessionSnapshots();
                    if (i < 0 || i >= (int)local.size()) {
                        pane.clear();
                        pane.add_text("Snapshot missing.");
                        return;
                    }
                    const auto& s = local[i];
                    pane.clear();
                    pane.add_rml(fmt::format("<b>{}</b><br/>Type: {}<br/>Saved: {}<br/>Favorite: {}<br/>Tag: {}",
                        s.name,
                        s.isFullState ? "Full" : "Stage Reload",
                        formatTimeAgo(s.timestamp),
                        s.favorite ? "Yes" : "No",
                        s.tag.empty() ? "(none)" : s.tag));

                    pane.add_rml("<br/>");
                    pane.add_section("Snapshot Manager");
                    pane.add_button("Rename (Auto)").on_pressed([i]() {
                        mDoAud_seStartMenu(kSoundClick);
                        auto now = std::chrono::duration_cast<std::chrono::seconds>(
                            std::chrono::system_clock::now().time_since_epoch()).count();
                        dusk::getSaveStates().renameSessionSnapshot(i, fmt::format("Snapshot {}", now));
                    });
                    pane.add_button("Toggle Favorite").on_pressed([i]() {
                        mDoAud_seStartMenu(kSoundClick);
                        dusk::getSaveStates().toggleFavoriteSessionSnapshot(i);
                    });
                    pane.add_button("Tag: Boss").on_pressed([i]() {
                        mDoAud_seStartMenu(kSoundClick);
                        dusk::getSaveStates().setSessionSnapshotTag(i, "Boss");
                    });
                    pane.add_button("Tag: Movement").on_pressed([i]() {
                        mDoAud_seStartMenu(kSoundClick);
                        dusk::getSaveStates().setSessionSnapshotTag(i, "Movement");
                    });
                    pane.add_button("Clear Tag").on_pressed([i]() {
                        mDoAud_seStartMenu(kSoundClick);
                        dusk::getSaveStates().setSessionSnapshotTag(i, "");
                    });
                    pane.add_button("Move Up").on_pressed([i]() {
                        mDoAud_seStartMenu(kSoundClick);
                        dusk::getSaveStates().moveSessionSnapshot(i, i > 0 ? i - 1 : i);
                    });
                    pane.add_button("Move Down").on_pressed([i]() {
                        mDoAud_seStartMenu(kSoundClick);
                        auto& mgr = dusk::getSaveStates();
                        const int last = (int)mgr.getSessionSnapshots().size() - 1;
                        mgr.moveSessionSnapshot(i, i < last ? i + 1 : i);
                    });
                    pane.add_button("Delete Snapshot").on_pressed([i]() {
                        mDoAud_seStartMenu(kSoundClick);
                        dusk::getSaveStates().deleteSessionSnapshot(i);
                    });
                });
        }

        leftPane.register_control(
            leftPane.add_button("Clear Session Snapshots").on_pressed([] {
                mDoAud_seStartMenu(kSoundClick);
                dusk::getSaveStates().clearSessionSnapshots();
            }),
            rightPane, [](Pane& pane) {
                pane.clear();
                pane.add_text("Delete current session timeline snapshots.");
            });
    }
}

} // namespace dusk::ui