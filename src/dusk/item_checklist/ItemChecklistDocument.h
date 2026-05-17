#pragma once

#include "dusk/ui/document.hpp"

#include <cstdint>
#include <unordered_map>

class ItemChecklist;

namespace dusk::ui {

class ItemChecklistDocument : public Document {
public:
    ItemChecklistDocument();

    void update() override;
    void refresh();

private:
    struct CardRefs {
        Rml::Element* root = nullptr;
        Rml::Element* icon = nullptr;
    };

    void build();
    void rebuildSections();
    void refreshSummary();
    void refreshItem(uint8_t itemId);

    CardRefs createCard(const ItemChecklist::ItemInfo& item, Rml::Element* parent);

    Rml::Element* mStatusText = nullptr;
    Rml::Element* mSummaryText = nullptr;
    Rml::Element* mSummaryFill = nullptr;
    Rml::Element* mSectionsRoot = nullptr;
    std::unordered_map<uint8_t, CardRefs> mCards;
};

}  // namespace dusk::ui
