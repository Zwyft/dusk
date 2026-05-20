if (NOT EXISTS "${PATCH_FILE}")
  message(FATAL_ERROR "patch_dawn_abseil_switch.cmake requires PATCH_FILE to point at Dawn's tools/fetch_dawn_dependencies.py")
endif ()

file(READ "${PATCH_FILE}" _dawn_fetch_deps)
set(_absl_hook [==[
        if submodule == 'third_party/abseil-cpp':
            absl_sysinfo = submodule_path / 'absl/base/internal/sysinfo.cc'
            if absl_sysinfo.is_file():
                patch_abseil_switch(absl_sysinfo)
]==])
set(_insert_after [==[
def log(msg):
    """Just makes it look good in the CMake log flow."""
    print(f"-- -- {msg}")
]==])
set(_helper_block [==[
def patch_abseil_switch(absl_sysinfo):
    """Patch Abseil's thread-id fallback for libnx."""
    text = absl_sysinfo.read_text()
    needle = "reinterpret_cast<intptr_t>(pthread_self())"
    if needle in text:
        log(f"Switch abseil patch already applied: {absl_sysinfo}")
        return

    patched = re.sub(
        r"static_cast<pid_t>\s*\(\s*pthread_self\s*\(\s*\)\s*\)",
        "static_cast<pid_t>(reinterpret_cast<intptr_t>(pthread_self()))",
        text,
    )
    if patched == text:
        raise RuntimeError(f"could not find abseil thread-id fallback in {absl_sysinfo}")

    absl_sysinfo.write_text(patched)
    log(f"applied Switch abseil patch: {absl_sysinfo}")
]==])

string(FIND "${_dawn_fetch_deps}" "def patch_abseil_switch(" _already_patched)
if (_already_patched GREATER -1)
  message(STATUS "aurora: dawn abseil patch already applied")
  return()
endif ()

set(_patched "${_dawn_fetch_deps}")
string(REPLACE [==[
import os
import sys
import subprocess
import argparse
from pathlib import Path
]==] [==[
import os
import sys
import subprocess
import argparse
import re
from pathlib import Path
]==] _patched "${_patched}")

string(FIND "${_patched}" "${_insert_after}" _anchor_index)
if (_anchor_index GREATER -1)
  string(REPLACE "${_insert_after}" "${_helper_block}" _patched "${_patched}")
else ()
  message(FATAL_ERROR "aurora: could not find Dawn fetch script log helper in ${PATCH_FILE}")
endif ()

string(FIND "${_patched}" "process_dir(args, submodule_path, required_subsubmodules)" _call_index)
if (_call_index GREATER -1)
  string(REPLACE "        process_dir(args, submodule_path, required_subsubmodules)" "        process_dir(args, submodule_path, required_subsubmodules)\n${_absl_hook}" _patched "${_patched}")
else ()
  message(FATAL_ERROR "aurora: could not find Dawn recursive dependency call in ${PATCH_FILE}")
endif ()

file(WRITE "${PATCH_FILE}" "${_patched}")
message(STATUS "aurora: patched Dawn dependency fetch script for Switch")
