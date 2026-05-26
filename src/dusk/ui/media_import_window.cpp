#include "media_import_window.hpp"

#include "button.hpp"
#include "dusk/file_select.hpp"
#include "dusk/logging.h"
#include "dusk/main.h"
#include "dusk/ui/ui.hpp"
#include "pane.hpp"

#include <SDL3/SDL_clipboard.h>
#include <curl/curl.h>
#include <fmt/format.h>

#include <cstdio>
#include <filesystem>
#include <algorithm>
#include <cctype>
#include <chrono>

namespace dusk::ui {
namespace {

std::filesystem::path media_dir() {
    const auto dir = dusk::ConfigPath / "game_media";
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    return dir;
}

bool is_allowed_image_ext(const std::filesystem::path& path) {
    auto ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return (char)std::tolower(c); });
    return ext == ".rvz" || ext == ".iso";
}

std::string trim_url(std::string value) {
    while (!value.empty() && (value.back() == '\n' || value.back() == '\r' || value.back() == ' ' || value.back() == '\t')) {
        value.pop_back();
    }
    while (!value.empty() && (value.front() == ' ' || value.front() == '\t')) {
        value.erase(value.begin());
    }
    return value;
}

std::string filename_from_url(const std::string& url) {
    const auto slash = url.find_last_of('/');
    std::string file = slash == std::string::npos ? "download.rvz" : url.substr(slash + 1);
    const auto q = file.find('?');
    if (q != std::string::npos) {
        file = file.substr(0, q);
    }
    if (file.empty()) {
        file = "download.rvz";
    }
    return file;
}

size_t write_to_file(void* ptr, size_t size, size_t nmemb, void* userdata) {
    return std::fwrite(ptr, size, nmemb, static_cast<FILE*>(userdata));
}

bool download_to_file(const std::string& url, const std::filesystem::path& outPath, std::string& error) {
    CURL* curl = curl_easy_init();
    if (curl == nullptr) {
        error = "curl init failed";
        return false;
    }

    FILE* fp = std::fopen(outPath.string().c_str(), "wb");
    if (fp == nullptr) {
        curl_easy_cleanup(curl);
        error = "cannot open destination file";
        return false;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_file);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

    const CURLcode code = curl_easy_perform(curl);
    long status = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);

    std::fclose(fp);
    curl_easy_cleanup(curl);

    if (code != CURLE_OK) {
        error = curl_easy_strerror(code);
        std::filesystem::remove(outPath);
        return false;
    }
    if (status < 200 || status >= 300) {
        error = fmt::format("http {}", status);
        std::filesystem::remove(outPath);
        return false;
    }
    return true;
}

std::string normalize_google_drive_url(const std::string& url) {
    if (url.find("drive.google.com") == std::string::npos) {
        return {};
    }

    auto idPos = url.find("/file/d/");
    if (idPos != std::string::npos) {
        idPos += 8;
        const auto end = url.find('/', idPos);
        const auto id = url.substr(idPos, end == std::string::npos ? std::string::npos : end - idPos);
        if (!id.empty()) {
            return fmt::format("https://drive.google.com/uc?export=download&id={}", id);
        }
    }

    idPos = url.find("id=");
    if (idPos != std::string::npos) {
        idPos += 3;
        const auto end = url.find('&', idPos);
        const auto id = url.substr(idPos, end == std::string::npos ? std::string::npos : end - idPos);
        if (!id.empty()) {
            return fmt::format("https://drive.google.com/uc?export=download&id={}", id);
        }
    }

    return {};
}

void show_toast(const char* title, const std::string& text, const char* type = "info") {
    push_toast(Toast{
        .type = type,
        .title = title,
        .content = text,
        .duration = std::chrono::seconds(6),
    });
}

void import_local_file(const char* path) {
    if (path == nullptr || path[0] == '\0') {
        show_toast("Media Import", "No file selected.", "warning");
        return;
    }

    const std::filesystem::path src = dusk::resolve_content_uri(path);
    if (!is_allowed_image_ext(src)) {
        show_toast("Media Import", "Only .rvz and .iso are accepted.", "danger");
        return;
    }

    const auto dest = media_dir() / src.filename();
    std::error_code ec;
    std::filesystem::copy_file(src, dest, std::filesystem::copy_options::overwrite_existing, ec);
    if (ec) {
        show_toast("Media Import", fmt::format("Copy failed: {}", ec.message()), "danger");
        return;
    }

    show_toast("Media Import", fmt::format("Imported {}", dest.filename().string()), "success");
}

