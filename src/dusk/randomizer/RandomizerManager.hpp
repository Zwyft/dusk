#ifndef DUSK_RANDOMIZER_MANAGER_HPP
#define DUSK_RANDOMIZER_MANAGER_HPP

#include <dolphin/types.h>
#include <string>
#include <vector>
#include <map>
#include <filesystem>

namespace dusk::randomizer {

struct Location {
    std::string name;
    std::string stage;
    u8 room;
    u32 id; // e.g. Tbox number or NPC ID
    u8 type; // 0 = Tbox, 1 = NPC/Event
};

class RandomizerManager {
public:
    static RandomizerManager& instance();

    void init();
    void generateSeed(const std::string& seedStr);
    
    // Returns the randomized item ID for a given location.
    // If randomizer is disabled or location is not randomized, returns 0xFF.
    u8 getItemAtLocation(const char* stage, u8 room, u32 id, u8 type);

    bool isEnabled() const;
    void saveMapping(const std::filesystem::path& path);
    void loadMapping(const std::filesystem::path& path);

private:
    RandomizerManager() = default;

    bool m_enabled = false;
    std::string m_seed;
    std::map<std::string, u8> m_mapping; // "stage:room:type:id" -> randomizedItemNo
};

} // namespace dusk::randomizer

#endif // DUSK_RANDOMIZER_MANAGER_HPP
