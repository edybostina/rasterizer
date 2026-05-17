cmake_minimum_required(VERSION 3.15)
project(rasterizer VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler flags
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Find SDL2
find_package(SDL2 QUIET)

if(NOT SDL2_FOUND)
    # Try using pkg-config as fallback
    find_package(PkgConfig)
    if(PkgConfig_FOUND)
        pkg_check_modules(SDL2 QUIET sdl2)
    endif()
    
    # Manual search as last resort
    if(NOT SDL2_FOUND)
        if(APPLE)
            # Check Homebrew paths on macOS
            set(SDL2_INCLUDE_HINTS
                /opt/homebrew/include
                /opt/homebrew/include/SDL2
                /usr/local/include
                /usr/local/include/SDL2
            )
            set(SDL2_LIBRARY_HINTS
                /opt/homebrew/lib
                /usr/local/lib
            )
        elseif(UNIX)
            # Check standard Linux paths
            set(SDL2_INCLUDE_HINTS
                /usr/include/SDL2
                /usr/local/include/SDL2
            )
            set(SDL2_LIBRARY_HINTS
                /usr/lib
                /usr/local/lib
                /usr/lib/x86_64-linux-gnu
            )
        endif()
        
        find_path(SDL2_INCLUDE_DIR
            NAMES SDL.h
            HINTS ${SDL2_INCLUDE_HINTS}
            PATH_SUFFIXES SDL2
        )
        
        find_library(SDL2_LIBRARY
            NAMES SDL2
            HINTS ${SDL2_LIBRARY_HINTS}
        )
        
        if(SDL2_INCLUDE_DIR AND SDL2_LIBRARY)
            set(SDL2_FOUND TRUE)
            set(SDL2_INCLUDE_DIRS ${SDL2_INCLUDE_DIR})
            set(SDL2_LIBRARIES ${SDL2_LIBRARY})
        endif()
    endif()
endif()

if(NOT SDL2_FOUND)
    message(FATAL_ERROR "SDL2 not found! Please install SDL2:\n"
                        "  macOS:   brew install sdl2\n"
                        "  Ubuntu:  sudo apt-get install libsdl2-dev\n"
                        "  Fedora:  sudo dnf install SDL2-devel\n"
                        "  Arch:    sudo pacman -S sdl2")
endif()

# Print SDL2 configuration
message(STATUS "SDL2 found:")
message(STATUS "  Include dirs: ${SDL2_INCLUDE_DIRS}")
message(STATUS "  Libraries: ${SDL2_LIBRARIES}")

# Create executable
add_executable(app
    src/main.cpp
    src/math.cpp
    src/object_loader.cpp
    src/image_creator.cpp
    src/util.cpp
    src/rasterizer.cpp
)

# Set target properties
set_target_properties(app PROPERTIES
    OUTPUT_NAME "app"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Include directories
target_include_directories(app PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${SDL2_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(app PRIVATE
    ${SDL2_LIBRARIES}
)

# Platform-specific settings
if(APPLE)
    # macOS specific settings
    target_link_libraries(app PRIVATE
        "-framework CoreFoundation"
    )
elseif(UNIX)
    # Linux specific settings
    target_link_libraries(app PRIVATE
        pthread
        dl
        m
    )
endif()

# Enable threading support
find_package(Threads REQUIRED)
target_link_libraries(app PRIVATE Threads::Threads)

# Installation rules
install(TARGETS app
    RUNTIME DESTINATION bin
)

# Copy resources to build directory (optional)
file(COPY ${CMAKE_SOURCE_DIR}/objects DESTINATION ${CMAKE_BINARY_DIR})
file(COPY ${CMAKE_SOURCE_DIR}/textures DESTINATION ${CMAKE_BINARY_DIR})

# Print build information
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "System: ${CMAKE_SYSTEM_NAME}")
