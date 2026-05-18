#include "ItemChecklist.h"

#include "d/d_com_inf_game.h"
#include "dusk/io.hpp"
#include "dusk/logging.h"
#include "dusk/main.h"
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
    const char* iconPath;
};

constexpr std::array<BundledItemDefinition, 12> kFallbackItems{{
    {74, "Fishing Rod", "res/item_tracker/rod.png"},
    {75, "Slingshot", "res/item_tracker/slingshot.png"},
    {72, "Lantern", "res/item_tracker/lantern.png"},
    {64, "Boomerang", "res/item_tracker/boomerang.png"},
    {69, "Iron Boots", "res/item_tracker/iron-boots.png"},
    {67, "Bow", "res/item_tracker/bow.png"},
    {62, "Hawkeye", "res/item_tracker/hawkeye.png"},
    {80, "Bomb Bag", "res/item_tracker/bombbag.png"},
    {79, "Giant Bomb Bags", "res/item_tracker/giantbombbag.png"},
    {70, "Clawshot", "res/item_tracker/clawshot.png"},
    {65, "Spinner", "res/item_tracker/spinner.png"},
    {66, "Ball and Chain", "res/item_tracker/chainball.png"},
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

    if (const char* basePath = SDL_GetBasePath(); basePath != nullptr) {
        const auto absolutePath = std::filesystem::path(basePath) / path;
        const std::string absoluteUtf8 = dusk::io::fs_path_to_string(absolutePath);
        io = SDL_IOFromFile(absoluteUtf8.c_str(), "rb");
        if (io != nullptr) {
            auto data = read_io_full(io);
            SDL_CloseIO(io);
            return data;
        }
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
        const auto data = read_bundled_bytes(dusk::ui::resource_path(kItemDefinitionsFile));
        if (!data.empty()) {
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
            info.category = "Wheel";
            info.iconPath = item.iconPath;
            mItemDefinitions.push_back(std::move(info));
        }
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
