if (NOT EXISTS "${PATCH_FILE}")
  message(FATAL_ERROR "patch_dawn_abseil_switch.cmake requires PATCH_FILE to point at Dawn's third_party/CMakeLists.txt")
endif ()

file(READ "${PATCH_FILE}" _dawn_third_party)
set(_patch_script "${CMAKE_CURRENT_LIST_DIR}/patch_abseil_switch.cmake")
set(_old [==[
    add_subdirectory(${DAWN_ABSEIL_DIR} "${CMAKE_CURRENT_BINARY_DIR}/abseil")
]==])
set(_new [==[
    if (EXISTS "${DAWN_ABSEIL_DIR}/absl/base/internal/sysinfo.cc")
        execute_process(
            COMMAND ${CMAKE_COMMAND}
                -DPATCH_FILE=${DAWN_ABSEIL_DIR}/absl/base/internal/sysinfo.cc
                -P __PATCH_ABSEIL_SCRIPT__
        )
    endif()
    add_subdirectory(${DAWN_ABSEIL_DIR} "${CMAKE_CURRENT_BINARY_DIR}/abseil")
]==])
string(REPLACE "__PATCH_ABSEIL_SCRIPT__" "${_patch_script}" _new "${_new}")

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
    message(FATAL_ERROR "aurora: could not find Dawn abseil add_subdirectory line in ${PATCH_FILE}")
  endif ()
endif ()
