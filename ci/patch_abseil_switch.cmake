if (NOT EXISTS "${PATCH_FILE}")
  message(FATAL_ERROR "patch_abseil_switch.cmake requires PATCH_FILE to point at abseil sysinfo.cc")
endif ()

file(READ "${PATCH_FILE}" _absl_sysinfo)
set(_needle "reinterpret_cast<intptr_t>(pthread_self())")
string(FIND "${_absl_sysinfo}" "${_needle}" _already_patched)
if (_already_patched GREATER -1)
  message(STATUS "aurora: abseil Switch patch already applied")
else ()
  set(_patched "${_absl_sysinfo}")
  string(REGEX REPLACE
    "static_cast<pid_t>\\(pthread_self\\(\\)\\)"
    "static_cast<pid_t>(reinterpret_cast<intptr_t>(pthread_self()))"
    _patched
    "${_patched}")
  if (_patched STREQUAL _absl_sysinfo)
    message(FATAL_ERROR "aurora: could not find abseil thread-id fallback in ${PATCH_FILE}")
  else ()
    file(WRITE "${PATCH_FILE}" "${_patched}")
    message(STATUS "aurora: applied abseil Switch thread-id patch")
  endif ()
endif ()
