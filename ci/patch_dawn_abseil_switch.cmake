if (NOT EXISTS "${PATCH_FILE}")
  message(FATAL_ERROR "patch_dawn_abseil_switch.cmake requires PATCH_FILE to point at Dawn's third_party/CMakeLists.txt")
endif ()

file(READ "${PATCH_FILE}" _dawn_third_party)
set(_patch_script "${CMAKE_CURRENT_LIST_DIR}/patch_abseil_switch.cmake")
string(CONCAT _old
  "  FetchContent_Declare(abseil-cpp\n"
  "    URL https://github.com/abseil/abseil-cpp/archive/refs/tags/20240722.0.tar.gz\n"
  "    DOWNLOAD_EXTRACT_TIMESTAMP TRUE\n"
  "    EXCLUDE_FROM_ALL\n"
  "  )"
)
string(CONCAT _new
  "  FetchContent_Declare(abseil-cpp\n"
  "    URL https://github.com/abseil/abseil-cpp/archive/refs/tags/20240722.0.tar.gz\n"
  "    DOWNLOAD_EXTRACT_TIMESTAMP TRUE\n"
  "    EXCLUDE_FROM_ALL\n"
  "    PATCH_COMMAND ${CMAKE_COMMAND}\n"
  "      -DPATCH_FILE=<SOURCE_DIR>/absl/base/internal/sysinfo.cc\n"
  "      -P ${_patch_script}\n"
  "  )"
)

string(FIND "${_dawn_third_party}" "patch_abseil_switch.cmake" _already_patched)
if (_already_patched GREATER -1)
  message(STATUS "aurora: dawn abseil patch already applied")
else ()
  string(FIND "${_dawn_third_party}" "${_old}" _anchor_index)
  if (_anchor_index GREATER -1)
    string(REPLACE "${_old}" "${_new}" _dawn_third_party "${_dawn_third_party}")
    file(WRITE "${PATCH_FILE}" "${_dawn_third_party}")
    message(STATUS "aurora: applied dawn abseil Switch patch")
  else ()
    message(FATAL_ERROR "aurora: could not find Dawn abseil FetchContent declaration in ${PATCH_FILE}")
  endif ()
endif ()
