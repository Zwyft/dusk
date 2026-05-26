#pragma once

#include "window.hpp"

#include <string>

namespace dusk::ui {

class MediaImportWindow : public Window {
public:
    explicit MediaImportWindow(bool allowRemote);

private:
    void build_tab(Rml::Element* content);
    bool mAllowRemote = false;
    std::string mManualUrl;
};

} // namespace dusk::ui
