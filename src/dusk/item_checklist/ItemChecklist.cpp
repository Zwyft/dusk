#include "ItemChecklist.h"

#include "JSystem/JKernel/JKRArchive.h"
#include "JSystem/JUtility/JUTTexture.h"
#include "d/d_com_inf_game.h"
#include "d/d_item_data.h"
#include "dusk/gx_decode.h"
#include "dusk/io.hpp"
#include "dusk/logging.h"
#include "dusk/main.h"

#include "fmt/format.h"
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <string_view>

using json = nlohmann::json;

namespace {

constexpr auto kItemDefinitionsFile = "res/item_checklist_items.json";
constexpr auto kOverrideFile = "item_checklist.json";

#pragma pack(push, 1)
struct BmpFileHeader {
    uint16_t type = 0x4D42;
    uint32_t size = 0;
    uint16_t reserved1 = 0;
    uint16_t reserved2 = 0;
    uint32_t offBits = 54;
};

struct BmpInfoHeader {
    uint32_t size = 40;
    int32_t width = 0;
    int32_t height = 0;
    uint16_t planes = 1;
    uint16_t bitCount = 32;
    uint32_t compression = 0;
    uint32_t imageSize = 0;
    int32_t xPelsPerMeter = 2835;
    int32_t yPelsPerMeter = 2835;
    uint32_t clrUsed = 0;
    uint32_t clrImportant = 0;
};
#pragma pack(pop)

std::string to_rml_path(const std::filesystem::path& path) {
    return path.generic_string();
}

std::string normalize_name(std::string_view value) {
    std::string result;
    result.reserve(value.size());
    bool capitalizeNext = true;
    for (char ch : value) {
        if (ch == '_' || ch == '-') {
            result.push_back(' ');
            capitalizeNext = true;
            continue;
        }
        if (capitalizeNext && std::isalpha(static_cast<unsigned char>(ch))) {
            result.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(ch))));
            capitalizeNext = false;
            continue;
        }
        result.push_back(ch);
        capitalizeNext = false;
    }
    return result;
}

bool write_bmp(const std::filesystem::path& path, const std::vector<uint8_t>& rgba, uint32_t width,
    uint32_t height) {
    if (rgba.size() != static_cast<size_t>(width) * static_cast<size_t>(height) * 4) {
        return false;
    }

    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
        return false;
    }

    const uint32_t imageSize = width * height * 4;
    const uint32_t fileSize = static_cast<uint32_t>(sizeof(BmpFileHeader) + sizeof(BmpInfoHeader)) +
                               imageSize;

    BmpFileHeader fileHeader;
    fileHeader.size = fileSize;

    BmpInfoHeader infoHeader;
    infoHeader.width = static_cast<int32_t>(width);
    infoHeader.height = static_cast<int32_t>(height);
    infoHeader.imageSize = imageSize;

    out.write(reinterpret_cast<const char*>(&fileHeader), sizeof(fileHeader));
    out.write(reinterpret_cast<const char*>(&infoHeader), sizeof(infoHeader));

    for (int32_t y = static_cast<int32_t>(height) - 1; y >= 0; --y) {
        const size_t rowStart = static_cast<size_t>(y) * width * 4;
        for (uint32_t x = 0; x < width; ++x) {
            const size_t i = rowStart + static_cast<size_t>(x) * 4;
            const char pixel[4] = {
                static_cast<char>(rgba[i + 2]),
                static_cast<char>(rgba[i + 1]),
                static_cast<char>(rgba[i + 0]),
                static_cast<char>(rgba[i + 3]),
            };
            out.write(pixel, sizeof(pixel));
        }
    }

    return static_cast<bool>(out);
}

std::vector<uint8_t> decode_timg(const ResTIMG* timg) {
    if (timg == nullptr || timg->imageOffset == 0 || timg->width == 0 || timg->height == 0) {
        return {};
    }
    const auto* pixelData = reinterpret_cast<const uint8_t*>(timg) + timg->imageOffset;
    return decode_gx_to_rgba(timg->format, timg->width, timg->height, pixelData);
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
    mIconCacheDir = dusk::ConfigPath / "item-tracker-icons";
    load();
    mInitialized = true;
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
    if (const auto* item = getItemInfo(itemId); item != nullptr && !item->iconPath.empty()) {
        return item->iconPath;
    }
    return "res/icon.png";
}

