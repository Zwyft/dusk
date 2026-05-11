#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace dusk::mod_manager {

struct ModInfo {
    std::string id;          // directory name
    std::string name;        // from mod.json
    std::string version;     // from mod.json
    std::string author;      // from mod.json
    std::string description; // from mod.json
    std::filesystem::path dirPath;
    bool enabled = false;
    bool valid = false;
    uint32_t textureCount = 0;
};

// Scan the mods/ directory and return all discovered mods.
std::vector<ModInfo> scan_mods();

// Enable a mod (symlink/copy its textures into texture_replacements/).
bool enable_mod(const ModInfo& mod);

// Disable a mod (remove its symlinks from texture_replacements/).
bool disable_mod(const ModInfo& mod);

// Toggle: enable if disabled, disable if enabled.
bool toggle_mod(const ModInfo& mod);

// Open the mods directory in the system file manager.
void open_mods_folder();

// Refresh all enabled mods (disable all, re-enable based on CVars).
void refresh_all();

// Called once at startup to apply enabled mods.
void initialize();

} // namespace dusk::mod_manager
