#pragma once

#include "ItemChecklist.h"
#include "dusk/ui/event.hpp"
#include "dusk/ui/window.hpp"

#include <cstdint>
#include <memory>
#include <unordered_map>

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
        std::string iconSource;
        bool collected = false;
        bool stateInitialized = false;
    };

    void build(Rml::Element* content);
    void rebuildSections();
    void refreshItem(uint8_t itemId);

    CardRefs createCard(const ::ItemChecklist::ItemInfo& item, Rml::Element* parent);

    Rml::Element* mSectionsRoot = nullptr;
    std::unique_ptr<ScopedEventListener> mCloseListener;
    std::unordered_map<uint8_t, CardRefs> mCards;
};

}  // namespace dusk::ui
