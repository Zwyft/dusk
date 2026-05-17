#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

namespace dusk::ui {
    class Document;
}

class ItemChecklist {
public:
    struct ItemInfo {
        uint8_t id;
        std::string name;
        std::string iconPath;
        std::string category;
        bool isQuestItem;
    };

    static ItemChecklist& instance();

    bool initialize();
    void shutdown();
    
    bool isCollected(uint8_t itemId) const;
    void setCollected(uint8_t itemId, bool collected);
    void toggleCollected(uint8_t itemId);
    
    void showChecklist();
    void hideChecklist();
    bool isVisible() const { return mVisible; }
    
    // Called when an item is collected in-game
    void onItemCollected(uint8_t itemId);
    
    // Refresh the UI to reflect current game state
    void refresh();
    
    const ItemInfo* getItemInfo(uint8_t itemId) const;
    std::vector<const ItemInfo*> getItemsByCategory(const std::string& category) const;
    
private:
    ItemChecklist() = default;
    ~ItemChecklist() = default;
    
    void loadItemDefinitions();
    void createDocument();
    void updateProgress();
    
    std::unordered_map<uint8_t, bool> mCollectedItems;
    std::vector<ItemInfo> mItemDefinitions;
    std::unordered_map<uint8_t, size_t> mItemMap;
    
    std::unique_ptr<dusk::ui::Document> mDocument;
    bool mVisible = false;
    bool mInitialized = false;
};

#endif // DUSK_ITEM_CHECKLIST_H