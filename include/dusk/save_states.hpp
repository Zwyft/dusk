#pragma once

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
    bool isFullState = false;
};

struct QuickSaveSlot {
    std::string encoded;
    std::string stageName;
    int8_t roomNo = 0;
    std::chrono::system_clock::time_point timestamp;
    bool occupied = false;
    bool isFullState = false;
};

class SaveStates {
public:
    void tick();

    void quickSave(int slot);
    bool quickLoad(int slot);
    bool hasQuickSave(int slot) const;
    std::string quickSaveInfo(int slot) const;

    void quickSaveFull(int slot);
    bool quickLoadFull(int slot);

    const std::vector<SavedStateEntry>& getNamedStates() const { return m_states; }
    const QuickSaveSlot* getQuickSaves() const { return m_quickSaves; }
    std::string getStatusMsg() const { return m_statusMsg; }

    void loadStatesFile();
    void saveStatesFile();
    void loadQuickSaves();
    void saveQuickSaves();
    void mergeFromFile(const std::string& path);
    void deleteNamedState(int index);
    void deleteQuickSave(int slot);
    void addNamedState(const std::string& name, const std::string& encoded, bool isFullState = false);
    void saveNamedState(const std::string& name);
    void clearAllNamedStates();
    bool applyEncodedState(const std::string& encoded, const std::string& name = {});

    void setStatusMsg(const std::string& msg) { m_statusMsg = msg; }

private:
    std::string encodeCurrentState();
    std::string encodeCurrentStateSaveOnly();
    void tickPendingApply();

    static void onMergeFileSelected(void* userdata, const char* path, const char* error);
    static bool ValidateEncodedState(const std::string& encoded);

    std::vector<SavedStateEntry> m_states;
    std::string m_statusMsg;
    std::optional<dSv_info_c>  m_pendingInfo;
    std::optional<dSv_save_c>  m_pendingSavedata;
    bool m_loaded = false;
    bool m_stateSharePeekSeen = false;
    std::string m_pendingMergePath;

    QuickSaveSlot m_quickSaves[4] = {};
    bool m_quickSavesLoaded = false;
};

SaveStates& getSaveStates();

} // namespace dusk