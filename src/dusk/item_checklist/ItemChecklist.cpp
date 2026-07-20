#include "ItemChecklist.h"

#include "d/d_com_inf_game.h"
#include "dusk/io.hpp"
#include "dusk/item_checklist_hooks.h"
#include "dusk/logging.h"
#include "dusk/main.h"
#include "dusk/platform_support.hpp"
#include "dusk/ui/ui.hpp"

#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_iostream.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <filesystem>
#include <string>
#include <system_error>

using json = nlohmann::json;

namespace {

constexpr auto kItemDefinitionsFile = "item_checklist_items.json";
constexpr auto kOverrideFile = "item_checklist.json";

struct BundledItemDefinition {
    uint8_t id;
    const char* name;
    const char* tab;
    const char* category;
    const char* iconPath;
    uint8_t liveItemId;
    bool useLiveState;
};

constexpr std::array<BundledItemDefinition, 12> kFallbackItems{{
    {74, "Fishing Rod", "Essentials", "Wheel", "res/item_tracker/rod.png", 74, true},
    {75, "Slingshot", "Essentials", "Wheel", "res/item_tracker/slingshot.png", 75, true},
    {72, "Lantern", "Essentials", "Wheel", "res/item_tracker/lantern.png", 72, true},
    {64, "Boomerang", "Essentials", "Wheel", "res/item_tracker/boomerang.png", 64, true},
    {69, "Iron Boots", "Essentials", "Wheel", "res/item_tracker/iron-boots.png", 69, true},
    {67, "Bow", "Essentials", "Wheel", "res/item_tracker/bow.png", 67, true},
    {62, "Hawkeye", "Essentials", "Wheel", "res/item_tracker/hawkeye.png", 62, true},
    {80, "Bomb Bag", "Essentials", "Wheel", "res/item_tracker/bombbag.png", 80, true},
    {79, "Giant Bomb Bags", "Essentials", "Wheel", "res/item_tracker/giantbombbag.png", 79, true},
    {70, "Clawshot", "Essentials", "Wheel", "res/item_tracker/clawshot.png", 70, true},
    {65, "Spinner", "Essentials", "Wheel", "res/item_tracker/spinner.png", 65, true},
    {66, "Ball and Chain", "Essentials", "Wheel", "res/item_tracker/chainball.png", 66, true},
}};

std::vector<u8> read_io_full(SDL_IOStream* io) {
    std::vector<u8> data;
    if (io == nullptr) {
        return data;
    }

    std::array<u8, 4096> buffer{};
    while (true) {
        const size_t bytesRead = SDL_ReadIO(io, buffer.data(), buffer.size());
        if (bytesRead == 0) {
            break;
        }
        data.insert(data.end(), buffer.begin(), buffer.begin() + bytesRead);
        if (bytesRead < buffer.size()) {
            break;
        }
    }
    return data;
}

std::vector<u8> read_bundled_bytes(const std::filesystem::path& path) {
    const std::string pathUtf8 = dusk::io::fs_path_to_string(path);

    SDL_IOStream* io = SDL_IOFromFile(pathUtf8.c_str(), "rb");
    if (io != nullptr) {
        auto data = read_io_full(io);
        SDL_CloseIO(io);
        return data;
    }

    const auto absolutePath = dusk::platform::BundledPath(path);
    const std::string absoluteUtf8 = dusk::io::fs_path_to_string(absolutePath);
    io = SDL_IOFromFile(absoluteUtf8.c_str(), "rb");
    if (io != nullptr) {
        auto data = read_io_full(io);
        SDL_CloseIO(io);
        return data;
    }

    return {};
}

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
    const auto* item = getItemInfo(itemId);
    if (item == nullptr) {
        return false;
    }
    if (!item->useLiveState) {
        if (const auto overrideIt = mManualOverrides.find(itemId); overrideIt != mManualOverrides.end()) {
            return overrideIt->second;
        }
    }
    if (const auto liveIt = mLiveCollected.find(itemId); liveIt != mLiveCollected.end()) {
        return liveIt->second;
    }
    return false;
}

void ItemChecklist::setCollected(uint8_t itemId, bool collected) {
    const auto* item = getItemInfo(itemId);
    if (item == nullptr || item->useLiveState) {
        return;
    }
    if (const auto existing = mManualOverrides.find(itemId);
        existing != mManualOverrides.end() && existing->second == collected) {
        return;
    }
    mManualOverrides[itemId] = collected;
    bumpRevision();
    save();
}

void ItemChecklist::toggleCollected(uint8_t itemId) {
    setCollected(itemId, !isCollected(itemId));
}

void ItemChecklist::onItemCollected(uint8_t itemId) {
    const auto* item = getItemInfo(itemId);
    if (item == nullptr) {
        return;
    }
    if (item->useLiveState) {
        syncItemStateFromGame(itemId, true);
        return;
    }
    setCollected(itemId, true);
}

