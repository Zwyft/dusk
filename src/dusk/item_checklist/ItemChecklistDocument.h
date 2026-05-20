#pragma once

#include "ItemChecklist.h"
#include "dusk/ui/event.hpp"
#include "dusk/ui/window.hpp"

#include <cstdint>
#include <memory>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace dusk::ui {

class ItemChecklistDocument : public Window {
public:
    ItemChecklistDocument();

    void update() override;
    void refresh();

private:
    static constexpr std::string_view kSpeedrunTab = "Speedrun";

    struct LayoutConfig {
        int columns = 4;
        const char* densityClass = "dense-normal";
    };

    struct CardRefs {
        Rml::Element* root = nullptr;
        Rml::Element* icon = nullptr;
        std::string iconSource;
        bool collected = false;
        bool stateInitialized = false;
    };

    void build(Rml::Element* content, const std::string& tab);
    void rebuildSections(const std::string& tab);
    void refreshItem(uint8_t itemId);
    LayoutConfig chooseLayout(const std::string& tab, size_t itemCount) const;

    CardRefs createCard(const ::ItemChecklist::ItemInfo& item, Rml::Element* parent);

    Rml::Element* mSectionsRoot = nullptr;
    std::vector<std::unique_ptr<ScopedEventListener>> mCardListeners;
    std::unordered_map<uint8_t, CardRefs> mCards;
    uint64_t mSeenRevision = 0;
};

}  // namespace dusk::ui
