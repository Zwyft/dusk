#include "RandomizerManager.hpp"
#include "dusk/settings.h"
#include "dusk/logging.h"
#include "fmt/format.h"
#include "nlohmann/json.hpp"
#include "dusk/io.hpp"
#include <algorithm>
#include <random>

namespace dusk::randomizer {

RandomizerManager& RandomizerManager::instance() {
    static RandomizerManager s_instance;
    return s_instance;
}

void RandomizerManager::init() {
    m_enabled = getSettings().randomizer.enabled.getValue();
    m_seed = getSettings().randomizer.seed.getValue();
    
    if (m_enabled && m_mapping.empty()) {
        // For now, if no mapping exists, generate a basic one if a seed is set
        if (!m_seed.empty()) {
            generateSeed(m_seed);
        }
    }
}

void RandomizerManager::generateSeed(const std::string& seedStr) {
    DuskLog.info("Generating randomized seed: {}", seedStr);
    m_mapping.clear();

    // Load locations from JSON
    std::vector<Location> locations;
    try {
        auto data = dusk::io::FileStream::ReadAllBytes("res/randomizer_locations.json");
        nlohmann::json j = nlohmann::json::parse(data);
        for (const auto& item : j) {
            locations.push_back({
                item["name"].get<std::string>(),
                item["stage"].get<std::string>(),
                item["room"].get<u8>(),
                item["id"].get<u32>(),
                item["type"].get<u8>()
            });
        }
    } catch (const std::exception& e) {
        DuskLog.error("Failed to load randomizer locations: {}", e.what());
        return;
    }

    std::seed_seq seed(seedStr.begin(), seedStr.end());
    std::mt19937 g(seed);

    std::vector<u8> itemPool;
    // For now, we just shuffle the original items back into the locations
    // In a full implementation, we'd have a separate item pool
    try {
        auto data = dusk::io::FileStream::ReadAllBytes("res/randomizer_locations.json");
        nlohmann::json j = nlohmann::json::parse(data);
        for (const auto& item : j) {
            itemPool.push_back(item["originalItem"].get<u8>());
        }
    } catch (...) {}

    std::shuffle(itemPool.begin(), itemPool.end(), g);

    for (size_t i = 0; i < locations.size(); ++i) {
        std::string key = fmt::format("{}:{}:{}:{}", locations[i].stage, locations[i].room, locations[i].type, locations[i].id);
        m_mapping[key] = itemPool[i];
        DuskLog.debug("Mapped {} -> item 0x{:02X}", key, itemPool[i]);
    }
}

u8 RandomizerManager::getItemAtLocation(const char* stage, u8 room, u32 id, u8 type) {
    if (!m_enabled) return 0xFF;

    std::string key = fmt::format("{}:{}:{}:{}", stage, room, type, id);
    auto it = m_mapping.find(key);
    if (it != m_mapping.end()) {
        return it->second;
    }

    return 0xFF;
}

bool RandomizerManager::isEnabled() const {
    return m_enabled;
}

void RandomizerManager::saveMapping(const std::filesystem::path& path) {
    nlohmann::json j;
    j["seed"] = m_seed;
    j["mapping"] = m_mapping;
    
    try {
        dusk::io::FileStream::WriteAllText(path, j.dump(4));
    } catch (const std::exception& e) {
        DuskLog.error("Failed to save randomizer mapping: {}", e.what());
    }
}

void RandomizerManager::loadMapping(const std::filesystem::path& path) {
    try {
        auto data = dusk::io::FileStream::ReadAllBytes(path);
        nlohmann::json j = nlohmann::json::parse(data);
        
        m_seed = j["seed"].get<std::string>();
        m_mapping = j["mapping"].get<std::map<std::string, u8>>();
        m_enabled = true;
    } catch (const std::exception& e) {
        DuskLog.error("Failed to load randomizer mapping: {}", e.what());
    }
}

} // namespace dusk::randomizer
