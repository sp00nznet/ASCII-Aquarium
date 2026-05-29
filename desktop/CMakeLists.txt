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

find_package(SDL2 REQUIRED)

set(AQ_SOURCES
    src/main.cpp
    src/renderer/Framebuffer.cpp
    src/renderer/Font.cpp
    src/sim/Aquarium.cpp
    src/sim/Background.cpp
    src/sim/Clock.cpp
)

add_executable(ascii-aquarium ${AQ_SOURCES})

target_include_directories(ascii-aquarium PRIVATE src assets)

if(TARGET SDL2::SDL2)
    target_link_libraries(ascii-aquarium PRIVATE SDL2::SDL2)
    if(TARGET SDL2::SDL2main)
        target_link_libraries(ascii-aquarium PRIVATE SDL2::SDL2main)
    endif()
else()
    target_include_directories(ascii-aquarium PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(ascii-aquarium PRIVATE ${SDL2_LIBRARIES})
endif()

if(MSVC)
    target_compile_options(ascii-aquarium PRIVATE /W4 /permissive-)
else()
    target_compile_options(ascii-aquarium PRIVATE
        -Wall -Wextra -Wpedantic -Wshadow
    )
endif()

install(TARGETS ascii-aquarium RUNTIME DESTINATION bin)
