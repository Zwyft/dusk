#include "dusk/mod_manager.hpp"

#include "dusk/logging.h"
#include "dusk/main.h"

#include <algorithm>
#include <fstream>
#include <nlohmann/json.hpp>
#include <set>

namespace dusk::mod_manager {
namespace {

std::filesystem::path mods_dir() {
    // Use the config path's parent or the config path itself
    auto dir = dusk::ConfigPath / "mods";
    if (!std::filesystem::exists(dir.parent_path())) {
        // Fallback: use a directory next to the executable
        return std::filesystem::current_path() / "mods";
    }
    return dir;
}

std::filesystem::path texture_replacements_dir() {
    return dusk::ConfigPath / "texture_replacements";
}

std::filesystem::path enabled_json_path() {
    return mods_dir() / ".enabled.json";
}

std::set<std::string> load_enabled_mods() {
    std::set<std::string> enabled;
    auto path = enabled_json_path();
    if (!std::filesystem::exists(path)) return enabled;

    std::ifstream ifs(path);
    if (!ifs.is_open()) return enabled;

    try {
        auto j = nlohmann::json::parse(ifs);
        if (j.is_array()) {
            for (auto& item : j) {
                if (item.is_string()) enabled.insert(item.get<std::string>());
            }
        }
    }
    catch (const nlohmann::json::parse_error&) {
        DuskLog.warn("Failed to parse enabled mods: {}", path.string());
    }
    return enabled;
}

void save_enabled_mods(const std::set<std::string>& enabled) {
    auto path = enabled_json_path();
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);

    nlohmann::json j = nlohmann::json::array();
    for (auto& id : enabled) j.push_back(id);

    std::ofstream ofs(path);
    if (ofs.is_open()) ofs << j.dump(2);
}

bool read_mod_json(const std::filesystem::path& jsonPath, ModInfo& info) {
    if (!std::filesystem::exists(jsonPath)) return false;

    std::ifstream ifs(jsonPath);
    if (!ifs.is_open()) return false;

    try {
        auto j = nlohmann::json::parse(ifs);
        info.name = j.value("name", info.id);
        info.version = j.value("version", std::string{"0.0.0"});
        info.author = j.value("author", std::string{});
        info.description = j.value("description", std::string{});
        return true;
    }
    catch (const nlohmann::json::parse_error&) {
        return false;
    }
}

uint32_t count_textures(const std::filesystem::path& dir) {
    uint32_t count = 0;
    std::error_code ec;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(dir, ec)) {
        if (ec) break;
        if (entry.is_regular_file() && entry.path().extension() == ".dds") {
            ++count;
        }
    }
    return count;
}

bool remove_mod_links(const std::string& modId) {
    auto targetDir = texture_replacements_dir();
    if (!std::filesystem::exists(targetDir)) return true;

    std::error_code ec;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(targetDir, ec)) {
        if (ec) return false;
        if (!entry.is_symlink()) continue;

        auto target = std::filesystem::read_symlink(entry.path(), ec);
        if (ec) continue;

        // Check if target path contains "/mods/{modId}/"
        auto targetStr = target.string();
        std::string marker = "/mods/" + modId + "/";
#ifdef _WIN32
        marker = "\\mods\\" + modId + "\\";
#endif
        if (targetStr.find(marker) != std::string::npos) {
            std::filesystem::remove(entry.path(), ec);
        }
    }
    return true;
}

bool create_mod_links(const ModInfo& mod) {
    auto sourceDir = mod.dirPath;
    auto targetDir = texture_replacements_dir();

    std::error_code rootEc;
    std::filesystem::create_directories(targetDir, rootEc);

    bool anySucceeded = false;
    std::error_code iterEc;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(sourceDir, iterEc)) {
        if (iterEc) break;
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() != ".dds") continue;

        std::error_code relEc;
        auto relative = std::filesystem::relative(entry.path(), sourceDir, relEc);
        if (relEc) continue;

        auto target = targetDir / relative;

        std::error_code dirEc;
        std::filesystem::create_directories(target.parent_path(), dirEc);

        std::error_code rmEc;
        if (std::filesystem::exists(target)) {
            std::filesystem::remove(target, rmEc);
        }

        std::error_code symEc;
        std::filesystem::create_symlink(entry.path(), target, symEc);
        if (symEc) {
            // Fallback: copy
            std::error_code copyEc;
            std::filesystem::copy_file(entry.path(), target,
                std::filesystem::copy_options::overwrite_existing, copyEc);
        }
        anySucceeded = true;
    }
    return anySucceeded || mod.textureCount == 0;
}

} // namespace

std::vector<ModInfo> scan_mods() {
    std::vector<ModInfo> mods;
    auto dir = mods_dir();

    std::error_code ec;
    if (!std::filesystem::exists(dir)) {
        std::filesystem::create_directories(dir, ec);
        return mods;
    }

    auto enabledSet = load_enabled_mods();

    for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
        if (ec) break;
        if (!entry.is_directory()) continue;

        ModInfo info;
        info.id = entry.path().filename().string();
        info.dirPath = entry.path();

        // Skip hidden/system directories
        if (!info.id.empty() && info.id[0] == '.') continue;

        auto jsonPath = entry.path() / "mod.json";
        info.valid = read_mod_json(jsonPath, info);
        info.textureCount = count_textures(entry.path());
        info.enabled = enabledSet.count(info.id) > 0;

        if (info.valid) {
            mods.push_back(std::move(info));
        }
    }

    std::sort(mods.begin(), mods.end(),
              [](const ModInfo& a, const ModInfo& b) { return a.name < b.name; });
    return mods;
}

bool enable_mod(const ModInfo& mod) {
    if (!mod.valid) return false;
    if (!create_mod_links(mod)) return false;

    auto enabled = load_enabled_mods();
    enabled.insert(mod.id);
    save_enabled_mods(enabled);
    return true;
}

bool disable_mod(const ModInfo& mod) {
    if (!mod.valid) return false;
    remove_mod_links(mod.id);

    auto enabled = load_enabled_mods();
    enabled.erase(mod.id);
    save_enabled_mods(enabled);
    return true;
}

bool toggle_mod(const ModInfo& mod) {
    if (mod.enabled) {
        return disable_mod(mod);
    }
    return enable_mod(mod);
}

void open_mods_folder() {
    auto dir = mods_dir();
    std::error_code ec;
    if (!std::filesystem::exists(dir)) {
        std::filesystem::create_directories(dir, ec);
    }
#ifdef __ANDROID__
    // On Android, we can't open folders — just log it
    DuskLog.info("Mods directory: {}", dir.string());
#elif _WIN32
    auto cmd = "explorer \"" + dir.string() + "\"";
    std::system(cmd.c_str());
#elif __APPLE__
    auto cmd = "open \"" + dir.string() + "\"";
    std::system(cmd.c_str());
#else
    auto cmd = "xdg-open \"" + dir.string() + "\" &";
    std::system(cmd.c_str());
#endif
}

void refresh_all() {
    auto mods = scan_mods();
    for (auto& mod : mods) {
        if (mod.enabled) {
            remove_mod_links(mod.id);
            create_mod_links(mod);
        } else {
            remove_mod_links(mod.id);
        }
    }
}

void initialize() {
    std::error_code ec;
    std::filesystem::create_directories(mods_dir(), ec);
    std::filesystem::create_directories(texture_replacements_dir(), ec);
    refresh_all();
}

} // namespace dusk::mod_manager