void on_pick_local(void*, const char* path, const char* error) {
    if (error != nullptr && error[0] != '\0') {
        show_toast("Media Import", error, "danger");
        return;
    }
    import_local_file(path);
}

bool download_from_url(const std::string& rawUrl) {
    const std::string url = trim_url(rawUrl);
    if (!(url.rfind("https://", 0) == 0 || url.rfind("http://", 0) == 0)) {
        show_toast("Media Import", "URL must start with http:// or https://", "danger");
        return false;
    }

    const auto filename = filename_from_url(url);
    const auto outPath = media_dir() / filename;
    if (!is_allowed_image_ext(outPath)) {
        show_toast("Media Import", "URL must resolve to .rvz or .iso filename.", "danger");
        return false;
    }

    std::string error;
    if (!download_to_file(url, outPath, error)) {
        show_toast("Media Import", fmt::format("Download failed: {}", error), "danger");
        return false;
    }

    show_toast("Media Import", fmt::format("Downloaded {}", outPath.filename().string()), "success");
    return true;
}

} // namespace

MediaImportWindow::MediaImportWindow(bool allowRemote) : Window(), mAllowRemote(allowRemote) {
    add_tab("Media Import", [this](Rml::Element* content) { build_tab(content); });
}

void MediaImportWindow::build_tab(Rml::Element* content) {
    auto& leftPane = add_child<Pane>(content, Pane::Type::Controlled);
    auto& rightPane = add_child<Pane>(content, Pane::Type::Uncontrolled);

    leftPane.add_section("Option A: Local Import");
    rightPane.add_text("Import your legally obtained .rvz/.iso from local storage.");

    leftPane.register_control(
        leftPane.add_button("Pick Local .rvz/.iso").on_pressed([] {
            mDoAud_seStartMenu(kSoundClick);
            static const SDL_DialogFileFilter filters[] = {
                {"Game Images (*.rvz;*.iso)", "rvz;iso"},
            };
            dusk::ShowFileSelect(on_pick_local, nullptr, nullptr, filters, 1, nullptr, false);
        }),
        rightPane, [](Pane& pane) {
            pane.clear();
            pane.add_text("Pick from device storage, USB, SD card, or mounted path.");
        });

    if (!mAllowRemote) {
        return;
    }

    leftPane.add_rml("<br/>");
    leftPane.add_section("Secret Remote Sources");

    leftPane.register_control(
        leftPane.add_button("Option B: Download Clipboard URL").on_pressed([] {
            mDoAud_seStartMenu(kSoundClick);
            if (!SDL_HasClipboardText()) {
                show_toast("Media Import", "Clipboard has no URL.", "warning");
                return;
            }
            char* text = SDL_GetClipboardText();
            if (text == nullptr || text[0] == '\0') {
                show_toast("Media Import", "Clipboard has no URL.", "warning");
            } else {
                download_from_url(text);
            }
            if (text != nullptr) {
                SDL_free(text);
            }
        }),
        rightPane, [](Pane& pane) {
            pane.clear();
            pane.add_text("Copies from clipboard URL. Allowed extensions: .rvz/.iso");
        });

    leftPane.register_control(
        leftPane.add_button("Option C: Google Drive URL from Clipboard").on_pressed([] {
            mDoAud_seStartMenu(kSoundClick);
            if (!SDL_HasClipboardText()) {
                show_toast("Media Import", "Clipboard has no URL.", "warning");
                return;
            }
            char* text = SDL_GetClipboardText();
            if (text == nullptr || text[0] == '\0') {
                show_toast("Media Import", "Clipboard has no URL.", "warning");
                if (text != nullptr) SDL_free(text);
                return;
            }
            const std::string normalized = normalize_google_drive_url(text);
            if (normalized.empty()) {
                show_toast("Media Import", "Could not parse Google Drive file URL.", "danger");
            } else {
                download_from_url(normalized);
            }
            SDL_free(text);
        }),
        rightPane, [](Pane& pane) {
            pane.clear();
            pane.add_text("Use your own Google Drive file link in clipboard; converted to direct download.");
        });
}

} // namespace dusk::ui
