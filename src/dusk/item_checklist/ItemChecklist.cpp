#include "ItemChecklist.h"

#include "d/d_item_data.h"
#include "d/d_com_inf_game.h"
#include "dusk/io.hpp"
#include "dusk/ui/ui.hpp"
#include "dusk/ui/button.hpp"
#include "dusk/ui/component.hpp"
#include "fmt/format.h"
#include <nlohmann/json.hpp>

#include <RmlUi/Core.h>
#include <filesystem>

namespace dusk::ui {

ItemChecklist& ItemChecklist::instance() {
    static ItemChecklist instance;
    return instance;
}

bool ItemChecklist::initialize() {
    if (mInitialized) return true;
    
    loadItemDefinitions();
    createDocument();
    
    load(); // Load saved state
    
    mInitialized = true;
    return true;
}

void ItemChecklist::shutdown() {
    save();
    mDocument.reset();
    mInitialized = false;
}

bool ItemChecklist::isCollected(uint8_t itemId) const {
    auto it = mCollectedItems.find(itemId);
    return it != mCollectedItems.end() && it->second;
}

void ItemChecklist::setCollected(uint8_t itemId, bool collected) {
    mCollectedItems[itemId] = collected;
    if (mDocument) {
        mDocument->setCollected(itemId, collected);
    }
    updateProgress();
}

void ItemChecklist::toggleCollected(uint8_t itemId) {
    setCollected(itemId, !isCollected(itemId));
}

void ItemChecklist::onItemCollected(uint8_t itemId) {
    setCollected(itemId, true);
}

void ItemChecklist::refresh() {
    if (!mInitialized) return;
    
    bool updated = false;
    for (const auto& item : mItemDefinitions) {
        // Check if item is collected using game's function
        bool collected = dComIfGs_isItemFirstBit(item.id) != 0;
        if (collected != isCollected(item.id)) {
            setCollected(item.id, collected);
            updated = true;
        }
    }
    
    if (mDocument) {
        mDocument->refresh();
    }
}

const ItemChecklist::ItemInfo* ItemChecklist::getItemInfo(uint8_t itemId) const {
    auto it = mItemMap.find(itemId);
    if (it != mItemMap.end()) {
        return &mItemDefinitions[it->second];
    }
    return nullptr;
}

std::vector<const ItemChecklist::ItemInfo*> ItemChecklist::getItemsByCategory(const std::string& category) const {
    std::vector<const ItemInfo*> result;
    for (const auto& item : mItemDefinitions) {
        if (item.category == category) {
            result.push_back(&item);
        }
    }
    return result;
}

void ItemChecklist::loadItemDefinitions() {
    // Try to load from JSON file first
    std::filesystem::path jsonPath = dusk::io::fs_path("res/item_checklist_items.json");
    if (std::filesystem::exists(jsonPath)) {
        std::ifstream file(jsonPath);
        if (file.is_open()) {
            try {
                nlohmann::json j;
                file >> j;
                
                for (const auto& item : j) {
                    ItemInfo info;
                    info.id = item["id"];
                    info.name = item["name"];
                    info.iconPath = item["iconPath"];
                    info.category = item["category"];
                    info.isQuestItem = item.value("isQuestItem", false);
                    
                    mItemDefinitions.push_back(info);
                }
                
                DuskLog.info("ItemChecklist: Loaded {} items from JSON", mItemDefinitions.size());
                // Build lookup map
                for (size_t i = 0; i < mItemDefinitions.size(); ++i) {
                    mItemMap[mItemDefinitions[i].id] = i;
                }
                return;
            } catch (const std::exception& e) {
                DuskLog.error("ItemChecklist: Failed to parse JSON: {}", e.what());
                // Fall back to generating from game data
            }
        }
    }
    
    // Generate proper paths using game data
    mItemDefinitions.clear();
    for (int itemId = 0; itemId < 256; ++itemId) {
        const char* arcName = dItem_data::getArcName(static_cast<u8>(itemId));
        if (arcName && arcName[0] != '\0') {
            std::string iconPath = "res/Object/" + std::string(arcName) + ".dds";
            
            // Check if the icon file exists (optional)
            std::filesystem::path iconFilePath = dusk::io::fs_path(iconPath);
            if (!std::filesystem::exists(iconFilePath)) {
                // Use placeholder if icon not found
                iconPath = "items/placeholder.png"; // You need to provide this
            }
            
            // Determine category based on item name (simple heuristic)
            std::string category = "Other";
            std::string name = arcName;
            
            // You can add more sophisticated mapping here
            mItemDefinitions.push_back({
                static_cast<u8>(itemId),
                name,
                iconPath,
                category,
                false
            });
        }
    }
    
    // Build lookup map
    for (size_t i = 0; i < mItemDefinitions.size(); ++i) {
        mItemMap[mItemDefinitions[i].id] = i;
    }
    
    DuskLog.info("ItemChecklist: Generated {} item definitions from game data", mItemDefinitions.size());
}

void ItemChecklist::save() {
    std::filesystem::path saveDir = dusk::io::fs_path("save");
    std::filesystem::create_directories(saveDir);
    
    std::filesystem::path savePath = saveDir / "item_checklist.json";
    std::ofstream file(savePath);
    if (!file.is_open()) {
        DuskLog.error("ItemChecklist: Failed to open save file for writing");
        return;
    }
    
    nlohmann::json j;
    for (const auto& [itemId, collected] : mCollectedItems) {
        j[std::to_string(itemId)] = collected;
    }
    
    file << j.dump(4);
    DuskLog.info("ItemChecklist: Saved state to {}", savePath.string());
}

void ItemChecklist::load() {
    std::filesystem::path savePath = dusk::io::fs_path("save/item_checklist.json");
    if (!std::filesystem::exists(savePath)) {
        DuskLog.info("ItemChecklist: No save file found");
        return;
    }
    
    std::ifstream file(savePath);
    if (!file.is_open()) {
        DuskLog.error("ItemChecklist: Failed to open save file for reading");
        return;
    }
    
    try {
        nlohmann::json j;
        file >> j;
        
        for (auto it = j.begin(); it != j.end(); ++it) {
            uint8_t itemId = std::stoi(it.key());
            bool collected = it.value();
            mCollectedItems[itemId] = collected;
        }
        
        DuskLog.info("ItemChecklist: Loaded saved state ({} items)", mCollectedItems.size());
        if (mDocument) {
            mDocument->refresh();
        }
    } catch (const std::exception& e) {
        DuskLog.error("ItemChecklist: Failed to parse save file: {}", e.what());
    }
}

void ItemChecklist::showChecklist() {
    if (!mInitialized) initialize();
    if (mDocument) {
        mVisible = true;
        mDocument->Show();
    }
}

void ItemChecklist::hideChecklist() {
    mVisible = false;
    if (mDocument) {
        mDocument->Hide();
    }
}

bool ItemChecklist::isVisible() const {
    return mVisible && mDocument && mDocument->IsVisible();
}

void ItemChecklist::createDocument() {
    // Create the RmlUI document
    mDocument = std::make_unique<ItemChecklistDocument>();
    if (mDocument) {
        mDocument->Initialize("res/rml/item_checklist.rml");
    }
}

void ItemChecklist::updateProgress() {
    if (mDocument) {
        mDocument->updateProgress();
    }
}

} // namespace dusk::ui