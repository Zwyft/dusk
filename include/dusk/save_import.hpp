#pragma once

#include <filesystem>

namespace dusk::save_import {

std::filesystem::path saves_dir();
std::filesystem::path detect_dolphin_saves();
bool can_open_saves_dir();
void open_saves_dir();
bool import_from_dolphin();

} // namespace dusk::save_import
