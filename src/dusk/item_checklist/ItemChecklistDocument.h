#pragma once

#include "ItemChecklist.h"
#include "window.hpp"

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace dusk::ui {

class ItemChecklistDocument : public Window {
public:
    ItemChecklistDocument();

    void update() override;
    void refresh();

private:
    struct CardRefs {
        Rml::Element* root = nullptr;
        Rml::Element* icon = nullptr;
    };

    void build(Rml::Element* content);
    void rebuildSections();
    void refreshSummary();
    void refreshItem(uint8_t itemId);

    CardRefs createCard(const ::ItemChecklist::ItemInfo& item, Rml::Element* parent);

    Rml::Element* mStatusText = nullptr;
    Rml::Element* mSummaryText = nullptr;
    Rml::Element* mSummaryFill = nullptr;
    Rml::Element* mSectionsRoot = nullptr;
    std::unordered_map<uint8_t, CardRefs> mCards;
    std::vector<uint8_t> mCollectedSnapshot;
    bool mIconsReadySnapshot = false;
};

}  // namespace dusk::ui
