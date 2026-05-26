#pragma once

#include <SDL3/SDL_dialog.h>

#include <string>
#include <string_view>

struct SDL_Window;

namespace dusk {

using FileCallback = void (*)(void* userdata, const char* path, const char* error);

void ShowFileSelect(FileCallback callback, void* userdata, SDL_Window* window,
    const SDL_DialogFileFilter* filters, int nfilters, const char* default_location,
    bool allow_many);

std::string display_name_for_path(std::string_view path);

#if defined(__ANDROID__) || defined(ANDROID)
std::string resolve_content_uri(std::string_view path);
std::string resolve_content_uri_for_playback(std::string_view path);
#else
inline std::string resolve_content_uri(std::string_view path) { return std::string(path); }
inline std::string resolve_content_uri_for_playback(std::string_view path) { return std::string(path); }
#endif

}  // namespace dusk
