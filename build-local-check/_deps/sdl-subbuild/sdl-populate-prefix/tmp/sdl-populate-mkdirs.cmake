# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/root/dusk/build-local-check/_deps/sdl-src"
  "/root/dusk/build-local-check/_deps/sdl-build"
  "/root/dusk/build-local-check/_deps/sdl-subbuild/sdl-populate-prefix"
  "/root/dusk/build-local-check/_deps/sdl-subbuild/sdl-populate-prefix/tmp"
  "/root/dusk/build-local-check/_deps/sdl-subbuild/sdl-populate-prefix/src/sdl-populate-stamp"
  "/root/dusk/build-local-check/_deps/sdl-subbuild/sdl-populate-prefix/src"
  "/root/dusk/build-local-check/_deps/sdl-subbuild/sdl-populate-prefix/src/sdl-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/root/dusk/build-local-check/_deps/sdl-subbuild/sdl-populate-prefix/src/sdl-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/root/dusk/build-local-check/_deps/sdl-subbuild/sdl-populate-prefix/src/sdl-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
