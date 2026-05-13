#include "dusk/save_states.hpp"
#include "dusk/io.hpp"
#include "dusk/logging.h"
#include "dusk/settings.h"
#include "dusk/main.h"
#include "d/d_com_inf_game.h"
#include "f_op/f_op_overlap_mng.h"

#include <absl/strings/escaping.h>
#include <nlohmann/json.hpp>

#include <filesystem>
#include <unordered_set>
#include <zstd.h>

namespace dusk {

using json = nlohmann::json;

#pragma pack(push, 1)
struct StateSharePacket {
    char    stageName[8];
    int8_t  roomNo;
    int8_t  layer;
    int16_t startPoint;
};
#pragma pack(pop)

static constexpr size_t PACKET_TOTAL     = sizeof(StateSharePacket) + sizeof(dSv_info_c);
static constexpr size_t PACKET_SAVE_ONLY = sizeof(StateSharePacket) + sizeof(dSv_save_c);
static constexpr auto STATES_FILENAME = "states.json";
static constexpr auto QUICKSAVE_PREFIX = "quicksave_";

static std::filesystem::path GetQuickSaveFilePath(int slot) {
    return dusk::ConfigPath / fmt::format("{}{}.json", QUICKSAVE_PREFIX, slot);
}

static std::filesystem::path GetStatesFilePath() {
    return dusk::ConfigPath / STATES_FILENAME;
}

void SaveStates::onMergeFileSelected(void* userdata, const char* path, const char*) {
    auto* self = static_cast<SaveStates*>(userdata);
    if (path != nullptr) {
        self->m_pendingMergePath = path;
    }
}

void SaveStates::loadQuickSaves() {
    m_quickSavesLoaded = true;
    for (int i = 0; i < 4; ++i) {
        auto& slot = m_quickSaves[i];
        const auto path = GetQuickSaveFilePath(i);
        slot.occupied = false;
        if (!std::filesystem::exists(path)) {
            continue;
        }
        try {
            auto data = io::FileStream::ReadAllBytes(path);
            auto j = json::parse(data);
            if (!j.is_object() || !j.contains("data")) {
                continue;
            }
            slot.encoded = j["data"].get<std::string>();
            if (!ValidateEncodedState(slot.encoded)) {
                slot.occupied = false;
                continue;
            }
            slot.occupied = true;
            if (j.contains("stage")) slot.stageName = j["stage"].get<std::string>();
            if (j.contains("room")) slot.roomNo = j["room"].get<int8_t>();
            if (j.contains("timestamp")) {
                auto ts = j["timestamp"].get<int64_t>();
                slot.timestamp = std::chrono::system_clock::time_point(std::chrono::seconds(ts));
            }
        } catch (...) {
            slot.occupied = false;
        }
    }
}

void SaveStates::saveQuickSaves() {
    for (int i = 0; i < 4; ++i) {
        const auto& slot = m_quickSaves[i];
        const auto path = GetQuickSaveFilePath(i);
        if (!slot.occupied) {
            std::error_code ec;
            std::filesystem::remove(path, ec);
            continue;
        }
        json j;
        j["data"] = slot.encoded;
        j["stage"] = slot.stageName;
        j["room"] = slot.roomNo;
        j["timestamp"] = std::chrono::duration_cast<std::chrono::seconds>(
            slot.timestamp.time_since_epoch()).count();
        try {
            io::FileStream::WriteAllText(path, j.dump(2));
        } catch (...) {
        }
    }
}

void SaveStates::quickSave(int slot) {
    if (slot < 0 || slot >= 4 || !dusk::IsGameLaunched) return;
    if (dusk::getTransientSettings().stateShareLoadActive) return;

    auto& entry = m_quickSaves[slot];
    entry.encoded = encodeCurrentState();
    entry.stageName = dComIfGp_getStartStageName();
    entry.roomNo = dComIfGp_getStartStageRoomNo();
    entry.timestamp = std::chrono::system_clock::now();
    entry.occupied = true;
    entry.isFullState = false;
    saveQuickSaves();
    m_statusMsg = fmt::format("Quick save {} saved.", slot + 1);
}

bool SaveStates::quickLoad(int slot) {
    if (slot < 0 || slot >= 4) return false;
    if (!m_quickSaves[slot].occupied) return false;
    if (dusk::getTransientSettings().stateShareLoadActive) return false;

    bool ok = applyEncodedState(m_quickSaves[slot].encoded,
                                fmt::format("Quick Save {}", slot + 1));
    if (ok) {
        m_statusMsg = fmt::format("Loaded quick save {}.", slot + 1);
    }
    return ok;
}

bool SaveStates::hasQuickSave(int slot) const {
    return slot >= 0 && slot < 4 && m_quickSaves[slot].occupied;
}

std::string SaveStates::quickSaveInfo(int slot) const {
    if (slot < 0 || slot >= 4 || !m_quickSaves[slot].occupied) return "";
    const auto& s = m_quickSaves[slot];
    auto now = std::chrono::system_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::seconds>(now - s.timestamp).count();
    std::string timeStr;
    if (diff < 60) timeStr = fmt::format("{}s ago", diff);
    else if (diff < 3600) timeStr = fmt::format("{}m ago", diff / 60);
    else if (diff < 86400) timeStr = fmt::format("{}h ago", diff / 3600);
    else timeStr = fmt::format("{}d ago", diff / 86400);
    std::string typeTag = s.isFullState ? "[FULL] " : "";
    return fmt::format("{}{} R{} ({})", typeTag, s.stageName, (int)s.roomNo, timeStr);
}

void SaveStates::quickSaveFull(int slot) {
    if (slot < 0 || slot >= 4 || !dusk::IsGameLaunched) return;
    if (dusk::getTransientSettings().stateShareLoadActive) return;

    auto& entry = m_quickSaves[slot];
    auto stateData = save_state::captureState();

    size_t bound = ZSTD_compressBound(stateData.size());
    std::string compressed(bound, '\0');
    compressed.resize(ZSTD_compress(compressed.data(), bound, stateData.data(), stateData.size(), 1));
    entry.encoded = absl::Base64Escape(compressed);
    entry.stageName = dComIfGp_getStartStageName();
    entry.roomNo = dComIfGp_getStartStageRoomNo();
    entry.timestamp = std::chrono::system_clock::now();
    entry.occupied = true;
    entry.isFullState = true;
    saveQuickSaves();
    m_statusMsg = fmt::format("Full quick save {} saved.", slot + 1);
}

bool SaveStates::quickLoadFull(int slot) {
    if (slot < 0 || slot >= 4) return false;
    if (!m_quickSaves[slot].occupied) return false;
    if (!m_quickSaves[slot].isFullState) return false;
    if (dusk::getTransientSettings().stateShareLoadActive) return false;

    std::string decoded;
    if (!absl::Base64Unescape(m_quickSaves[slot].encoded, &decoded)) {
        m_statusMsg = "Invalid base64.";
        return false;
    }

    unsigned long long dSize = ZSTD_getFrameContentSize(decoded.data(), decoded.size());
    if (dSize == ZSTD_CONTENTSIZE_ERROR || dSize == ZSTD_CONTENTSIZE_UNKNOWN) {
        m_statusMsg = "Not a valid state string.";
        return false;
    }

    std::vector<uint8_t> stateData(dSize);
    size_t result = ZSTD_decompress(stateData.data(), stateData.size(), decoded.data(), decoded.size());
    if (ZSTD_isError(result)) {
        m_statusMsg = fmt::format("Decompression failed: {}", ZSTD_getErrorName(result));
        return false;
    }

    save_state::restoreState(stateData);
    m_statusMsg = fmt::format("Loaded full state from slot {}.", slot + 1);
    return true;
}

void SaveStates::tick() {
    if (!m_quickSavesLoaded) {
        loadQuickSaves();
    }

    if (!m_loaded) {
        loadStatesFile();
    }

    if (!m_pendingMergePath.empty()) {
        mergeFromFile(m_pendingMergePath);
        m_pendingMergePath.clear();
    }

    tickPendingApply();
}

std::string SaveStates::encodeCurrentState() {
    StateSharePacket pkt = {};
    strncpy(pkt.stageName, dComIfGp_getStartStageName(), 7);
    pkt.roomNo     = dComIfGp_getStartStageRoomNo();
    pkt.layer      = dComIfGp_getStartStageLayer();
    pkt.startPoint = dComIfGp_getStartStagePoint();

    std::string raw(PACKET_TOTAL, '\0');
    memcpy(raw.data(), &pkt, sizeof(pkt));
    memcpy(raw.data() + sizeof(pkt), &g_dComIfG_gameInfo.info, sizeof(dSv_info_c));

    size_t bound = ZSTD_compressBound(raw.size());
    std::string compressed(bound, '\0');
    compressed.resize(ZSTD_compress(compressed.data(), bound, raw.data(), raw.size(), 1));

    return absl::Base64Escape(compressed);
}

bool SaveStates::applyEncodedState(const std::string& encoded, const std::string& name) {
    std::string decoded;
    if (!absl::Base64Unescape(encoded, &decoded)) {
        m_statusMsg = "Invalid base64.";
        return false;
    }

    unsigned long long dSize = ZSTD_getFrameContentSize(decoded.data(), decoded.size());
    if (dSize == ZSTD_CONTENTSIZE_ERROR || dSize == ZSTD_CONTENTSIZE_UNKNOWN) {
        m_statusMsg = "Not a valid state string.";
        return false;
    }

    const bool isFull    = (dSize == PACKET_TOTAL);
    const bool isPartial = (dSize == PACKET_SAVE_ONLY);
    if (!isFull && !isPartial) {
        m_statusMsg = "Not a valid state string.";
        return false;
    }

    std::string raw(static_cast<size_t>(dSize), '\0');
    size_t result = ZSTD_decompress(raw.data(), raw.size(), decoded.data(), decoded.size());
    if (ZSTD_isError(result)) {
        m_statusMsg = fmt::format("Decompression failed: {}", ZSTD_getErrorName(result));
        return false;
    }

    StateSharePacket pkt;
    memcpy(&pkt, raw.data(), sizeof(pkt));
    pkt.stageName[7] = '\0';

    if (isFull) {
        memcpy(&g_dComIfG_gameInfo.info, raw.data() + sizeof(pkt), sizeof(dSv_info_c));
        m_pendingInfo = g_dComIfG_gameInfo.info;
        m_pendingSavedata.reset();
    } else {
        memcpy(&g_dComIfG_gameInfo.info.mSavedata, raw.data() + sizeof(pkt), sizeof(dSv_save_c));
        m_pendingSavedata = g_dComIfG_gameInfo.info.mSavedata;
        m_pendingInfo.reset();
    }

    s16 spawnPoint = pkt.startPoint == -4 ? -1 : pkt.startPoint;
    if (spawnPoint == -1) {
        dComIfGs_setRestartRoomParam(pkt.roomNo & 0x3F);
    }

    dusk::getTransientSettings().stateShareLoadActive = true;
    m_stateSharePeekSeen = false;
    dComIfGp_setNextStage(pkt.stageName, spawnPoint, pkt.roomNo, pkt.layer, 0.0f, 0, 1, 0, 0, 1, 3);

    if (name.empty()) {
        m_statusMsg = fmt::format("{} room {} layer {}.", pkt.stageName, (int)pkt.roomNo, (int)pkt.layer);
    } else {
        m_statusMsg = fmt::format("{}: {} room {} layer {}.", name, pkt.stageName, (int)pkt.roomNo, (int)pkt.layer);
    }
    return true;
}

void SaveStates::tickPendingApply() {
    if (!m_pendingInfo.has_value() && !m_pendingSavedata.has_value()) {
        return;
    }
    if (dComIfGp_isEnableNextStage()) {
        return;
    }
    if (m_pendingInfo.has_value()) {
        g_dComIfG_gameInfo.info = *m_pendingInfo;
        m_pendingInfo.reset();
    } else {
        g_dComIfG_gameInfo.info.mSavedata = *m_pendingSavedata;
        m_pendingSavedata.reset();
    }
    dComIfGp_offOxygenShowFlag();
    dComIfGp_setMaxOxygen(600);
    dComIfGp_setOxygen(600);
}

bool SaveStates::ValidateEncodedState(const std::string& encoded) {
    std::string decoded;
    if (!absl::Base64Unescape(encoded, &decoded)) {
        return false;
    }
    unsigned long long dSize = ZSTD_getFrameContentSize(decoded.data(), decoded.size());
    return dSize == PACKET_TOTAL || dSize == PACKET_SAVE_ONLY;
}

void SaveStates::loadStatesFile() {
    m_loaded = true;
    const std::filesystem::path filePath = GetStatesFilePath();
    if (!std::filesystem::exists(filePath)) {
        return;
    }
    try {
        auto data = io::FileStream::ReadAllBytes(filePath);
        auto j = json::parse(data);
        if (!j.is_array()) {
            return;
        }
        for (const auto& entry : j) {
            if (!entry.contains("name") || !entry.contains("data")) {
                continue;
            }
            SavedStateEntry s;
            s.name    = entry["name"].get<std::string>();
            s.encoded = entry["data"].get<std::string>();
            m_states.push_back(std::move(s));
        }
    } catch (const std::exception& e) {
        m_statusMsg = fmt::format("Failed to load states: {}", e.what());
    }
}

void SaveStates::saveStatesFile() {
    json j = json::array();
    for (const auto& s : m_states) {
        j.push_back(json{{"name", s.name}, {"data", s.encoded}});
    }
    try {
        io::FileStream::WriteAllText(GetStatesFilePath(), j.dump(2));
    } catch (const std::exception& e) {
        m_statusMsg = fmt::format("Failed to save states: {}", e.what());
    }
}

void SaveStates::mergeFromFile(const std::string& path) {
    try {
        auto data = io::FileStream::ReadAllBytes(path.c_str());
        auto j = json::parse(data);
        if (!j.is_array()) {
            m_statusMsg = "File does not contain a JSON array.";
            return;
        }

        std::unordered_set<std::string> existingNames;
        for (const auto& s : m_states) {
            existingNames.insert(s.name);
        }

        int added   = 0;
        int skipped = 0;
        for (const auto& entry : j) {
            if (!entry.contains("name") || !entry.contains("data")) {
                ++skipped;
                continue;
            }
            const std::string name    = entry["name"].get<std::string>();
            const std::string encoded = entry["data"].get<std::string>();
            if (!ValidateEncodedState(encoded)) {
                ++skipped;
                continue;
            }
            if (existingNames.count(name)) {
                ++skipped;
                continue;
            }
            SavedStateEntry s;
            s.name    = name;
            s.encoded = encoded;
            existingNames.insert(s.name);
            m_states.push_back(std::move(s));
            ++added;
        }

        if (added > 0) {
            saveStatesFile();
        }
        m_statusMsg = fmt::format("Merged: {} added, {} skipped.", added, skipped);
    } catch (const std::exception& e) {
        m_statusMsg = fmt::format("Failed to load file: {}", e.what());
    }
}

void SaveStates::deleteNamedState(int index) {
    if (index >= 0 && index < (int)m_states.size()) {
        m_states.erase(m_states.begin() + index);
        saveStatesFile();
    }
}

void SaveStates::clearAllNamedStates() {
    m_states.clear();
    saveStatesFile();
    m_statusMsg = "All states cleared.";
}

static SaveStates g_saveStates;

SaveStates& getSaveStates() {
    return g_saveStates;
}

} // namespace dusk