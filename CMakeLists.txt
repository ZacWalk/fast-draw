cmake_minimum_required(VERSION 3.24)

project(fast-draw
    VERSION 1.0.0
    DESCRIPTION "fast-draw - a small vector drawing program"
    LANGUAGES CXX)

if(NOT WIN32)
    message(FATAL_ERROR "fast-draw is a Win32 SDK application and only builds on Windows.")
endif()

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "fast-draw is 64-bit only; configure with an x64 toolchain.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()

# ---------------------------------------------------------------------------
# Third-party: zlib and libpng, fetched and built from source.
# ---------------------------------------------------------------------------
include(FetchContent)

set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)

set(ZLIB_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(ZLIB_BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(ZLIB_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(ZLIB_INSTALL OFF CACHE BOOL "" FORCE)
set(SKIP_INSTALL_ALL ON CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(zlib
    GIT_REPOSITORY https://github.com/madler/zlib.git
    GIT_TAG v1.3.1
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(zlib)

# zconf.h is generated into the build tree, so both directories are needed.
target_include_directories(zlibstatic PUBLIC "${zlib_SOURCE_DIR}" "${zlib_BINARY_DIR}")
add_library(ZLIB::ZLIB ALIAS zlibstatic)

# libpng locates zlib through the classic FindZLIB variables.
set(ZLIB_INCLUDE_DIR "${zlib_SOURCE_DIR}" "${zlib_BINARY_DIR}" CACHE STRING "" FORCE)
set(ZLIB_INCLUDE_DIRS "${zlib_SOURCE_DIR}" "${zlib_BINARY_DIR}" CACHE STRING "" FORCE)
set(ZLIB_LIBRARY ZLIB::ZLIB CACHE STRING "" FORCE)
set(ZLIB_LIBRARIES ZLIB::ZLIB CACHE STRING "" FORCE)
set(ZLIB_FOUND TRUE CACHE BOOL "" FORCE)

set(PNG_SHARED OFF CACHE BOOL "" FORCE)
set(PNG_STATIC ON CACHE BOOL "" FORCE)
set(PNG_TESTS OFF CACHE BOOL "" FORCE)
set(PNG_TOOLS OFF CACHE BOOL "" FORCE)
set(PNG_FRAMEWORK OFF CACHE BOOL "" FORCE)
set(PNG_BUILD_ZLIB OFF CACHE BOOL "" FORCE)

FetchContent_Declare(libpng
    GIT_REPOSITORY https://github.com/pnggroup/libpng.git
    GIT_TAG v1.6.44
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(libpng)

# zlib and libpng are third-party; do not hold them to our warning level.
foreach(third_party zlibstatic png_static)
    if(TARGET ${third_party})
        set_target_properties(${third_party} PROPERTIES FOLDER "third-party")
        target_compile_options(${third_party} PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/W0>)
    endif()
endforeach()

add_library(fast-draw-png INTERFACE)
target_link_libraries(fast-draw-png INTERFACE png_static ZLIB::ZLIB)
target_include_directories(fast-draw-png INTERFACE
    "${libpng_SOURCE_DIR}"
    "${libpng_BINARY_DIR}"
    "${zlib_SOURCE_DIR}"
    "${zlib_BINARY_DIR}"
)

# ---------------------------------------------------------------------------
# Everything except the entry point lives in a static library so the tests can
# link against the same code the application uses.
# ---------------------------------------------------------------------------
add_library(fast-draw-lib STATIC
    src/canvas.cpp
    src/canvas.h
    src/color_popup.cpp
    src/color_popup.h
    src/document.cpp
    src/document.h
    src/draw_obj.cpp
    src/draw_obj.h
    src/file_io.cpp
    src/file_io.h
    src/geometry.h
    src/json.cpp
    src/json.h
    src/main_window.cpp
    src/main_window.h
    src/png_export.cpp
    src/png_export.h
    src/render.cpp
    src/render.h
    src/resource.h
    src/shape_palette.cpp
    src/shape_palette.h
    src/shapes.cpp
    src/shapes.h
    src/style.cpp
    src/style.h
    src/style_panel.cpp
    src/style_panel.h
    src/tab_bar.cpp
    src/tab_bar.h
    src/ui.cpp
    src/ui.h
    src/utf8.cpp
    src/utf8.h
    src/view.cpp
    src/view.h
)

target_include_directories(fast-draw-lib PUBLIC src)

target_compile_definitions(fast-draw-lib PUBLIC
    UNICODE
    _UNICODE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    STRICT
)

if(MSVC)
    target_compile_options(fast-draw-lib PUBLIC
        /W4
        /permissive-
        /utf-8
        /EHsc
        /Zc:__cplusplus
        /Zc:preprocessor
    )
endif()

target_link_libraries(fast-draw-lib PUBLIC
    fast-draw-png
    comdlg32
    gdi32
    user32
    shell32
    msimg32
)

add_executable(fast-draw WIN32
    src/main.cpp
    src/fast-draw.rc
    src/fast-draw.manifest
)

target_link_libraries(fast-draw PRIVATE fast-draw-lib)

if(MSVC)
    # The .rc uses the same UNICODE settings as the C++ sources.
    target_compile_definitions(fast-draw PRIVATE $<$<COMPILE_LANGUAGE:RC>:UNICODE;_UNICODE>)
endif()

set_target_properties(fast-draw PROPERTIES OUTPUT_NAME "fast-draw")

add_subdirectory(tests)
