#pragma once
#include "window.hpp"

namespace dusk::ui {

void reset_for_speedrun_mode();

class SettingsWindow : public Window {
public:
    SettingsWindow(bool prelaunch = false);

    void update() override;

protected:
    bool mPrelaunch;
};

}  // namespace dusk::ui
