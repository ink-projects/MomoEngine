cmake_minimum_required(VERSION 3.14...3.31)
project(
    momoengine # Name this whatever you want
    LANGUAGES CXX C
)

## Enable decentralized package loading
include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)

## Install glfw
FetchContent_Declare(
    glfw3
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw3)

# Install spdlog
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog/
    GIT_TAG v1.15.3
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(spdlog)

# Include stb
FetchContent_Declare(
  stb
  GIT_REPOSITORY https://github.com/nothings/stb/
  GIT_TAG        master
  GIT_SHALLOW TRUE
  GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable( stb )
## stb doesn't have a `CMakeLists.txt`. Let's declare a header-only library with an include path.
add_library( stb INTERFACE )
target_include_directories( stb INTERFACE ${stb_SOURCE_DIR} )

## Include glm
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG master
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(glm)

## Include WebGPU
set(WEBGPU_BACKEND "dawn" CACHE STRING "WebGPU backend (wgpu or dawn)")
FetchContent_Declare(
    webgpu
    GIT_REPOSITORY https://github.com/yig/WebGPU-distribution
    GIT_TAG        cs425-2025
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(webgpu)

## Include glfw3webgpu
FetchContent_Declare(
    glfw3webgpu
    GIT_REPOSITORY https://github.com/eliemichel/glfw3webgpu
    GIT_TAG        fdcabcc54b56b50c12c10f5317abf8ae7ac32c29
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(glfw3webgpu)

## Declare the engine library
add_library(momoengine STATIC
    src/Engine.cpp
    src/GraphicsManager.cpp
    src/InputManager.cpp
    src/ResourceManager.cpp
)
set_target_properties(momoengine PROPERTIES CXX_STANDARD 20)

## Declare our engine's header path.
## This allows targets that depend on the engine to #include them.
target_include_directories(momoengine PUBLIC src)

## Link all libraries
target_link_libraries(momoengine
    PUBLIC
        glfw
        spdlog::spdlog
        glm::glm
        webgpu
        glfw3webgpu
	stb
)

## Demo executable
add_executable(helloworld demo/helloworld.cpp)
set_target_properties(helloworld PROPERTIES CXX_STANDARD 20)
target_link_libraries(helloworld PRIVATE momoengine)

## Run target with correct working directory
add_custom_target(run_helloworld helloworld USES_TERMINAL WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

## Copy WebGPU binaries (needed on some platforms)
target_copy_webgpu_binaries(helloworld)

# Copy assets to build directory
add_custom_command(TARGET helloworld POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:helloworld>/assets
)