if(NOT DEFINED SDL_SOURCE_DIR)
    message(FATAL_ERROR "patch_sdl_switch.cmake requires -DSDL_SOURCE_DIR=<path>")
endif()

set(SDL_CHECKS_FILE "${SDL_SOURCE_DIR}/cmake/sdlchecks.cmake")
set(SDL_CMAKELISTS_FILE "${SDL_SOURCE_DIR}/CMakeLists.txt")
set(SDL_DYNAPI_HEADER_FILE "${SDL_SOURCE_DIR}/src/dynapi/SDL_dynapi.h")
set(SDL_SYSTIME_FILE "${SDL_SOURCE_DIR}/src/time/unix/SDL_systime.c")

if(NOT EXISTS "${SDL_CHECKS_FILE}")
    message(FATAL_ERROR "Could not find SDL checks file at ${SDL_CHECKS_FILE}")
endif()
if(NOT EXISTS "${SDL_CMAKELISTS_FILE}")
    message(FATAL_ERROR "Could not find SDL CMakeLists at ${SDL_CMAKELISTS_FILE}")
endif()
if(NOT EXISTS "${SDL_SYSTIME_FILE}")
    message(FATAL_ERROR "Could not find SDL unix time file at ${SDL_SYSTIME_FILE}")
endif()
if(NOT EXISTS "${SDL_DYNAPI_HEADER_FILE}")
    message(FATAL_ERROR "Could not find SDL dynapi header at ${SDL_DYNAPI_HEADER_FILE}")
endif()

file(READ "${SDL_CHECKS_FILE}" SDL_CHECKS_CONTENT)
file(READ "${SDL_CMAKELISTS_FILE}" SDL_CMAKELISTS_CONTENT)
file(READ "${SDL_SYSTIME_FILE}" SDL_SYSTIME_CONTENT)
file(READ "${SDL_DYNAPI_HEADER_FILE}" SDL_DYNAPI_HEADER_CONTENT)

set(SDL_CHECKS_OLD [=[macro(CheckPTHREAD)
  cmake_push_check_state()
  if(SDL_PTHREADS)
    if(ANDROID OR SDL_PTHREADS_PRIVATE)
      # the android libc provides built-in support for pthreads, so no
      # additional linking or compile flags are necessary
]=])

set(SDL_CHECKS_NEW [=[macro(CheckPTHREAD)
  cmake_push_check_state()
  if(SDL_PTHREADS)
    if((CMAKE_SYSTEM_NAME STREQUAL "Generic" AND CMAKE_SYSTEM_VERSION STREQUAL "DKA-NX") OR SWITCH OR DUSK_SWITCH_LIBNX_TOOLCHAIN)
      # libnx provides pthreads, recursive mutexes, and semaphores without a
      # separate libpthread. SDL's Generic fallback incorrectly probes with
      # platform heuristics that never enable the backend for this target.
      set(PTHREAD_CFLAGS "-D_REENTRANT")
      set(PTHREAD_LDFLAGS "")
      set(HAVE_PTHREADS TRUE)
      set(HAVE_PTHREAD_H TRUE)
      set(SDL_THREAD_PTHREAD 1)
      set(SDL_THREAD_PTHREAD_RECURSIVE_MUTEX 1)
      separate_arguments(PTHREAD_CFLAGS)
      sdl_compile_options(PRIVATE ${PTHREAD_CFLAGS})
      sdl_sources(
        "${SDL3_SOURCE_DIR}/src/thread/pthread/SDL_systhread.c"
        "${SDL3_SOURCE_DIR}/src/thread/pthread/SDL_sysmutex.c"
        "${SDL3_SOURCE_DIR}/src/thread/pthread/SDL_syscond.c"
        "${SDL3_SOURCE_DIR}/src/thread/pthread/SDL_sysrwlock.c"
        "${SDL3_SOURCE_DIR}/src/thread/pthread/SDL_systls.c"
      )
      if(SDL_PTHREADS_SEM)
        set(HAVE_PTHREADS_SEM TRUE)
        set(HAVE_SEM_TIMEDWAIT TRUE)
        sdl_sources("${SDL3_SOURCE_DIR}/src/thread/pthread/SDL_syssem.c")
      else()
        sdl_sources("${SDL3_SOURCE_DIR}/src/thread/generic/SDL_syssem.c")
      endif()
      set(HAVE_SDL_THREADS TRUE)
    elseif(ANDROID OR SDL_PTHREADS_PRIVATE)
      # the android libc provides built-in support for pthreads, so no
      # additional linking or compile flags are necessary
]=])

