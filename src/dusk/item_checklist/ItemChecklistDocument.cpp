#include "ItemChecklistDocument.h"

#include "ItemChecklist.h"

#include "dusk/ui/event.hpp"
#include "dusk/ui/ui.hpp"

#include <RmlUi/Core.h>

#include "fmt/format.h"

namespace dusk::ui {
namespace {

const Rml::String kChecklistStyles = R"RCSS(
.tracker-window {
    position: absolute;
    top: 24dp;
    right: 24dp;
    bottom: 24dp;
    left: 24dp;
    display: flex;
    justify-content: center;
    align-items: stretch;
    pointer-events: none;
}

.tracker-shell {
    width: 100%;
    max-width: 1320dp;
    display: flex;
    flex-direction: column;
    gap: 16dp;
    padding: 20dp 22dp 24dp;
    border: 1dp solid rgba(255, 255, 255, 0.08);
    border-radius: 22dp;
    background: linear-gradient(180deg, rgba(18, 18, 18, 0.98), rgba(28, 28, 28, 0.96));
    box-shadow: 0 20dp 60dp rgba(0, 0, 0, 0.45);
    pointer-events: auto;
    overflow: hidden;
}

.tracker-header {
    display: flex;
    justify-content: space-between;
    align-items: flex-start;
    gap: 16dp;
}

.tracker-titleblock {
    display: flex;
    flex-direction: column;
    gap: 6dp;
}

.tracker-kicker {
    font-family: "Fira Sans Condensed";
    font-size: 12dp;
    text-transform: uppercase;
    letter-spacing: 0.12em;
    color: #8ea6c0;
}

.tracker-titleblock h1 {
    margin: 0;
    font-family: "AlegreyaSC";
    font-size: 30dp;
    font-weight: bold;
    color: #f1efe7;
}

.tracker-status {
    font-size: 14dp;
    color: #a29b8b;
}

.tracker-close {
    width: 38dp;
    height: 38dp;
    padding: 0;
    border: 0;
    border-radius: 10dp;
    background: rgba(255, 255, 255, 0.06);
    color: #f1efe7;
    display: flex;
    align-items: center;
    justify-content: center;
}

.tracker-close:hover {
    background: rgba(255, 255, 255, 0.11);
}

.tracker-summary-row {
    display: flex;
    flex-direction: column;
    gap: 10dp;
    padding: 14dp 16dp;
    border-radius: 18dp;
    background: rgba(255, 255, 255, 0.035);
}

.tracker-summary-meta {
    display: flex;
    align-items: center;
    gap: 12dp;
    font-family: "Fira Sans Condensed";
    font-size: 15dp;
    letter-spacing: 0.04em;
    text-transform: uppercase;
}

.tracker-progress {
    height: 6dp;
    border-radius: 999dp;
    background: rgba(255, 255, 255, 0.08);
    overflow: hidden;
}

.tracker-progress-fill {
    width: 0%;
    height: 100%;
    border-radius: 999dp;
    background: linear-gradient(90deg, #7bd66b, #cbe56c);
    box-shadow: 0 0 14dp rgba(138, 220, 105, 0.45);
}

.tracker-sections {
    display: flex;
    flex-direction: column;
    gap: 18dp;
    overflow-y: auto;
    padding-right: 6dp;
}

.tracker-section {
    display: flex;
    flex-direction: column;
    gap: 10dp;
}

.tracker-section-title {
    display: flex;
    align-items: center;
    gap: 10dp;
    font-family: "Fira Sans Condensed";
    font-size: 17dp;
    font-weight: bold;
    text-transform: uppercase;
    color: #ede7d2;
    letter-spacing: 0.08em;
}

.tracker-section-title::after {
    content: "";
    flex: 1 1 auto;
    height: 1dp;
    background: linear-gradient(90deg, rgba(237, 231, 210, 0.45), rgba(237, 231, 210, 0));
}

.tracker-grid {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(58dp, 58dp));
    gap: 12dp;
}

.tracker-card {
    width: 58dp;
    height: 70dp;
    padding: 6dp 4dp 4dp;
    border: 1dp solid rgba(255, 255, 255, 0.06);
    border-radius: 14dp;
    background: rgba(255, 255, 255, 0.035);
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: flex-start;
    gap: 4dp;
    transition: transform 0.12s ease, background-color 0.12s ease, border-color 0.12s ease,
        opacity 0.12s ease, filter 0.12s ease;
}

.tracker-card:hover {
    transform: translateY(-2dp);
    border-color: rgba(255, 255, 255, 0.14);
    background: rgba(255, 255, 255, 0.06);
}

.tracker-card.locked {
    opacity: 0.32;
}

.tracker-card.locked .tracker-card-icon {
    filter: grayscale(100%) brightness(0.55) contrast(0.9);
    opacity: 0.75;
}

.tracker-card.owned {
    opacity: 1;
}

.tracker-card.owned .tracker-card-icon {
    filter: none;
    opacity: 1;
}

.tracker-card-icon {
    width: 42dp;
    height: 42dp;
    image-rendering: auto;
}

.tracker-card-label {
    display: none;
}

icon {
    font-family: "MaterialSymbolsRounded";
    font-weight: normal;
    display: inline-block;
    vertical-align: middle;
}
)RCSS";

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

    content->SetInnerRML(R"RML(<style>)RML" + kChecklistStyles + R"RML(</style>)RML" +
                         kChecklistContent);
    mStatusText = content->GetElementById("tracker-status");
    mSummaryText = content->GetElementById("tracker-summary");
    mSummaryFill = content->GetElementById("tracker-summary-fill");
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
    icon->SetAttribute("src", ItemChecklist::instance().iconPathFor(item.id));

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
        it->second.icon->SetAttribute("src", ItemChecklist::instance().iconPathFor(itemId));
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

    Window::update();
}

}  // namespace dusk::ui
