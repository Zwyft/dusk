set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR aarch64)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
set(CMAKE_SYSTEM_VERSION "DKA-NX")
set(SWITCH TRUE)

if(NOT DEFINED ENV{DEVKITPRO})
    message(FATAL_ERROR "switch-libnx toolchain requires DEVKITPRO to be set")
endif()

if(NOT DEFINED ENV{DEVKITA64})
    message(FATAL_ERROR "switch-libnx toolchain requires DEVKITA64 to be set")
endif()

set(DEVKITPRO $ENV{DEVKITPRO})
set(DEVKITA64 $ENV{DEVKITA64})
set(LIBNX "${DEVKITPRO}/libnx")
set(PORTLIBS "${DEVKITPRO}/portlibs/switch")

if(NOT EXISTS "${LIBNX}/include/switch.h")
    message(FATAL_ERROR
        "switch-libnx toolchain could not find libnx headers at ${LIBNX}/include/switch.h")
endif()

if(NOT EXISTS "${PORTLIBS}")
    message(FATAL_ERROR
        "switch-libnx toolchain could not find switch portlibs at ${PORTLIBS}")
endif()

if(WIN32)
    set(CMAKE_C_COMPILER "${DEVKITA64}/bin/aarch64-none-elf-gcc.exe")
    set(CMAKE_CXX_COMPILER "${DEVKITA64}/bin/aarch64-none-elf-g++.exe")
    set(CMAKE_ASM_COMPILER "${DEVKITA64}/bin/aarch64-none-elf-gcc.exe")
    set(CMAKE_LINKER "${DEVKITA64}/bin/aarch64-none-elf-ld.exe")
    set(CMAKE_AR "${DEVKITA64}/bin/aarch64-none-elf-gcc-ar.exe")
    set(CMAKE_RANLIB "${DEVKITA64}/bin/aarch64-none-elf-gcc-ranlib.exe")
    set(CMAKE_STRIP "${DEVKITA64}/bin/aarch64-none-elf-strip.exe")
    set(CMAKE_NM "${DEVKITA64}/bin/aarch64-none-elf-gcc-nm.exe")
else()
    set(CMAKE_C_COMPILER "${DEVKITA64}/bin/aarch64-none-elf-gcc")
    set(CMAKE_CXX_COMPILER "${DEVKITA64}/bin/aarch64-none-elf-g++")
    set(CMAKE_ASM_COMPILER "${DEVKITA64}/bin/aarch64-none-elf-gcc")
    set(CMAKE_LINKER "${DEVKITA64}/bin/aarch64-none-elf-ld")
    set(CMAKE_AR "${DEVKITA64}/bin/aarch64-none-elf-gcc-ar")
    set(CMAKE_RANLIB "${DEVKITA64}/bin/aarch64-none-elf-gcc-ranlib")
    set(CMAKE_STRIP "${DEVKITA64}/bin/aarch64-none-elf-strip")
    set(CMAKE_NM "${DEVKITA64}/bin/aarch64-none-elf-gcc-nm")
endif()

if(NOT EXISTS "${CMAKE_C_COMPILER}")
    message(FATAL_ERROR "switch-libnx toolchain could not find C compiler at ${CMAKE_C_COMPILER}")
endif()

if(NOT EXISTS "${CMAKE_CXX_COMPILER}")
    message(FATAL_ERROR "switch-libnx toolchain could not find C++ compiler at ${CMAKE_CXX_COMPILER}")
endif()

list(APPEND CMAKE_PROGRAM_PATH "${DEVKITPRO}/tools/bin")
list(APPEND CMAKE_PROGRAM_PATH "${DEVKITA64}/bin")

set(CMAKE_FIND_ROOT_PATH
    "${DEVKITPRO}"
    "${DEVKITA64}"
    "${PORTLIBS}"
    "${LIBNX}"
)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# libnx's headers (including poll.h and switch.h) are not part of the compiler's
# default search path, so expose them globally for all Switch targets.
include_directories(SYSTEM "${LIBNX}/include")

