#pragma once

#include "window.hpp"

namespace dusk::ui {

class SaveStatesWindow : public Window {
public:
    SaveStatesWindow();

private:
    void build_quick_saves_tab(Rml::Element* content);
    void build_named_states_tab(Rml::Element* content);
    void build_practice_mode_tab(Rml::Element* content);
    void build_session_snapshots_tab(Rml::Element* content);
    void refresh_quick_saves();
    void refresh_named_states();
    void delete_named_state(int index);
    void clear_all_named_states();

    void on_quick_save(int slot);
    void on_quick_save_full(int slot);
    void on_quick_load(int slot);
    void on_delete_quick_save(int slot);

    void on_load_named_state(int index);
    void on_copy_named_state(int index);
    void on_delete_named_state(int index);

    Rml::Element* mQuickSavesList = nullptr;
    Rml::Element* mNamedStatesList = nullptr;
};

} // namespace dusk::ui