void ItemChecklist::loadItemDefinitions() {
    mItemDefinitions.clear();
    mItemMap.clear();

    if (std::ifstream file{std::filesystem::path(kItemDefinitionsFile)}; file.is_open()) {
        try {
            json root;
            file >> root;
            for (const auto& item : root) {
                ItemInfo info;
                info.id = item.value("id", 0);
                info.name = item.value("name", "");
                info.category = item.value("category", "Other");
                info.iconPath = item.value("iconPath", "");
                info.isQuestItem = item.value("isQuestItem", false);
                mItemDefinitions.push_back(std::move(info));
            }
        } catch (const std::exception& e) {
            DuskLog.error("ItemChecklist: failed to parse item definitions: {}", e.what());
            mItemDefinitions.clear();
        }
    }

    if (mItemDefinitions.empty()) {
        for (uint16_t itemId = 0; itemId < 256; ++itemId) {
            const char* arcName = dItem_data::getArcName(static_cast<u8>(itemId));
            if (arcName == nullptr || arcName[0] == '\0') {
                continue;
            }

            ItemInfo info;
            info.id = static_cast<uint8_t>(itemId);
            info.name = normalize_name(arcName);
            info.category = itemId < 0x18 ? "Equipment" : "Quest";
            info.isQuestItem = itemId >= 0x18;
            mItemDefinitions.push_back(std::move(info));
        }
    }

    for (size_t i = 0; i < mItemDefinitions.size(); ++i) {
        mItemMap[mItemDefinitions[i].id] = i;
    }
}

bool ItemChecklist::isGameAssetReady() const {
    return dComIfGp_getItemIconArchive() != nullptr;
}

bool ItemChecklist::collectedFromGame(const ItemInfo& item) const {
    return dComIfGs_isItemFirstBit(item.id) != 0;
}

void ItemChecklist::syncItemStateFromGame() {
    for (const auto& item : mItemDefinitions) {
        mLiveCollected[item.id] = collectedFromGame(item);
    }

    if (!mIconsReady && isGameAssetReady()) {
        rebuildIconCache();
    }
}

void ItemChecklist::rebuildIconCache() {
    auto* archive = dComIfGp_getItemIconArchive();
    if (archive == nullptr) {
        return;
    }

    std::error_code ec;
    std::filesystem::create_directories(mIconCacheDir, ec);
    if (ec) {
        DuskLog.warn("ItemChecklist: failed to create icon cache dir {}: {}",
            to_rml_path(mIconCacheDir), ec.message());
        return;
    }

    bool wroteAnyIcon = false;
    for (auto& item : mItemDefinitions) {
        const int texIndex = dItem_data::getTexture(item.id);
        if (texIndex < 0) {
            if (item.iconPath.empty()) {
                item.iconPath = "res/icon.png";
            }
            continue;
        }

        const auto* timg = static_cast<const ResTIMG*>(archive->getIdxResource(static_cast<u32>(texIndex)));
        const auto rgba = decode_timg(timg);
        if (rgba.empty()) {
            if (item.iconPath.empty()) {
                item.iconPath = "res/icon.png";
            }
            continue;
        }

        const auto iconPath = mIconCacheDir / fmt::format("item_{:03}.bmp", item.id);
        if (write_bmp(iconPath, rgba, timg->width, timg->height)) {
            item.iconPath = to_rml_path(iconPath);
            wroteAnyIcon = true;
        } else {
            if (item.iconPath.empty()) {
                item.iconPath = "res/icon.png";
            }
        }
    }

    mIconsReady = true;
    if (!wroteAnyIcon) {
        DuskLog.warn("ItemChecklist: disc icon cache rebuilt with fallbacks only");
    }
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

    std::ofstream file(savePath);
    if (!file.is_open()) {
        DuskLog.error("ItemChecklist: failed to open {} for writing", to_rml_path(savePath));
        return;
    }

    file << root.dump(2);
}

void ItemChecklist::load() {
    const auto savePath = dusk::ConfigPath / kOverrideFile;
    std::ifstream file(savePath);
    if (!file.is_open()) {
        return;
    }

    try {
        json root;
        file >> root;
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
