cmake_minimum_required(VERSION 3.5...3.10)
project(WaterSimulation)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/modules/" ${CMAKE_MODULE_PATH})

# Adds Corrade as a subproject
add_subdirectory(external/corrade EXCLUDE_FROM_ALL)

# Add Magnum as a subproject, enable Sdl2Application
set(MAGNUM_WITH_SDL2APPLICATION ON CACHE BOOL "" FORCE)
add_subdirectory(external/magnum EXCLUDE_FROM_ALL)

add_subdirectory(src)
