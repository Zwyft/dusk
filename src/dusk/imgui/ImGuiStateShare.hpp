#ifndef DUSK_IMGUI_STATESHARE_HPP
#define DUSK_IMGUI_STATESHARE_HPP

#include "d/d_save.h"
#include "dusk/save_state.hpp"
#include <chrono>
#include <optional>
#include <string>
#include <vector>

namespace dusk {

struct SavedStateEntry {
    std::string name;
    std::string encoded;
    bool isFullState = false; // true = actor snapshot, false = stage reload
};

struct QuickSaveSlot {
    std::string encoded;
    std::string stageName;
    int8_t roomNo = 0;
    std::chrono::system_clock::time_point timestamp;
    bool occupied = false;
    bool isFullState = false;
};

class ImGuiStateShare {
public:
    void draw(bool& open);
    void tick();

    void quickSave(int slot);
    bool quickLoad(int slot);
    bool hasQuickSave(int slot) const;
    std::string quickSaveInfo(int slot) const;

    void quickSaveFull(int slot);
    bool quickLoadFull(int slot);

private:
    std::string encodeCurrentState();
    bool applyEncodedState(const std::string& encoded, const std::string& name = {});
    void tickPendingApply();
    void loadStatesFile();
    void saveStatesFile();
    void mergeFromFile(const std::string& path);
    static void onMergeFileSelected(void* userdata, const char* path, const char* error);

    void loadQuickSaves();
    void saveQuickSaves();

    std::vector<SavedStateEntry> m_states;
    std::string m_statusMsg;
    std::optional<dSv_info_c>  m_pendingInfo;
    std::optional<dSv_save_c>  m_pendingSavedata;
    int m_renamingIndex = -1;
    char m_renameBuffer[128] = {};
    bool m_loaded = false;
    bool m_stateSharePeekSeen = false;
    std::string m_pendingMergePath;

    QuickSaveSlot m_quickSaves[4] = {};
    bool m_quickSavesLoaded = false;
    bool m_showQuickMenu = false;
};

}

#endif