set(SDL_BRANCH_OLD [=[elseif(NGAGE)

  enable_language(CXX)
]=])

set(SDL_DYNAPI_OLD [=[#elif defined(SDL_PLATFORM_VITA)
#define SDL_DYNAMIC_API 0 // vitasdk doesn't support dynamic linking
#elif defined(SDL_PLATFORM_3DS)
#define SDL_DYNAMIC_API 0 // devkitARM doesn't support dynamic linking
]=])

set(SDL_DYNAPI_NEW [=[#elif defined(SDL_PLATFORM_VITA)
#define SDL_DYNAMIC_API 0 // vitasdk doesn't support dynamic linking
#elif defined(SDL_PLATFORM_3DS)
#define SDL_DYNAMIC_API 0 // devkitARM doesn't support dynamic linking
#elif defined(__SWITCH__) || defined(SWITCH)
#define SDL_DYNAMIC_API 0 // libnx/devkitA64 doesn't provide dlopen-style dynamic loading
]=])

set(SDL_BRANCH_NEW [=[elseif((CMAKE_SYSTEM_NAME STREQUAL "Generic" AND CMAKE_SYSTEM_VERSION STREQUAL "DKA-NX") OR SWITCH OR DUSK_SWITCH_LIBNX_TOOLCHAIN)
  sdl_glob_sources(
    "${SDL3_SOURCE_DIR}/src/core/unix/*.c"
    "${SDL3_SOURCE_DIR}/src/core/unix/*.h"
  )

  sdl_glob_sources("${SDL3_SOURCE_DIR}/src/misc/unix/*.c")
  set(HAVE_SDL_MISC TRUE)

  sdl_glob_sources("${SDL3_SOURCE_DIR}/src/locale/unix/*.c")
  set(HAVE_SDL_LOCALE TRUE)

  set(SDL_FILESYSTEM_UNIX 1)
  sdl_glob_sources("${SDL3_SOURCE_DIR}/src/filesystem/unix/*.c")
  set(HAVE_SDL_FILESYSTEM TRUE)

  set(SDL_FSOPS_POSIX 1)
  sdl_sources("${SDL3_SOURCE_DIR}/src/filesystem/posix/SDL_sysfsops.c")
  set(HAVE_SDL_FSOPS TRUE)

  set(SDL_TIME_UNIX 1)
  sdl_glob_sources("${SDL3_SOURCE_DIR}/src/time/unix/*.c")
  set(HAVE_SDL_TIME TRUE)

  set(SDL_TIMER_UNIX 1)
  sdl_glob_sources("${SDL3_SOURCE_DIR}/src/timer/unix/*.c")
  set(HAVE_SDL_TIMERS TRUE)

  CheckPTHREAD()

  # Switch/libnx has no GTK/AppIndicator stack, so SDL's tray backend must stay off.
  set(SDL_TRAY OFF)
  set_source_files_properties("${SDL3_SOURCE_DIR}/src/core/unix/SDL_gtk.c" PROPERTIES HEADER_FILE_ONLY TRUE)

elseif(NGAGE)

  enable_language(CXX)
]=])

