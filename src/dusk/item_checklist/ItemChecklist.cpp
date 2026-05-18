#include "ItemChecklist.h"

#include "d/d_com_inf_game.h"
#include "dusk/io.hpp"
#include "dusk/logging.h"
#include "dusk/main.h"
#include "dusk/ui/ui.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <filesystem>
#include <string>
#include <system_error>

using json = nlohmann::json;

namespace {

constexpr auto kItemDefinitionsFile = "item_checklist_items.json";
constexpr auto kOverrideFile = "item_checklist.json";

}  // namespace

ItemChecklist& ItemChecklist::instance() {
    static ItemChecklist instance;
    return instance;
}

bool ItemChecklist::initialize() {
    if (mInitialized) {
        return true;
    }

    loadItemDefinitions();
    load();
    mInitialized = true;
    mIconsReady = true;
    refresh();
    return true;
}

void ItemChecklist::shutdown() {
    save();
    mInitialized = false;
    mIconsReady = false;
    mLiveCollected.clear();
    mManualOverrides.clear();
}

bool ItemChecklist::isCollected(uint8_t itemId) const {
    if (const auto overrideIt = mManualOverrides.find(itemId); overrideIt != mManualOverrides.end()) {
        return overrideIt->second;
    }
    if (const auto liveIt = mLiveCollected.find(itemId); liveIt != mLiveCollected.end()) {
        return liveIt->second;
    }
    return false;
}

void ItemChecklist::setCollected(uint8_t itemId, bool collected) {
    mManualOverrides[itemId] = collected;
    save();
}

void ItemChecklist::toggleCollected(uint8_t itemId) {
    setCollected(itemId, !isCollected(itemId));
}

void ItemChecklist::onItemCollected(uint8_t itemId) {
    setCollected(itemId, true);
}

void ItemChecklist::refresh() {
    if (!mInitialized) {
        return;
    }

    syncItemStateFromGame();
}

const ItemChecklist::ItemInfo* ItemChecklist::getItemInfo(uint8_t itemId) const {
    if (const auto it = mItemMap.find(itemId); it != mItemMap.end()) {
        return &mItemDefinitions[it->second];
    }
    return nullptr;
}

std::vector<const ItemChecklist::ItemInfo*> ItemChecklist::getItemsByCategory(
    const std::string& category) const {
    std::vector<const ItemInfo*> result;
    for (const auto& item : mItemDefinitions) {
        if (category == "All" || item.category == category) {
            result.push_back(&item);
        }
    }
    return result;
}

std::vector<std::string> ItemChecklist::categories() const {
    std::vector<std::string> result;
    result.reserve(mItemDefinitions.size());
    for (const auto& item : mItemDefinitions) {
        if (std::find(result.begin(), result.end(), item.category) == result.end()) {
            result.push_back(item.category);
        }
    }
    return result;
}

std::string ItemChecklist::iconPathFor(uint8_t itemId) const {
    if (const auto* item = getItemInfo(itemId); item != nullptr) {
        return item->iconPath;
    }
    return {};
}

void ItemChecklist::loadItemDefinitions() {
    mItemDefinitions.clear();
    mItemMap.clear();

    try {
        const auto data =
            dusk::io::FileStream::ReadAllBytes(dusk::ui::resource_path(kItemDefinitionsFile));
        json root = json::parse(data);
        for (const auto& item : root) {
            ItemInfo info;
            info.id = item.value("id", 0);
            info.name = item.value("name", "");
            info.category = item.value("category", "Wheel");
            info.iconPath = item.value("iconPath", "");
            info.isQuestItem = item.value("isQuestItem", false);
            mItemDefinitions.push_back(std::move(info));
        }
    } catch (const std::exception& e) {
        DuskLog.error("ItemChecklist: failed to parse item definitions: {}", e.what());
        mItemDefinitions.clear();
    }

    for (size_t i = 0; i < mItemDefinitions.size(); ++i) {
        mItemMap[mItemDefinitions[i].id] = i;
    }
}

void ItemChecklist::syncItemStateFromGame() {
    for (const auto& item : mItemDefinitions) {
        mLiveCollected[item.id] = collectedFromGame(item);
    }
}

bool ItemChecklist::collectedFromGame(const ItemInfo& item) const {
    return dComIfGs_isItemFirstBit(item.id) != 0;
}

void ItemChecklist::save() {
    const auto savePath = dusk::ConfigPath / kOverrideFile;
    std::error_code ec;
    std::filesystem::create_directories(savePath.parent_path(), ec);
    if (ec) {
        DuskLog.error("ItemChecklist: failed to create save dir: {}", ec.message());
        return;
    }

    json root = json::object();
    auto& overrides = root["manualOverrides"];
    overrides = json::object();
    for (const auto& [itemId, collected] : mManualOverrides) {
        overrides[std::to_string(itemId)] = collected;
    }

    dusk::io::FileStream::WriteAllText(savePath, root.dump(2));
}

void ItemChecklist::load() {
    const auto savePath = dusk::ConfigPath / kOverrideFile;
    if (!std::filesystem::exists(savePath)) {
        return;
    }

    try {
        const auto data = dusk::io::FileStream::ReadAllBytes(savePath);
        json root = json::parse(data);
        const auto overridesIt = root.find("manualOverrides");
        if (overridesIt != root.end() && overridesIt->is_object()) {
            mManualOverrides.clear();
            for (const auto& [key, value] : overridesIt->items()) {
                mManualOverrides[static_cast<uint8_t>(std::stoul(key))] = value.get<bool>();
            }
        }
    } catch (const std::exception& e) {
        DuskLog.error("ItemChecklist: failed to parse save file: {}", e.what());
    }
}
