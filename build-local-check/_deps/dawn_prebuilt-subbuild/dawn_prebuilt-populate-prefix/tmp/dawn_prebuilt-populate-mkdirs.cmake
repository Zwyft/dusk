# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/root/dusk/build-local-check/_deps/dawn_prebuilt-src"
  "/root/dusk/build-local-check/_deps/dawn_prebuilt-build"
  "/root/dusk/build-local-check/_deps/dawn_prebuilt-subbuild/dawn_prebuilt-populate-prefix"
  "/root/dusk/build-local-check/_deps/dawn_prebuilt-subbuild/dawn_prebuilt-populate-prefix/tmp"
  "/root/dusk/build-local-check/_deps/dawn_prebuilt-subbuild/dawn_prebuilt-populate-prefix/src/dawn_prebuilt-populate-stamp"
  "/root/dusk/build-local-check/_deps/dawn_prebuilt-subbuild/dawn_prebuilt-populate-prefix/src"
  "/root/dusk/build-local-check/_deps/dawn_prebuilt-subbuild/dawn_prebuilt-populate-prefix/src/dawn_prebuilt-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/root/dusk/build-local-check/_deps/dawn_prebuilt-subbuild/dawn_prebuilt-populate-prefix/src/dawn_prebuilt-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/root/dusk/build-local-check/_deps/dawn_prebuilt-subbuild/dawn_prebuilt-populate-prefix/src/dawn_prebuilt-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
