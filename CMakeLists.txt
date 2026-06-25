cmake_minimum_required(VERSION 3.22)
project(MTGEssentiaPlugins VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(COPY_PLUGIN_AFTER_BUILD "Copy plugins after build" OFF)
set(CUSTOM_PLUGIN_INSTALL_DIR "" CACHE PATH "Override install dir")
set(PLUGIN_FORMATS VST3)    # semicolon‑separated list, e.g. "VST3;AU"

# Optional override for where Essentia was installed
set(ESSENTIA_ROOT "" CACHE PATH "Path to an existing Essentia install (optional)")

# User‑supplied plugin install dir (takes precedence over OS defaults)
set(CUSTOM_PLUGIN_INSTALL_DIR "" CACHE PATH
    "Custom directory to copy plugin into after build")

# Compiler flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        -Wall -Wextra -Wpedantic
        -Werror
        -Wreorder -Wdeprecated-declarations
        -Wunused -Wswitch -Wreturn-type
    )
elseif(MSVC)
    add_definitions(-DNOMINMAX -D_USE_MATH_DEFINES)
endif()

set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# ─── Dependencies ──────────────────────────────────────────────────────────
find_package(JUCE CONFIG REQUIRED)
set(JUCE_ENABLE_MODULE_SOURCE_GROUPS ON)
find_package(Eigen3 3.3 REQUIRED NO_MODULE)

# ---- Essentia discovery ---------------------------------------------------
# Prepend user‑supplied prefix (if any) so it wins over system paths
if(ESSENTIA_ROOT)
    list(PREPEND CMAKE_PREFIX_PATH "${ESSENTIA_ROOT}")
endif()

find_path(ESSENTIA_INCLUDE_DIR
          NAMES essentia/algorithm.h
          PATH_SUFFIXES include)

find_library(ESSENTIA_LIBRARY
             NAMES essentia
             PATH_SUFFIXES lib)

if(NOT ESSENTIA_INCLUDE_DIR OR NOT ESSENTIA_LIBRARY)
    message(FATAL_ERROR
        "Essentia not found. Either:\n"
        "  • run scripts/build-essentia-*.sh (recommended), or\n"
        "  • pass -DCMAKE_PREFIX_PATH=<essentia-prefix>, or\n"
        "  • set ESSENTIA_ROOT=<essentia-prefix>.")
endif()

# Create imported target for cleaner linking
add_library(essentia STATIC IMPORTED GLOBAL)
set_target_properties(essentia PROPERTIES
  IMPORTED_LOCATION "${ESSENTIA_LIBRARY}"
  INTERFACE_INCLUDE_DIRECTORIES "${ESSENTIA_INCLUDE_DIR}"
)

# ---- Plugin install destination ------------------------------------------
if(CUSTOM_PLUGIN_INSTALL_DIR)
    set(PLUGIN_INSTALL_DIR "${CUSTOM_PLUGIN_INSTALL_DIR}")
elseif(APPLE)
    set(PLUGIN_INSTALL_DIR "/Users/$ENV{USER}/Library/Audio/Plug-Ins/VST3")
elseif(WIN32)
    set(PLUGIN_INSTALL_DIR "C:/Program Files/Common Files/VST3")
else()
    set(PLUGIN_INSTALL_DIR "")  # no automatic copy on Linux by default
endif()

# ---- Sub‑projects ---------------------------------------------------------
add_subdirectory(plugins/Energy)
add_subdirectory(plugins/RMS)
add_subdirectory(plugins/Audio2Midi)
add_subdirectory(plugins/ZeroCrossingRate)
add_subdirectory(plugins/Onset2Midi)

# enable_testing()
# add_subdirectory(test)
