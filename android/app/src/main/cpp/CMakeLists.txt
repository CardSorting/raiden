cmake_minimum_required(VERSION 3.22.1)
project(sky_circuit_android LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(RAIDEN_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../../..")

# Required by raylib's CMake options for Android builds.
set(PLATFORM "Android" CACHE STRING "" FORCE)

include(FetchContent)
FetchContent_Declare(
  raylib
  GIT_REPOSITORY https://github.com/raysan5/raylib.git
  GIT_TAG 5.5
)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(raylib)

add_library(main SHARED
  "${RAIDEN_ROOT}/main.cpp"
  "${RAIDEN_ROOT}/Game.cpp"
  "${RAIDEN_ROOT}/AudioSystem.cpp"
  "${RAIDEN_ROOT}/Player.cpp"
  "${RAIDEN_ROOT}/Enemy.cpp"
  "${RAIDEN_ROOT}/Bullet.cpp"
  "${RAIDEN_ROOT}/Powerup.cpp"
  "${RAIDEN_ROOT}/StageDirector.cpp"
  "${RAIDEN_ROOT}/Effects.cpp"
  "${RAIDEN_ROOT}/SpriteManager.cpp"
)

target_include_directories(main PRIVATE "${RAIDEN_ROOT}")
target_link_libraries(main PRIVATE raylib android log m dl)