void ItemChecklist::refresh() {
    if (!mInitialized) {
        return;
    }

    syncItemStateFromGame();
}

void ItemChecklist::onItemSlotChanged(uint8_t itemId) {
    if (!mInitialized) {
        return;
    }

    bool changed = false;
    for (const auto& item : mItemDefinitions) {
        if (!item.useLiveState || item.liveItemId != itemId) {
            continue;
        }

        const bool collected = collectedFromGame(item);
        if (mLiveCollected[item.id] != collected) {
            mLiveCollected[item.id] = collected;
            changed = true;
        }
    }

    if (changed) {
        bumpRevision();
    }
}

void ItemChecklist::onItemFirstBitChanged(uint8_t itemId, bool collected) {
    if (!mInitialized) {
        return;
    }
    syncItemStateFromGame(itemId, collected);
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

std::vector<const ItemChecklist::ItemInfo*> ItemChecklist::getItemsByTab(const std::string& tab) const {
    std::vector<const ItemInfo*> result;
    for (const auto& item : mItemDefinitions) {
        if (tab == "Speedrun" || item.tab == tab) {
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

std::vector<std::string> ItemChecklist::tabs() const {
    std::vector<std::string> result;
    result.reserve(mItemDefinitions.size());
    for (const auto& item : mItemDefinitions) {
        if (item.tab.empty()) {
            continue;
        }
        if (std::find(result.begin(), result.end(), item.tab) == result.end()) {
            result.push_back(item.tab);
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
        const auto data = read_bundled_bytes(dusk::ui::resource_path(kItemDefinitionsFile));
        if (!data.empty()) {
            json root = json::parse(data);
            for (const auto& item : root) {
                ItemInfo info;
                info.id = item.value("id", 0);
                info.name = item.value("name", "");
                info.tab = item.value("tab", "Essentials");
                info.category = item.value("category", "Wheel");
                info.iconPath = item.value("iconPath", "");
                info.liveItemId = item.value("liveItemId", info.id);
                info.useLiveState = item.value("useLiveState", true);
                info.isQuestItem = item.value("isQuestItem", false);
                mItemDefinitions.push_back(std::move(info));
            }
        } else {
            DuskLog.warn("ItemChecklist: bundled item definitions missing, using fallback list");
        }
    } catch (const std::exception& e) {
        DuskLog.error("ItemChecklist: failed to parse item definitions: {}", e.what());
        mItemDefinitions.clear();
    }

    if (mItemDefinitions.empty()) {
        mItemDefinitions.reserve(kFallbackItems.size());
        for (const auto& item : kFallbackItems) {
            ItemInfo info;
            info.id = item.id;
            info.name = item.name;
            info.tab = item.tab;
            info.category = item.category;
            info.iconPath = item.iconPath;
            info.liveItemId = item.liveItemId;
            info.useLiveState = item.useLiveState;
            mItemDefinitions.push_back(std::move(info));
        }
    }

    for (size_t i = 0; i < mItemDefinitions.size(); ++i) {
        mItemMap[mItemDefinitions[i].id] = i;
    }
}

void ItemChecklist::syncItemStateFromGame() {
    bool changed = false;
    for (const auto& item : mItemDefinitions) {
        const bool collected = collectedFromGame(item);
        if (mLiveCollected[item.id] != collected) {
            mLiveCollected[item.id] = collected;
            changed = true;
        }
    }
    if (changed) {
        bumpRevision();
    }
}

void ItemChecklist::syncItemStateFromGame(uint8_t itemId, bool collected) {
    bool changed = false;
    for (const auto& item : mItemDefinitions) {
        if (!item.useLiveState || item.liveItemId != itemId) {
            continue;
        }
        if (mLiveCollected[item.id] != collected) {
            mLiveCollected[item.id] = collected;
            changed = true;
        }
    }
    if (changed) {
        bumpRevision();
    }
}

bool ItemChecklist::collectedFromGame(const ItemInfo& item) const {
    if (!item.useLiveState) {
        return false;
    }
    return dComIfGs_isItemFirstBit(item.liveItemId) != 0;
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

void ItemChecklist::bumpRevision() {
    ++mRevision;
}

namespace dusk::item_checklist {

void on_item_slot_changed(uint8_t itemId) {
    auto& checklist = ItemChecklist::instance();
    if (!checklist.isInitialized()) {
        return;
    }
    checklist.onItemSlotChanged(itemId);
}

void on_item_first_bit_changed(uint8_t itemId, bool collected) {
    auto& checklist = ItemChecklist::instance();
    if (!checklist.isInitialized()) {
        return;
    }
    checklist.onItemFirstBitChanged(itemId, collected);
}

}  // namespace dusk::item_checklist
