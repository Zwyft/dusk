if (NOT EXISTS "${PATCH_FILE}")
  message(FATAL_ERROR "patch_dawn_abseil_switch.cmake requires PATCH_FILE to point at Dawn's top-level CMakeLists.txt")
endif ()

file(READ "${PATCH_FILE}" _dawn_root_cmake)
set(_patch_script "${CMAKE_CURRENT_LIST_DIR}/patch_abseil_switch.cmake")
set(_insert_after [==[
set_if_not_defined(DAWN_ABSEIL_DIR "${DAWN_THIRD_PARTY_DIR}/abseil-cpp" "Directory in which to find Abseil")
]==])
set(_injected_block [==[
set_if_not_defined(DAWN_ABSEIL_DIR "${DAWN_THIRD_PARTY_DIR}/abseil-cpp" "Directory in which to find Abseil")
if (EXISTS "${DAWN_ABSEIL_DIR}/absl/base/internal/sysinfo.cc")
    execute_process(
        COMMAND ${CMAKE_COMMAND}
            -DPATCH_FILE=${DAWN_ABSEIL_DIR}/absl/base/internal/sysinfo.cc
            -P __PATCH_ABSEIL_SCRIPT__
        RESULT_VARIABLE _patch_result
    )
    if (NOT _patch_result EQUAL 0)
        message(FATAL_ERROR "Dawn Abseil Switch patch failed with code ${_patch_result}")
    endif()
endif()
]==])
string(REPLACE "__PATCH_ABSEIL_SCRIPT__" "${_patch_script}" _injected_block "${_injected_block}")

string(FIND "${_dawn_root_cmake}" "${_injected_block}" _already_patched)
if (_already_patched GREATER -1)
  message(STATUS "aurora: dawn abseil patch already applied")
  return()
endif ()

string(FIND "${_dawn_root_cmake}" "${_insert_after}" _anchor_index)
if (_anchor_index GREATER -1)
  string(REPLACE "${_insert_after}" "${_injected_block}" _dawn_root_cmake "${_dawn_root_cmake}")
  file(WRITE "${PATCH_FILE}" "${_dawn_root_cmake}")
  message(STATUS "aurora: applied dawn abseil Switch patch")
else ()
  message(FATAL_ERROR "aurora: could not find Dawn Abseil dir definition in ${PATCH_FILE}")
endif ()
