#!/usr/bin/env python3
"""
Apply out-of-tree patches to the aurora submodule at build time.

Run from the repository root before cmake configuration.
"""

import sys
from pathlib import Path

ROOT = Path(__file__).parent.parent
CARD = ROOT / "extern/aurora/lib/dolphin/card.cpp"
ABSEIL = ROOT / "extern/aurora/extern/CMakeLists.txt"
DAWN_PROVIDER = ROOT / "extern/aurora/cmake/AuroraDawnProvider.cmake"
DAWN_ABSEIL_PATCH = (ROOT / "ci/patch_dawn_abseil_switch.cmake").as_posix()


def apply(path: Path, old: str, new: str, description: str) -> None:
    text = path.read_text()
    if new in text:
        print(f"[patch-aurora] already applied: {description}")
        return
    if old not in text:
        print(f"[patch-aurora] ERROR: anchor not found for: {description}", file=sys.stderr)
        sys.exit(1)
    path.write_text(text.replace(old, new, 1))
    print(f"[patch-aurora] applied: {description}")


# ---------------------------------------------------------------------------
# card.cpp: prefer local saves/ folder in CARDInit
# ---------------------------------------------------------------------------
apply(
    CARD,
    '#include "../card/CardGciFolder.hpp"',
    '#include "../card/CardGciFolder.hpp"\n#include <SDL3/SDL_filesystem.h>',
    "card.cpp: add SDL_filesystem include",
)

apply(
    CARD,
    "  std::filesystem::path cardWorkingDir;\n"
    "  if (aurora::g_config.userPath != nullptr)\n"
    "    cardWorkingDir = reinterpret_cast<const char8_t*>(aurora::g_config.userPath);\n"
    "  else\n"
    "    cardWorkingDir = std::filesystem::current_path();",
    "  std::filesystem::path cardWorkingDir;\n"
    "  {\n"
    "    const char* basePath = SDL_GetBasePath();\n"
    "    if (basePath != nullptr) {\n"
    '      std::filesystem::path localSaves = std::filesystem::path(basePath) / "saves";\n'
    "      SDL_free((void*)basePath);\n"
    "      if (std::filesystem::exists(localSaves)) {\n"
    "        cardWorkingDir = localSaves;\n"
    '        Log.info("Using local saves folder: {}", localSaves.string());\n'
    "      }\n"
    "    }\n"
    "    if (cardWorkingDir.empty()) {\n"
    "      if (aurora::g_config.userPath != nullptr)\n"
    "        cardWorkingDir = reinterpret_cast<const char8_t*>(aurora::g_config.userPath);\n"
    "      else\n"
    "        cardWorkingDir = std::filesystem::current_path();\n"
    "    }\n"
    "  }",
    "card.cpp: prefer local saves/ folder in CARDInit",
)

apply(
    ABSEIL,
    "  FetchContent_Declare(abseil-cpp\n"
    "    URL https://github.com/abseil/abseil-cpp/archive/refs/tags/20240722.0.tar.gz\n"
    "    DOWNLOAD_EXTRACT_TIMESTAMP TRUE\n"
    "    EXCLUDE_FROM_ALL\n"
    "  )",
    "  FetchContent_Declare(abseil-cpp\n"
    "    URL https://github.com/abseil/abseil-cpp/archive/refs/tags/20240722.0.tar.gz\n"
    "    DOWNLOAD_EXTRACT_TIMESTAMP TRUE\n"
    "    EXCLUDE_FROM_ALL\n"
    "    PATCH_COMMAND ${CMAKE_COMMAND}\n"
    "      -DPATCH_FILE=<SOURCE_DIR>/absl/base/internal/sysinfo.cc\n"
    "      -P ${CMAKE_CURRENT_LIST_DIR}/../../../ci/patch_abseil_switch.cmake\n"
    "  )",
    "abseil-cpp: patch Switch thread-id fallback",
)

apply(
    DAWN_PROVIDER,
    "    FetchContent_Declare(dawn\n"
    "      URL \"https://github.com/google/dawn/archive/refs/tags/${AURORA_DAWN_VERSION}.tar.gz\"\n"
    "      DOWNLOAD_EXTRACT_TIMESTAMP TRUE\n"
    "      EXCLUDE_FROM_ALL\n"
    "    )",
    "    FetchContent_Declare(dawn\n"
    "      URL \"https://github.com/google/dawn/archive/refs/tags/${AURORA_DAWN_VERSION}.tar.gz\"\n"
    "      DOWNLOAD_EXTRACT_TIMESTAMP TRUE\n"
    "      EXCLUDE_FROM_ALL\n"
    "      PATCH_COMMAND ${CMAKE_COMMAND}\n"
    "        -DPATCH_FILE=<SOURCE_DIR>/third_party/CMakeLists.txt\n"
    f"        -P {DAWN_ABSEIL_PATCH}\n"
    "    )",
    "dawn: patch Switch thread-id fallback in vendored abseil",
)
