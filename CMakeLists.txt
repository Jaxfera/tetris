cmake_minimum_required(VERSION 3.1...3.13)

if(${CMAKE_VERSION} VERSION_LESS 3.12)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

project(tetris
    VERSION 0.5
    DESCRIPTION "Tetris clone badly written in C++ :)"
    LANGUAGES CXX
)

# Wrap compilation with ccache if available
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
endif()

# Add clang-tidy if available
if(CMAKE_VERSION VERSION_GREATER 3.6)
    option(CLANG_TIDY_FIX "Perform fixes for Clang-Tidy" OFF)
    find_program(CLANG_TIDY_EXE 
        NAMES "clang-tidy"
        DOC "Path to clang-tidy executable"
    )
    if(CLANG_TIDY_EXE)
        if(CLANG_TIDY_FIX)
            set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE}" "-fix")
        else()
            set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE}")
        endif()
    endif()
endif()

# Set build type to Release if not specified
set(default_build_type "Release")
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
    set(CMAKE_BUILD_TYPE "${default_build_type}"
        CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
                "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

find_package(Curses REQUIRED)
find_package(Threads REQUIRED)

add_executable(tetris "main.cpp")
target_link_libraries(tetris
    ${CURSES_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
)
target_include_directories(tetris PRIVATE "${CMAKE_SOURCE_DIR}")
target_compile_features(tetris PUBLIC cxx_std_17)
set_target_properties(tetris PROPERTIES CXX_EXTENSIONS OFF)

# Enable interprocedural optimization if available
include(CheckIPOSupported)
check_ipo_supported(RESULT result)
if(result)
    set_target_properties(tetris PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()
