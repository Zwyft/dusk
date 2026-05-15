#pragma once

#include "window.hpp"

namespace dusk::ui {

class Pane;

class ModsWindow : public Window {
public:
    ModsWindow();
    void update() override;

private:
    void refresh_mod_list(Pane& leftPane, Pane& rightPane);
    bool mNeedsRefresh = true;
};

}  // namespace dusk::ui
