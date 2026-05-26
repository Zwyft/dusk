#pragma once

#include "window.hpp"

namespace dusk::ui {

class MediaImportWindow : public Window {
public:
    explicit MediaImportWindow(bool allowRemote);

private:
    void build_tab(Rml::Element* content);
    bool mAllowRemote = false;
};

} // namespace dusk::ui
