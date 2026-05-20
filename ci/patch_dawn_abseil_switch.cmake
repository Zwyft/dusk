if (NOT EXISTS "${PATCH_FILE}")
  message(FATAL_ERROR "patch_dawn_abseil_switch.cmake requires PATCH_FILE to point at Dawn's top-level CMakeLists.txt")
endif ()

get_filename_component(_dawn_root_dir "${PATCH_FILE}" DIRECTORY)
set(_absl_sysinfo "${_dawn_root_dir}/third_party/abseil-cpp/absl/base/internal/sysinfo.cc")
if (NOT EXISTS "${_absl_sysinfo}")
  message(FATAL_ERROR "aurora: could not find Dawn vendored abseil sysinfo.cc at ${_absl_sysinfo}")
endif ()

execute_process(
  COMMAND ${CMAKE_COMMAND}
    -DPATCH_FILE=${_absl_sysinfo}
    -P ${CMAKE_CURRENT_LIST_DIR}/patch_abseil_switch.cmake
  RESULT_VARIABLE _patch_result
)
if (NOT _patch_result EQUAL 0)
  message(FATAL_ERROR "aurora: Dawn Abseil Switch patch failed with code ${_patch_result}")
endif ()

message(STATUS "aurora: patched Dawn vendored abseil for Switch")
