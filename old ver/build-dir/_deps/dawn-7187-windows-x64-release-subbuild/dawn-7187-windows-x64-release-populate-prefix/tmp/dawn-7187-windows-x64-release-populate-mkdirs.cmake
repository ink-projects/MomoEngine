# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "C:/Users/Momo/Desktop/Momo Engine/build-dir/_deps/dawn-7187-windows-x64-release-src")
  file(MAKE_DIRECTORY "C:/Users/Momo/Desktop/Momo Engine/build-dir/_deps/dawn-7187-windows-x64-release-src")
endif()
file(MAKE_DIRECTORY
  "C:/Users/Momo/Desktop/Momo Engine/build-dir/_deps/dawn-7187-windows-x64-release-build"
  "C:/Users/Momo/Desktop/Momo Engine/build-dir/_deps/dawn-7187-windows-x64-release-subbuild/dawn-7187-windows-x64-release-populate-prefix"
  "C:/Users/Momo/Desktop/Momo Engine/build-dir/_deps/dawn-7187-windows-x64-release-subbuild/dawn-7187-windows-x64-release-populate-prefix/tmp"
  "C:/Users/Momo/Desktop/Momo Engine/build-dir/_deps/dawn-7187-windows-x64-release-subbuild/dawn-7187-windows-x64-release-populate-prefix/src/dawn-7187-windows-x64-release-populate-stamp"
  "C:/Users/Momo/Desktop/Momo Engine/build-dir/_deps/dawn-7187-windows-x64-release-subbuild/dawn-7187-windows-x64-release-populate-prefix/src"
  "C:/Users/Momo/Desktop/Momo Engine/build-dir/_deps/dawn-7187-windows-x64-release-subbuild/dawn-7187-windows-x64-release-populate-prefix/src/dawn-7187-windows-x64-release-populate-stamp"
)

set(configSubDirs Debug)
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/Users/Momo/Desktop/Momo Engine/build-dir/_deps/dawn-7187-windows-x64-release-subbuild/dawn-7187-windows-x64-release-populate-prefix/src/dawn-7187-windows-x64-release-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/Users/Momo/Desktop/Momo Engine/build-dir/_deps/dawn-7187-windows-x64-release-subbuild/dawn-7187-windows-x64-release-populate-prefix/src/dawn-7187-windows-x64-release-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
