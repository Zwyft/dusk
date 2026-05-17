#pragma once

#include "dusk/ui/document.hpp"

namespace dusk::ui {

class ItemChecklistDocument : public Document {
public:
    struct ItemElementData {
        uint8_t itemId;
        Rml::Element* element;
    };
    
    ItemChecklistDocument();
    
    void Initialize(const Rml::String& source) override;
    void update() override;
    
    void refresh();
    void setCollected(uint8_t itemId, bool collected);
    void updateProgress();
    
private:
    void createItemGrid();
    Rml::Element* createItemElement(uint8_t itemId);
    void updateItemVisuals(uint8_t itemId);
    
    std::unordered_map<uint8_t, Rml::Element*> mItemElements;
    Rml::Element* mProgressText = nullptr;
    Rml::Element* mProgressBar = nullptr;
    Rml::Element* mProgressFill = nullptr;
    
    int mTotalItems = 0;
    int mCollectedItems = 0;
};

} // namespace dusk::ui

#endif // DUSK_ITEM_CHECKLIST_DOCUMENT_H