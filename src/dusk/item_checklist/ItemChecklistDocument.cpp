#include "ItemChecklistDocument.h"

#include "ItemChecklist.h"

#include "dusk/ui/event.hpp"
#include "dusk/ui/ui.hpp"

#include <RmlUi/Core.h>
#include <SDL3/SDL_timer.h>

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
                <div id="tracker-status" class="tracker-status">Waiting for disc assets</div>
            </div>
            <button id="tracker-close" class="tracker-close">
                <icon class="material-symbols-rounded">close</icon>
            </button>
        </div>

        <div class="tracker-summary-row">
            <div class="tracker-summary-meta">
                <span id="tracker-summary">0 / 0 collected</span>
            </div>
            <div class="tracker-progress">
                <div id="tracker-summary-fill" class="tracker-progress-fill"></div>
            </div>
        </div>

        <div id="tracker-sections" class="tracker-sections"></div>
    </div>
</div>
)RML";

}  // namespace

ItemChecklistDocument::ItemChecklistDocument() {
    add_tab("Checklist", [this](Rml::Element* content) { build(content); });
}

void ItemChecklistDocument::build(Rml::Element* content) {
    if (content == nullptr) {
        return;
    }

    content->SetInnerRML(kChecklistContent);
    mStatusText = content->GetElementById("tracker-status");
    mSummaryText = content->GetElementById("tracker-summary");
    mSummaryFill = content->GetElementById("tracker-summary-fill");
    mSectionsRoot = content->GetElementById("tracker-sections");

    mCloseListener.reset();
    if (auto* closeButton = content->GetElementById("tracker-close"); closeButton != nullptr) {
        mCloseListener = std::make_unique<ScopedEventListener>(
            closeButton, Rml::EventId::Click, [this](Rml::Event&) { request_close(); });
    }

    mLastRefreshTick = 0;
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

    const auto categories = ItemChecklist::instance().categories();
    for (const auto& category : categories) {
        auto* section = append(mSectionsRoot, "div");
        section->SetClass("tracker-section", true);

        auto* heading = append(section, "div");
        heading->SetClass("tracker-section-title", true);
        heading->SetInnerRML(escape(category));

        auto* grid = append(section, "div");
        grid->SetClass("tracker-grid", true);

        for (const auto* item : ItemChecklist::instance().getItemsByCategory(category)) {
            mCards[item->id] = createCard(*item, grid);
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

    auto* icon = append(button, "img");
    if (icon == nullptr) {
        return refs;
    }
    icon->SetClass("tracker-card-icon", true);
    const auto iconPath = ItemChecklist::instance().iconPathFor(item.id);
    if (!iconPath.empty()) {
        icon->SetAttribute("src", iconPath);
    }

    auto* label = append(button, "div");
    if (label == nullptr) {
        return refs;
    }
    label->SetClass("tracker-card-label", true);
    label->SetInnerRML(escape(item.name));

    refs.root = button;
    refs.icon = icon;
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
    root->SetClass("owned", collected);
    root->SetClass("locked", !collected);
    root->SetAttribute(
        "title", fmt::format("{} - {}", item->name, collected ? "Unlocked" : "Locked"));

    if (it->second.icon != nullptr) {
        const auto iconPath = ItemChecklist::instance().iconPathFor(itemId);
        if (iconPath.empty()) {
            it->second.icon->RemoveAttribute("src");
        } else {
            it->second.icon->SetAttribute("src", iconPath);
        }
    }
}

void ItemChecklistDocument::refreshSummary() {
    const auto& items = ItemChecklist::instance().items();
    const auto total = static_cast<int>(items.size());
    int collected = 0;
    for (const auto& item : items) {
        if (ItemChecklist::instance().isCollected(item.id)) {
            ++collected;
        }
    }

    if (mSummaryText != nullptr) {
        mSummaryText->SetInnerRML(
            fmt::format("{} / {} collected", collected, total));
    }
    if (mSummaryFill != nullptr) {
        const float fraction = total > 0 ? static_cast<float>(collected) / total : 0.0f;
        mSummaryFill->SetAttribute("style", fmt::format("width: {:.2f}%;", fraction * 100.0f));
    }
    if (mStatusText != nullptr) {
        mStatusText->SetInnerRML(ItemChecklist::instance().iconsReady()
                ? "Icons extracted from the loaded disc"
                : "Waiting for disc assets");
    }
}

void ItemChecklistDocument::refresh() {
    for (const auto& item : ItemChecklist::instance().items()) {
        refreshItem(item.id);
    }
    refreshSummary();
}

void ItemChecklistDocument::update() {
    if (visible()) {
        constexpr Uint64 kRefreshIntervalNs = 250'000'000ULL;
        const Uint64 now = SDL_GetTicksNS();
        if (mLastRefreshTick == 0 || now - mLastRefreshTick >= kRefreshIntervalNs) {
            mLastRefreshTick = now;
            ItemChecklist::instance().refresh();

            const auto& items = ItemChecklist::instance().items();
            const bool iconsReady = ItemChecklist::instance().iconsReady();
            bool dirty = iconsReady != mIconsReadySnapshot ||
                         items.size() != mCollectedSnapshot.size();
            if (!dirty) {
                for (size_t i = 0; i < items.size(); ++i) {
                    const bool collected = ItemChecklist::instance().isCollected(items[i].id);
                    if (mCollectedSnapshot[i] != static_cast<uint8_t>(collected)) {
                        dirty = true;
                        break;
                    }
                }
            }

            if (dirty) {
                mIconsReadySnapshot = iconsReady;
                mCollectedSnapshot.clear();
                mCollectedSnapshot.reserve(items.size());
                for (const auto& item : items) {
                    mCollectedSnapshot.push_back(static_cast<uint8_t>(
                        ItemChecklist::instance().isCollected(item.id)));
                }
                refresh();
            }
        }
    }

    Window::update();
}

}  // namespace dusk::ui
