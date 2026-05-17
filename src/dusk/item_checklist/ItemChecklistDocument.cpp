#include "ItemChecklistDocument.h"

#include "ItemChecklist.h"

#include "dusk/io.hpp"
#include "dusk/ui/ui.hpp"

#include <RmlUi/Core.h>

#include <fstream>
#include <iterator>
#include <string>

#include "fmt/format.h"

namespace dusk::ui {
namespace {

const Rml::String kDocumentSource = R"RML(
<rml>
<head>
    <link type="text/rcss" href="res/rcss/item_checklist.rcss" />
</head>
<body>
    <div id="tracker-root" class="tracker-window">
        <div class="tracker-shell">
            <div class="tracker-header">
                <div class="tracker-titleblock">
                    <div class="tracker-kicker">Item tracker</div>
                    <h1>Emotracker-style checklist</h1>
                    <div id="tracker-status" class="tracker-status">Waiting for disc assets</div>
                </div>
                <button class="tracker-close" onmousedown="this.blur(); {dusk::ui::ItemChecklist::instance().hideChecklist();}">
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
</body>
</rml>
)RML";

Rml::String load_document_source() {
    if (std::ifstream file(dusk::io::fs_path("res/rml/item_checklist.rml")); file.is_open()) {
        return Rml::String{
            std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>())};
    }
    return kDocumentSource;
}

}  // namespace

ItemChecklistDocument::ItemChecklistDocument() : Document(load_document_source()) {
    build();
}

void ItemChecklistDocument::build() {
    mStatusText = mDocument->GetElementById("tracker-status");
    mSummaryText = mDocument->GetElementById("tracker-summary");
    mSummaryFill = mDocument->GetElementById("tracker-summary-fill");
    mSectionsRoot = mDocument->GetElementById("tracker-sections");
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
    const ItemChecklist::ItemInfo& item, Rml::Element* parent) {
    CardRefs refs;
    auto* doc = mDocument;
    if (doc == nullptr || parent == nullptr) {
        return refs;
    }

    auto* button = doc->CreateElement("button");
    button->SetClass("tracker-card", true);
    button->SetAttribute("type", "button");
    button->SetAttribute("data-item-id", std::to_string(item.id));
    button->SetAttribute("title", item.name);

    auto* icon = doc->CreateElement("img");
    icon->SetClass("tracker-card-icon", true);
    icon->SetAttribute("src", ItemChecklist::instance().iconPathFor(item.id));
    button->AppendChild(std::move(icon));

    auto* label = doc->CreateElement("div");
    label->SetClass("tracker-card-label", true);
    label->SetInnerRML(escape(item.name));
    button->AppendChild(std::move(label));

    refs.root = button;
    refs.icon = icon;
    parent->AppendChild(std::move(button));
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
    Document::update();
    ItemChecklist::instance().refresh();
}

}  // namespace dusk::ui
