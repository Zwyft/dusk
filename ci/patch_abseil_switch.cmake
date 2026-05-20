if (NOT EXISTS "${PATCH_FILE}")
  message(FATAL_ERROR "patch_abseil_switch.cmake requires PATCH_FILE to point at abseil sysinfo.cc")
endif ()

file(READ "${PATCH_FILE}" _absl_sysinfo)
set(_needle "reinterpret_cast<intptr_t>(pthread_self())")
string(FIND "${_absl_sysinfo}" "${_needle}" _already_patched)
if (_already_patched GREATER -1)
  message(STATUS "aurora: abseil Switch patch already applied")
  return()
endif ()

set(_patterns
  "static_cast<pid_t>\\s*\\(\\s*pthread_self\\s*\\(\\s*\\)\\s*\\)"
  "static_cast<\\s*pid_t\\s*>\\s*\\(\\s*pthread_self\\s*\\(\\s*\\)\\s*\\)"
)

set(_patched "${_absl_sysinfo}")
foreach (_pattern IN LISTS _patterns)
  string(REGEX REPLACE
    "${_pattern}"
    "static_cast<pid_t>(reinterpret_cast<intptr_t>(pthread_self()))"
    _next
    "${_patched}")
  set(_patched "${_next}")
endforeach ()

if (_patched STREQUAL _absl_sysinfo)
  message(FATAL_ERROR "aurora: could not find abseil thread-id fallback in ${PATCH_FILE}")
endif ()

file(WRITE "${PATCH_FILE}" "${_patched}")
message(STATUS "aurora: applied abseil Switch thread-id patch")