string(FIND "${SDL_CHECKS_CONTENT}" "if((CMAKE_SYSTEM_NAME STREQUAL \"Generic\" AND CMAKE_SYSTEM_VERSION STREQUAL \"DKA-NX\") OR SWITCH OR DUSK_SWITCH_LIBNX_TOOLCHAIN)" SDL_SWITCH_CHECK_POS)
if(SDL_SWITCH_CHECK_POS EQUAL -1)
    string(FIND "${SDL_CHECKS_CONTENT}" "${SDL_CHECKS_OLD}" SDL_PATCH_POS)
    if(SDL_PATCH_POS EQUAL -1)
        message(FATAL_ERROR "patch_sdl_switch: failed to find CheckPTHREAD insertion point in ${SDL_CHECKS_FILE}")
    endif()
    string(REPLACE "${SDL_CHECKS_OLD}" "${SDL_CHECKS_NEW}" SDL_CHECKS_CONTENT "${SDL_CHECKS_CONTENT}")
    file(WRITE "${SDL_CHECKS_FILE}" "${SDL_CHECKS_CONTENT}")
    message(STATUS "patch_sdl_switch: patched ${SDL_CHECKS_FILE}")
else()
    message(STATUS "patch_sdl_switch: SDL CheckPTHREAD already patched")
endif()

string(FIND "${SDL_CMAKELISTS_CONTENT}" "src/core/unix/*.c" SDL_SWITCH_BRANCH_HINT)
string(FIND "${SDL_CMAKELISTS_CONTENT}" "elseif((CMAKE_SYSTEM_NAME STREQUAL \"Generic\" AND CMAKE_SYSTEM_VERSION STREQUAL \"DKA-NX\") OR SWITCH OR DUSK_SWITCH_LIBNX_TOOLCHAIN)" SDL_SWITCH_BRANCH_POS)
if(SDL_SWITCH_BRANCH_POS EQUAL -1)
    string(FIND "${SDL_CMAKELISTS_CONTENT}" "${SDL_BRANCH_OLD}" SDL_BRANCH_INSERT_POS)
    if(SDL_BRANCH_INSERT_POS EQUAL -1)
        message(FATAL_ERROR "patch_sdl_switch: failed to find platform branch insertion point in ${SDL_CMAKELISTS_FILE}")
    endif()
    string(REPLACE "${SDL_BRANCH_OLD}" "${SDL_BRANCH_NEW}" SDL_CMAKELISTS_CONTENT "${SDL_CMAKELISTS_CONTENT}")
    file(WRITE "${SDL_CMAKELISTS_FILE}" "${SDL_CMAKELISTS_CONTENT}")
    message(STATUS "patch_sdl_switch: patched ${SDL_CMAKELISTS_FILE}")
else()
    message(STATUS "patch_sdl_switch: SDL platform branch already patched")
endif()

find_package(Python3 REQUIRED COMPONENTS Interpreter)
execute_process(
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_LIST_DIR}/patch_sdl_time_switch.py" "${SDL_SYSTIME_FILE}"
    RESULT_VARIABLE SDL_TIME_PATCH_RESULT
)
if(NOT SDL_TIME_PATCH_RESULT EQUAL 0)
    message(FATAL_ERROR "patch_sdl_switch: SDL time patch helper failed")
endif()


string(FIND "${SDL_DYNAPI_HEADER_CONTENT}" "defined(__SWITCH__) || defined(SWITCH)" SDL_DYNAPI_SWITCH_POS)
if(SDL_DYNAPI_SWITCH_POS EQUAL -1)
    string(FIND "${SDL_DYNAPI_HEADER_CONTENT}" "${SDL_DYNAPI_OLD}" SDL_DYNAPI_PATCH_POS)
    if(SDL_DYNAPI_PATCH_POS EQUAL -1)
        message(FATAL_ERROR "patch_sdl_switch: failed to find dynapi insertion point in ${SDL_DYNAPI_HEADER_FILE}")
    endif()
    string(REPLACE "${SDL_DYNAPI_OLD}" "${SDL_DYNAPI_NEW}" SDL_DYNAPI_HEADER_CONTENT "${SDL_DYNAPI_HEADER_CONTENT}")
    file(WRITE "${SDL_DYNAPI_HEADER_FILE}" "${SDL_DYNAPI_HEADER_CONTENT}")
    message(STATUS "patch_sdl_switch: patched ${SDL_DYNAPI_HEADER_FILE}")
else()
    message(STATUS "patch_sdl_switch: SDL dynapi header already patched")
endif()
