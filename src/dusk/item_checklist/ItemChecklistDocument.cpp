#include "ItemChecklistDocument.h"

#include "ItemChecklist.h"
#include "dusk/ui/button.hpp"
#include "dusk/ui/component.hpp"
#include "dusk/ui/pane.hpp"
#include "dusk/ui/window.hpp"
#include "fmt/format.h"

#include <RmlUi/Core.h>

namespace dusk::ui {

ItemChecklistDocument::ItemChecklistDocument() : Document("item_checklist") {}

void ItemChecklistDocument::Initialize(const Rml::String& source) {
    Document::Initialize(source);
    
    // Get references to UI elements
    auto* content = GetElementById("item-grid");
    if (content) {
        createItemGrid();
    }
    
    mProgressText = GetElementById("progress-text");
    mProgressBar = GetElementById("progress-bar");
    mProgressFill = GetElementById("progress-fill");
    
    // Initial refresh
    refresh();
}

void ItemChecklistDocument::createItemGrid() {
    auto* grid = GetElementById("item-grid");
    if (!grid) return;
    
    const auto& allItems = ItemChecklist::instance().getItemsByCategory("All");
    mTotalItems = static_cast<int>(allItems.size());
    
    for (const auto* item : allItems) {
        auto* elem = createItemElement(item->id);
        if (elem) {
            grid->AppendChild(elem);
            mItemElements[item->id] = elem;
        }
    }
}

Rml::Element* ItemChecklistDocument::createItemElement(uint8_t itemId) {
    const auto* item = ItemChecklist::instance().getItemInfo(itemId);
    if (!item) return nullptr;
    
    auto* doc = GetOwnerDocument();
    auto* div = doc->CreateElement("div");
    div->SetAttribute("class", "item");
    div->SetAttribute("data-item-id", std::to_string(itemId));
    
    // Icon
    auto* icon = doc->CreateElement("icon");
    icon->SetAttribute("class", "item-icon");
    icon->SetAttribute("style", fmt::format("background-image: url('{}')", item->iconPath));
    div->AppendChild(std::move(icon));
    
    // Name
    auto* name = doc->CreateElement("div");
    name->SetAttribute("class", "item-name");
    name->SetInnerRML(item->name);
    div->AppendChild(std::move(name));
    
    // Category label (small)
    auto* cat = doc->CreateElement("div");
    cat->SetAttribute("class", "item-category");
    cat->SetInnerRML(item->category);
    div->AppendChild(std::move(cat));
    
    // Click handler to toggle collection state (for testing)
    div->SetPointerButtonCallback([this, itemId](Rml::ElementPointerEvent& event) {
        if (event.button == 0 && event.type == Rml::PointerEventType::Click) {
            ItemChecklist::instance().toggleCollected(itemId);
            event.StopPropagation();
        }
    });
    
    return div;
}

void ItemChecklistDocument::refresh() {
    // Update all item visuals based on current collection state
    for (const auto& [itemId, elem] : mItemElements) {
        updateItemVisuals(itemId);
    }
    updateProgress();
}

void ItemChecklistDocument::setCollected(uint8_t itemId, bool collected) {
    updateItemVisuals(itemId);
    updateProgress();
}

void ItemChecklistDocument::updateItemVisuals(uint8_t itemId) {
    auto it = mItemElements.find(itemId);
    if (it == mItemElements.end() || !it->second) return;
    
    bool collected = ItemChecklist::instance().isCollected(itemId);
    
    if (collected) {
        it->second->SetClass("collected", true);
        it->second->SetClass("uncollected", false);
    } else {
        it->second->SetClass("uncollected", true);
        it->second->SetClass("collected", false);
    }
}

void ItemChecklistDocument::updateProgress() {
    if (!mProgressText || !mProgressBar || !mProgressFill) return;
    
    mCollectedItems = 0;
    mTotalItems = 0;
    
    // Count all items
    const auto& allItems = ItemChecklist::instance().getItemsByCategory("All");
    mTotalItems = static_cast<int>(allItems.size());
    
    // Count collected
    for (const auto* item : allItems) {
        if (ItemChecklist::instance().isCollected(item->id)) {
            mCollectedItems++;
        }
    }
    
    // Update text
    mProgressText->SetInnerRML(fmt::format("{} / {} ({}%)", 
        mCollectedItems, mTotalItems, 
        mTotalItems > 0 ? (mCollectedItems * 100) / mTotalItems : 0));
    
    // Update progress bar
    if (mTotalItems > 0) {
        float fraction = static_cast<float>(mCollectedItems) / mTotalItems;
        mProgressFill->SetAttribute("style", fmt::format("width: {}%", fraction * 100.0f));
    } else {
        mProgressFill->SetAttribute("style", "width: 0%");
    }
}

void ItemChecklistDocument::update() {
    Document::update();
    // Poll game state every frame
    ItemChecklist::instance().refresh();
}

} // namespace dusk::ui