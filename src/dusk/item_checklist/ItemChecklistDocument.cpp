#include "ItemChecklistDocument.h"

#include "ItemChecklist.h"

#include "dusk/ui/event.hpp"
#include "dusk/ui/ui.hpp"

#include <RmlUi/Core.h>

#include "fmt/format.h"

namespace dusk::ui {
namespace {

const Rml::String kChecklistContent = R"RML(
<div id="tracker-root" class="tracker-window">
    <div class="tracker-shell">
        <div class="tracker-header">
            <div class="tracker-titleblock">
                <div class="tracker-kicker">Item tracker</div>
                <h1>Emotracker-style checklist</h1>
            </div>
            <button id="tracker-close" class="tracker-close">
                <icon class="material-symbols-rounded">close</icon>
            </button>
        </div>

        <div id="tracker-sections" class="tracker-sections"></div>
    </div>
</div>
)RML";

}  // namespace

ItemChecklistDocument::ItemChecklistDocument() {
    mRoot->SetClass("checklist-window", true);
    add_tab("Checklist", [this](Rml::Element* content) { build(content); });
}

void ItemChecklistDocument::build(Rml::Element* content) {
    if (content == nullptr) {
        return;
    }

    content->SetInnerRML(kChecklistContent);
    mSectionsRoot = content->GetElementById("tracker-sections");

    mCloseListener.reset();
    if (auto* closeButton = content->GetElementById("tracker-close"); closeButton != nullptr) {
        mCloseListener = std::make_unique<ScopedEventListener>(
            closeButton, Rml::EventId::Click, [this](Rml::Event&) { request_close(); });
    }

    rebuildSections();
    refresh();
}

void ItemChecklistDocument::rebuildSections() {
    if (mSectionsRoot == nullptr) {
        return;
    }

    while (mSectionsRoot->GetNumChildren() != 0) {
        mSectionsRoot->RemoveChild(mSectionsRoot->GetFirstChild());
    }
    mCards.clear();

    auto* grid = append(mSectionsRoot, "div");
    if (grid == nullptr) {
        return;
    }
    grid->SetClass("tracker-grid", true);

    for (const auto& item : ItemChecklist::instance().items()) {
        mCards[item.id] = createCard(item, grid);
    }
}

ItemChecklistDocument::CardRefs ItemChecklistDocument::createCard(
    const ::ItemChecklist::ItemInfo& item, Rml::Element* parent) {
    CardRefs refs;
    if (parent == nullptr) {
        return refs;
    }

    auto* button = append(parent, "button");
    if (button == nullptr) {
        return refs;
    }
    button->SetClass("tracker-card", true);
    button->SetAttribute("type", "button");
    button->SetAttribute("data-item-id", std::to_string(item.id));
    button->SetAttribute("title", item.name);

    const auto iconPath = ItemChecklist::instance().iconPathFor(item.id);
    if (!iconPath.empty()) {
        auto* icon = append(button, "img");
        if (icon == nullptr) {
            return refs;
        }
        icon->SetClass("tracker-card-icon", true);
        icon->SetAttribute("src", iconPath);
        refs.icon = icon;
        refs.iconSource = iconPath;
    }

    auto* label = append(button, "div");
    if (label == nullptr) {
        return refs;
    }
    label->SetClass("tracker-card-label", true);
    label->SetInnerRML(escape(item.name));

    refs.root = button;
    return refs;
}

void ItemChecklistDocument::refreshItem(uint8_t itemId) {
    const auto* item = ItemChecklist::instance().getItemInfo(itemId);
    auto it = mCards.find(itemId);
    if (item == nullptr || it == mCards.end() || it->second.root == nullptr) {
        return;
    }

    const bool collected = ItemChecklist::instance().isCollected(itemId);
    auto* root = it->second.root;
    if (!it->second.stateInitialized || it->second.collected != collected) {
        it->second.collected = collected;
        it->second.stateInitialized = true;
        root->SetClass("owned", collected);
        root->SetClass("locked", !collected);
        root->SetAttribute(
            "title", fmt::format("{} - {}", item->name, collected ? "Unlocked" : "Locked"));
    }

    if (it->second.icon != nullptr) {
        const auto iconPath = ItemChecklist::instance().iconPathFor(itemId);
        if (iconPath.empty()) {
            it->second.icon->RemoveAttribute("src");
            it->second.iconSource.clear();
        } else if (iconPath != it->second.iconSource) {
            it->second.icon->SetAttribute("src", iconPath);
            it->second.iconSource = iconPath;
        }
    }
}

void ItemChecklistDocument::refresh() {
    for (const auto& item : ItemChecklist::instance().items()) {
        refreshItem(item.id);
    }
}

void ItemChecklistDocument::update() {
    Window::update();
}

}  // namespace dusk::ui
