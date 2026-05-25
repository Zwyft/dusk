#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class ItemChecklist {
public:
    struct ItemInfo {
        uint8_t id = 0;
        std::string name;
        std::string tab;
        std::string category;
        std::string iconPath;
        uint8_t liveItemId = 0;
        bool useLiveState = true;
        bool isQuestItem = false;
    };

    static ItemChecklist& instance();

    bool initialize();
    void shutdown();
    bool isInitialized() const { return mInitialized; }

    bool isCollected(uint8_t itemId) const;
    void setCollected(uint8_t itemId, bool collected);
    void toggleCollected(uint8_t itemId);

    void onItemCollected(uint8_t itemId);
    void refresh();
    void onItemSlotChanged(uint8_t itemId);
    void onItemFirstBitChanged(uint8_t itemId, bool collected);
    uint64_t revision() const { return mRevision; }

    const ItemInfo* getItemInfo(uint8_t itemId) const;
    std::vector<const ItemInfo*> getItemsByCategory(const std::string& category) const;
    std::vector<const ItemInfo*> getItemsByTab(const std::string& tab) const;
    const std::vector<ItemInfo>& items() const { return mItemDefinitions; }
    std::vector<std::string> categories() const;
    std::vector<std::string> tabs() const;

    std::string iconPathFor(uint8_t itemId) const;
    bool iconsReady() const { return mIconsReady; }

    bool exportManualOverrides(const std::filesystem::path& destination, std::string* error = nullptr) const;
    bool importManualOverrides(const std::filesystem::path& source, std::string* error = nullptr);

private:
    ItemChecklist() = default;
    ~ItemChecklist() = default;

    void loadItemDefinitions();
    void save();
    void load();

    void syncItemStateFromGame();
    void syncItemStateFromGame(uint8_t itemId, bool collected);
    bool collectedFromGame(const ItemInfo& item) const;
    void bumpRevision();

    std::unordered_map<uint8_t, bool> mLiveCollected;
    std::unordered_map<uint8_t, bool> mManualOverrides;
    std::vector<ItemInfo> mItemDefinitions;
    std::unordered_map<uint8_t, size_t> mItemMap;

    bool mInitialized = false;
    bool mIconsReady = false;
    uint64_t mRevision = 1;
};
