cmake_minimum_required(VERSION 3.16)
project(SkyCircuit LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (CMAKE_SYSTEM_NAME STREQUAL "Android")
  # Ensure raylib uses Android backend when cross-compiling for phones.
  set(PLATFORM "Android" CACHE STRING "" FORCE)
endif()

find_package(raylib QUIET)
if (NOT raylib_FOUND)
  include(FetchContent)
  FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.5
  )
  set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(raylib)
endif()

set(SKY_CIRCUIT_SOURCES
  main.cpp
  Game.cpp
  AudioSystem.cpp
  Player.cpp
  Enemy.cpp
  Bullet.cpp
  Powerup.cpp
  StageDirector.cpp
  Effects.cpp
  SpriteManager.cpp
)

add_executable(sky_circuit ${SKY_CIRCUIT_SOURCES})


target_include_directories(sky_circuit PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_options(sky_circuit PRIVATE
  $<$<CXX_COMPILER_ID:AppleClang,Clang,GNU>:-Wall -Wextra -Wpedantic -Werror>
  $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
)

target_link_libraries(sky_circuit PRIVATE raylib)

if (APPLE)
  target_link_libraries(sky_circuit PRIVATE "-framework IOKit" "-framework Cocoa" "-framework OpenGL")
elseif (UNIX)
  if (CMAKE_SYSTEM_NAME STREQUAL "Android")
    target_link_libraries(sky_circuit PRIVATE m dl)
  else()
    target_link_libraries(sky_circuit PRIVATE m pthread dl rt X11)
  endif()
endif()
