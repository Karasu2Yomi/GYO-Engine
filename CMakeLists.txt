cmake_minimum_required(VERSION 3.30)
project(GYO-Engine LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(GYO_BUILD_RUNTIME "Build the minimal standalone runtime application" OFF)
option(GYO_BUILD_SANDBOX "Build the optional ImGui/JSON sandbox application" OFF)
option(GYO_BUILD_OBJECT_FPS "Build the Object_FPS GYO conformance game" ON)
option(
    GYO_BUILD_SDL_GPU_BACKEND
    "Build the optional Windows/MSVC SDL_GPU render backend"
    OFF
)

include(CTest)

# Keep dependency sources private to each build tree. Disconnected updates still
# allow the first population, but do not mutate an already populated dependency.
set(FETCHCONTENT_UPDATES_DISCONNECTED ON CACHE BOOL
    "Do not update already-populated FetchContent dependencies")

add_subdirectory(third_party)
add_subdirectory(engine)

if(GYO_BUILD_RUNTIME OR GYO_BUILD_SANDBOX OR GYO_BUILD_OBJECT_FPS OR
   GYO_BUILD_SDL_GPU_BACKEND)
    add_subdirectory(platform)
    add_subdirectory(render)
endif()

if(GYO_BUILD_RUNTIME OR GYO_BUILD_SANDBOX OR GYO_BUILD_OBJECT_FPS)
    add_subdirectory(input)
    add_subdirectory(apps)
endif()

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
