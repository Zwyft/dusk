#pragma once

#include "dusk/ui/document.hpp"

namespace dusk::ui {

class ModsWindow : public Document {
public:
    ModsWindow();
    void show() override;
    void update() override;
    bool handle_nav_command(Rml::Event& event, NavCommand cmd) override;

private:
    void refresh_ui();
    void build_mod_list(Rml::Element* parent);

    Rml::Element* mRoot = nullptr;
    Rml::Element* mModList = nullptr;
    Rml::Element* mModCount = nullptr;
    bool mNeedsRefresh = true;
    int mFocusedIndex = 0;
};

} // namespace dusk::ui
