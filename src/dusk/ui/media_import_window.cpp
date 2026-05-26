#include "media_import_window.hpp"

#include "button.hpp"
#include "dusk/file_select.hpp"
#include "dusk/logging.h"
#include "dusk/http/http.hpp"
#include "dusk/main.h"
#include "dusk/ui/ui.hpp"
#include "pane.hpp"
#include "string_button.hpp"

#include <SDL3/SDL_clipboard.h>
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

std::string filename_from_url(const std::string& url, const std::string& fallbackFilename = "download.rvz") {
    const auto slash = url.find_last_of('/');
    std::string file = slash == std::string::npos ? "download.rvz" : url.substr(slash + 1);
    const auto q = file.find('?');
    if (q != std::string::npos) {
        file = file.substr(0, q);
    }
    const auto fragment = file.find('#');
    if (fragment != std::string::npos) {
        file = file.substr(0, fragment);
    }
    if (file.empty() || std::filesystem::path(file).extension().empty()) {
        file = fallbackFilename;
    }
    return file;
}

bool download_to_file(const std::string& url, const std::filesystem::path& outPath, std::string& error) {
    dusk::http::Request req{};
    req.url = url;
    req.timeout = std::chrono::seconds(30);
    req.maxBodyBytes = 64 * 1024 * 1024;

    auto result = dusk::http::get(req);
    if (result.error != dusk::http::Error::None) {
        error = result.message.empty() ? "request failed" : result.message;
        return false;
    }

    if (result.response.statusCode < 200 || result.response.statusCode >= 300) {
        error = fmt::format("http {}", result.response.statusCode);
        return false;
    }

    FILE* fp = std::fopen(outPath.string().c_str(), "wb");
    if (fp == nullptr) {
        error = "cannot open destination file";
        return false;
    }
    const size_t written = std::fwrite(result.response.body.data(), 1, result.response.body.size(), fp);
    std::fclose(fp);
    if (written != result.response.body.size()) {
        std::filesystem::remove(outPath);
        error = "short write";
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

bool download_from_url(const std::string& rawUrl, const std::string& fallbackFilename = "download.rvz") {
    const std::string url = trim_url(rawUrl);
    if (!(url.rfind("https://", 0) == 0 || url.rfind("http://", 0) == 0)) {
        show_toast("Media Import", "URL must start with http:// or https://", "danger");
        return false;
    }

    const auto filename = filename_from_url(url, fallbackFilename);
    const auto outPath = media_dir() / filename;
    if (!is_allowed_image_ext(outPath)) {
        show_toast("Media Import", "Download filename must end in .rvz or .iso.", "danger");
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

bool download_google_drive_url(const std::string& rawUrl) {
    const std::string normalized = normalize_google_drive_url(rawUrl);
    if (normalized.empty()) {
        show_toast("Media Import", "Could not parse Google Drive file URL.", "danger");
        return false;
    }
    return download_from_url(normalized, "google-drive-download.rvz");
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
        leftPane.add_child<StringButton>(StringButton::Props{
            .key = "Option B: Enter Download URL",
            .getValue = [this] { return Rml::String(mManualUrl); },
            .setValue = [this](Rml::String value) { mManualUrl = trim_url(std::move(value)); },
            .maxLength = 2048,
        }),
        rightPane, [](Pane& pane) {
            pane.clear();
            pane.add_text("Press A/Enter to type or paste any http:// or https:// .rvz/.iso download link.");
            pane.add_text("Press A/Enter again when done, then choose Download Entered URL.");
        });

    leftPane.register_control(
        leftPane.add_button("Option C: Download Entered URL").on_pressed([this] {
            mDoAud_seStartMenu(kSoundClick);
            if (trim_url(mManualUrl).empty()) {
                show_toast("Media Import", "Enter a URL first.", "warning");
                return;
            }
            download_from_url(mManualUrl);
        }),
        rightPane, [this](Pane& pane) {
            pane.clear();
            pane.add_text("Downloads the URL you typed above. Links with no filename are saved as download.rvz.");
            if (!trim_url(mManualUrl).empty()) {
                pane.add_text(fmt::format("Current URL: {}", trim_url(mManualUrl)));
            }
        });

    leftPane.register_control(
        leftPane.add_button("Option D: Download Clipboard URL").on_pressed([] {
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
        leftPane.add_button("Option E: Google Drive URL from Clipboard").on_pressed([] {
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
            download_google_drive_url(text);
            SDL_free(text);
        }),
        rightPane, [](Pane& pane) {
            pane.clear();
            pane.add_text("Use your own Google Drive file link in clipboard; converted to direct download.");
        });
}

} // namespace dusk::ui