set(THREADS_PREFER_PTHREAD_FLAG TRUE CACHE BOOL
    "Prefer pthread flags when detecting threads" FORCE)
set(SDL_THREADS_ENABLED_BY_DEFAULT ON CACHE BOOL
    "Default SDL threads subsystem to enabled on Switch" FORCE)
set(SDL_THREADS ON CACHE BOOL
    "Enable SDL threads subsystem for Switch" FORCE)
set(HAVE_SDL_THREADS TRUE CACHE BOOL
    "Force SDL threads subsystem detection on Switch" FORCE)
set(HAVE_SDL_TIMERS TRUE CACHE BOOL
    "Force SDL timers subsystem detection on Switch" FORCE)
set(SDL_PTHREADS TRUE CACHE BOOL
    "Switch build uses POSIX threads" FORCE)
set(SDL_PTHREADS_PRIVATE TRUE CACHE BOOL
    "Switch build uses built-in pthread support without extra link flags" FORCE)
set(SDL_PTHREADS_SEM TRUE CACHE BOOL
    "Switch build uses pthread semaphores" FORCE)
set(CMAKE_THREAD_LIBS_INIT "" CACHE STRING "Thread libraries for Switch builds" FORCE)
set(CMAKE_USE_PTHREADS_INIT TRUE CACHE BOOL "Switch build uses pthread-style threading" FORCE)
set(CMAKE_HAVE_THREADS_LIBRARY TRUE CACHE BOOL "Switch build has threads support" FORCE)
set(CMAKE_HAVE_PTHREAD_H TRUE CACHE BOOL "Switch build has pthread.h available" FORCE)
set(CMAKE_DL_LIBS "" CACHE STRING "Switch uses no separate libdl" FORCE)

set(CMAKE_INSTALL_PREFIX "${PORTLIBS}" CACHE PATH
    "Install libraries to the Switch portlibs directory")
set(CMAKE_PREFIX_PATH "${PORTLIBS}" CACHE PATH
    "Search Switch portlibs for packages")

set_property(GLOBAL PROPERTY TARGET_SUPPORTS_SHARED_LIBS FALSE)
add_definitions(-DSWITCH -D__SWITCH__ -DSQLITE_MAX_MMAP_SIZE=0 -DABSL_FORCE_THREAD_IDENTITY_MODE=2)

set(DUSK_SWITCH_ARCH_FLAGS
    "-march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE")
set(DUSK_SWITCH_COMMON_C_FLAGS
    "-g -Wall -O2 -ffunction-sections ${DUSK_SWITCH_ARCH_FLAGS} -Wno-psabi")
set(CMAKE_C_FLAGS
    "${DUSK_SWITCH_COMMON_C_FLAGS}"
    CACHE STRING "C flags for experimental Switch builds" FORCE)
set(CMAKE_CXX_FLAGS
    "${DUSK_SWITCH_COMMON_C_FLAGS} -frtti -fexceptions"
    CACHE STRING "CXX flags for experimental Switch builds" FORCE)
set(CMAKE_ASM_FLAGS
    "-g ${DUSK_SWITCH_ARCH_FLAGS}"
    CACHE STRING "ASM flags for experimental Switch builds" FORCE)
set(CMAKE_EXE_LINKER_FLAGS "-specs=${LIBNX}/switch.specs -Wl,--gc-sections -Wl,--allow-multiple-definition" CACHE STRING "Executable linker flags" FORCE)
set(CMAKE_STATIC_LINKER_FLAGS "" CACHE STRING "Static linker flags" FORCE)
set(CMAKE_MODULE_LINKER_FLAGS "" CACHE STRING "Module linker flags" FORCE)

set(DUSK_SWITCH_LIBNX_TOOLCHAIN ON CACHE BOOL
    "Using the experimental libnx-based Nintendo Switch toolchain scaffold")
