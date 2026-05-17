#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace dusk::ui {
class ItemChecklistDocument;
}

class ItemChecklist {
public:
    struct ItemInfo {
        uint8_t id = 0;
        std::string name;
        std::string category;
        std::string iconPath;
        bool isQuestItem = false;
    };

    static ItemChecklist& instance();

    bool initialize();
    void shutdown();

    bool isCollected(uint8_t itemId) const;
    void setCollected(uint8_t itemId, bool collected);
    void toggleCollected(uint8_t itemId);

    void showChecklist();
    void hideChecklist();
    bool isVisible() const;

    void onItemCollected(uint8_t itemId);
    void refresh();

    const ItemInfo* getItemInfo(uint8_t itemId) const;
    std::vector<const ItemInfo*> getItemsByCategory(const std::string& category) const;
    const std::vector<ItemInfo>& items() const { return mItemDefinitions; }
    std::vector<std::string> categories() const;

    std::string iconPathFor(uint8_t itemId) const;
    bool iconsReady() const { return mIconsReady; }

private:
    ItemChecklist() = default;
    ~ItemChecklist() = default;

    void loadItemDefinitions();
    void createDocument();
    void save();
    void load();

    void syncItemStateFromGame();
    void rebuildIconCache();
    bool isGameAssetReady() const;
    bool collectedFromGame(const ItemInfo& item) const;

    std::unordered_map<uint8_t, bool> mLiveCollected;
    std::unordered_map<uint8_t, bool> mManualOverrides;
    std::vector<ItemInfo> mItemDefinitions;
    std::unordered_map<uint8_t, size_t> mItemMap;

    std::unique_ptr<dusk::ui::ItemChecklistDocument> mDocument;
    std::filesystem::path mIconCacheDir;
    bool mVisible = false;
    bool mInitialized = false;
    bool mIconsReady = false;
};
