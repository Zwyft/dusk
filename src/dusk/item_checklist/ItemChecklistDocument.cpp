#include "ItemChecklistDocument.h"

#include "ItemChecklist.h"

#include "dusk/ui/event.hpp"
#include "dusk/ui/ui.hpp"
#include "dusk/settings.h"

#include <RmlUi/Core.h>

#include "fmt/format.h"

namespace dusk::ui {
namespace {

const Rml::String kChecklistContent = R"RML(
<div id="tracker-root" class="tracker-window">
    <div id="tracker-sections" class="tracker-sections"></div>
</div>
)RML";

}  // namespace

ItemChecklistDocument::ItemChecklistDocument() {
    auto& checklist = ItemChecklist::instance();
    if (!checklist.isInitialized()) {
        checklist.initialize();
    }

    const auto trackerTabs = checklist.tabs();
    for (const auto& tab : trackerTabs) {
        add_tab(tab, [this, tab](Rml::Element* content) { build(content, tab); });
    }
    add_tab(kSpeedrunTab.data(), [this](Rml::Element* content) {
        build(content, std::string{kSpeedrunTab});
    });
}

void ItemChecklistDocument::build(Rml::Element* content, const std::string& tab) {
    if (content == nullptr) {
        return;
    }

    content->SetInnerRML(kChecklistContent);
    mSectionsRoot = content->GetElementById("tracker-sections");
    rebuildSections(tab);
    refresh();
    mSeenRevision = ItemChecklist::instance().revision();
}

ItemChecklistDocument::LayoutConfig ItemChecklistDocument::chooseLayout(
    const std::string& tab, size_t itemCount) const {
    const int densityMode = getSettings().backend.checklistDensityMode.getValue();
    if (densityMode == 1) {
        if (tab == kSpeedrunTab) {
            return {.columns = 10, .densityClass = "dense-normal"};
        }
        if (itemCount <= 16) {
            return {.columns = 4, .densityClass = "dense-normal"};
        }
        if (itemCount <= 28) {
            return {.columns = 5, .densityClass = "dense-normal"};
        }
        return {.columns = 6, .densityClass = "dense-medium"};
    }
    if (densityMode == 2) {
        if (tab == kSpeedrunTab) {
            return {.columns = 14, .densityClass = "dense-compact"};
        }
        if (itemCount <= 16) {
            return {.columns = 6, .densityClass = "dense-medium"};
        }
        if (itemCount <= 30) {
            return {.columns = 7, .densityClass = "dense-medium"};
        }
        return {.columns = 8, .densityClass = "dense-compact"};
    }

    if (tab == kSpeedrunTab) {
        return {.columns = 12, .densityClass = "dense-medium"};
    }
    if (itemCount <= 12) {
        return {.columns = 4, .densityClass = "dense-normal"};
    }
    if (itemCount <= 20) {
        return {.columns = 5, .densityClass = "dense-normal"};
    }
    if (itemCount <= 30) {
        return {.columns = 6, .densityClass = "dense-medium"};
    }
    if (itemCount <= 42) {
        return {.columns = 7, .densityClass = "dense-medium"};
    }
    return {.columns = 8, .densityClass = "dense-compact"};
}

void ItemChecklistDocument::rebuildSections(const std::string& tab) {
    if (mSectionsRoot == nullptr) {
        return;
    }

    while (mSectionsRoot->GetNumChildren() != 0) {
        mSectionsRoot->RemoveChild(mSectionsRoot->GetFirstChild());
    }
    mCards.clear();
    mCardListeners.clear();

    auto* grid = append(mSectionsRoot, "div");
    if (grid == nullptr) {
        return;
    }
    grid->SetClass("tracker-grid", true);
    const bool isSpeedrunTab = tab == kSpeedrunTab;
    if (isSpeedrunTab) {
        grid->SetClass("speedrun-grid", true);
    }

    const auto items = ItemChecklist::instance().getItemsByTab(tab);
    const auto layout = chooseLayout(tab, items.size());
    grid->SetClass(layout.densityClass, true);
    grid->SetClass(fmt::format("cols-{}", layout.columns), true);

    Rml::Element* row = nullptr;
    int index = 0;
    for (const auto* item : items) {
        if (item == nullptr) {
            continue;
        }
        if (index % layout.columns == 0) {
            row = append(grid, "div");
            if (row == nullptr) {
                return;
            }
            row->SetClass("tracker-row", true);
        }
        mCards[item->id] = createCard(*item, row);
        ++index;
    }

    if (isSpeedrunTab) {
        const int targetCells = layout.columns * 7;
        while (index < targetCells) {
            if (index % layout.columns == 0) {
                row = append(grid, "div");
                if (row == nullptr) {
                    return;
                }
                row->SetClass("tracker-row", true);
            }
            auto* placeholder = append(row, "div");
            if (placeholder == nullptr) {
                return;
            }
            placeholder->SetClass("tracker-card", true);
            placeholder->SetClass("tracker-card-placeholder", true);
            ++index;
        }
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
    if (!item.useLiveState) {
        mCardListeners.emplace_back(std::make_unique<ScopedEventListener>(
            button, Rml::EventId::Click, [itemId = item.id, this](Rml::Event&) {
                ItemChecklist::instance().toggleCollected(itemId);
                refreshItem(itemId);
                mSeenRevision = ItemChecklist::instance().revision();
            }));
    }

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
    const auto revision = ItemChecklist::instance().revision();
    if (revision != mSeenRevision) {
        refresh();
        mSeenRevision = revision;
    }
}

}  // namespace dusk::ui
