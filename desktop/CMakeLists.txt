cmake_minimum_required(VERSION 3.16)
project(ascii-aquarium-desktop
    VERSION 0.0.1
    DESCRIPTION "ASCII Aquarium — x86 Linux/Windows desktop port"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# SDL2: prefer a system install (e.g. Linux `apt install libsdl2-dev`); if none
# is found (typical on Windows), download and build it automatically so the
# project stays a one-command build everywhere.
#
# AQ_BUNDLED_SDL2=ON forces the fetch-and-static-build path even when a system
# SDL2 exists — used by the CI release builds (Windows/macOS) to produce a
# standalone binary with no SDL2 runtime dependency.
option(AQ_BUNDLED_SDL2 "Fetch and statically build SDL2 instead of using a system install" OFF)
# When ON, require a system SDL2 and error out if it's missing, instead of
# silently fetching from GitHub. CI's Linux job sets this so a runner that's
# missing libsdl2-dev fails fast with a clear message rather than hanging on a
# network fetch it may not be able to reach.
option(AQ_REQUIRE_SYSTEM_SDL2 "Require a system SDL2; do not fall back to fetching" OFF)

function(aq_fetch_sdl2)
    message(STATUS "Fetching and statically building SDL2 from source")
    include(FetchContent)
    set(SDL_SHARED OFF CACHE BOOL "" FORCE)
    set(SDL_STATIC ON  CACHE BOOL "" FORCE)
    set(SDL_TEST   OFF CACHE BOOL "" FORCE)
    set(SDL2_DISABLE_INSTALL ON CACHE BOOL "" FORCE)
    FetchContent_Declare(SDL2
        URL https://github.com/libsdl-org/SDL/releases/download/release-2.32.4/SDL2-2.32.4.zip
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(SDL2)
endfunction()

if(AQ_BUNDLED_SDL2)
    aq_fetch_sdl2()
else()
    find_package(SDL2 QUIET)
    if(NOT SDL2_FOUND)
        if(AQ_REQUIRE_SYSTEM_SDL2)
            message(FATAL_ERROR
                "System SDL2 not found. Install the development package "
                "(Debian/Ubuntu: apt install libsdl2-dev) or configure with "
                "-DAQ_BUNDLED_SDL2=ON to build SDL2 from source.")
        endif()
        message(STATUS "System SDL2 not found — falling back to a bundled build")
        aq_fetch_sdl2()
    endif()
endif()

set(AQ_SOURCES
    src/main.cpp
    src/renderer/Framebuffer.cpp
    src/renderer/Font.cpp
    src/sim/Aquarium.cpp
    src/sim/Background.cpp
    src/sim/Clock.cpp
    src/sim/Lighting.cpp
    src/ui/Ui.cpp
    src/app/Config.cpp
    src/app/Capture.cpp
)

add_executable(ascii-aquarium ${AQ_SOURCES})

# On Windows, build a GUI (windowed) app so launching it doesn't pop a console
# window — important for kiosk use. SDL2main provides the WinMain entry point.
if(WIN32)
    set_target_properties(ascii-aquarium PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

target_include_directories(ascii-aquarium PRIVATE src assets)

# SDL2main first (provides the Windows entry-point shim), then the library —
# preferring the static target from a fetched build for a standalone binary.
if(TARGET SDL2::SDL2main)
    target_link_libraries(ascii-aquarium PRIVATE SDL2::SDL2main)
endif()
if(TARGET SDL2::SDL2-static)
    target_link_libraries(ascii-aquarium PRIVATE SDL2::SDL2-static)
elseif(TARGET SDL2::SDL2)
    target_link_libraries(ascii-aquarium PRIVATE SDL2::SDL2)
else()
    target_include_directories(ascii-aquarium PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(ascii-aquarium PRIVATE ${SDL2_LIBRARIES})
endif()

# If SDL2 ended up as a shared library, drop its DLL next to the executable.
if(WIN32 AND TARGET SDL2::SDL2 AND NOT TARGET SDL2::SDL2-static)
    add_custom_command(TARGET ascii-aquarium POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SDL2::SDL2> $<TARGET_FILE_DIR:ascii-aquarium>)
endif()

if(MSVC)
    target_compile_options(ascii-aquarium PRIVATE /W4 /permissive-)
    # Quiet MSVC's nags about standard strncpy/getenv (used safely here).
    target_compile_definitions(ascii-aquarium PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(ascii-aquarium PRIVATE
        -Wall -Wextra -Wpedantic -Wshadow
    )
endif()

install(TARGETS ascii-aquarium RUNTIME DESTINATION bin)
