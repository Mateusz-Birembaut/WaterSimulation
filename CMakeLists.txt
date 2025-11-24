cmake_minimum_required(VERSION 3.5...3.10)
project(WaterSimulation)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/modules/" ${CMAKE_MODULE_PATH})

# Adds Corrade as a subproject
set(CORRADE_WITH_UTILITY ON CACHE BOOL "" FORCE)
set(CORRADE_WITH_RC ON CACHE BOOL "" FORCE)
add_subdirectory(external/corrade EXCLUDE_FROM_ALL)

# Enable corrade_add_resource
set(CORRADE_INCLUDE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
include(${PROJECT_SOURCE_DIR}/external/corrade/modules/UseCorrade.cmake)

# Add Magnum as a subproject, enable Sdl2Application
set(MAGNUM_WITH_SDL2APPLICATION ON CACHE BOOL "" FORCE)
add_subdirectory(external/magnum EXCLUDE_FROM_ALL)

# Magnum Integration
set(MAGNUM_WITH_IMGUIINTEGRATION ON CACHE BOOL "" FORCE)
set(IMGUI_DIR ${PROJECT_SOURCE_DIR}/external/imgui CACHE STRING "" FORCE) # Specify the location of imgui
add_subdirectory(external/magnum-integration EXCLUDE_FROM_ALL)

# Magnum Plugins
set(MAGNUM_WITH_STBIMAGEIMPORTER ON CACHE BOOL "" FORCE) # To import textures
set(MAGNUM_WITH_STBRESIZEIMAGECONVERTER ON CACHE BOOL "" FORCE) # To resize and convert textures
set(MAGNUM_BUILD_PLUGINS_STATIC ON CACHE BOOL "" FORCE) # Static linking 
add_subdirectory(external/magnum-plugins EXCLUDE_FROM_ALL)


add_subdirectory(src)

add_compile_options(O2)
add_compile_options(g)
