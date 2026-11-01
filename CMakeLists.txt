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
# Shared libraries: libdesktop owns the Win32 window/reactor/presentation
# boundary; libfiles owns the PNG codec and its zlib dependency.
# ---------------------------------------------------------------------------
include(FetchContent)

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0091 NEW)
set(BUILD_SHARED_LIBS OFF)
set(BUILD_TESTING OFF)

set(FAST_DRAW_LIBDESKTOP_REVISION 31aae322af6e0696a9597726705b1121bd31580b)
set(FAST_DRAW_LIBFILES_REVISION 27de9d9fb67d7be7fcf9ff8a9f87dad5a91ae3eb)

if(NOT DEFINED FETCHCONTENT_SOURCE_DIR_LIBDESKTOP
   AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../libdesktop/CMakeLists.txt")
    set(FETCHCONTENT_SOURCE_DIR_LIBDESKTOP "${CMAKE_CURRENT_SOURCE_DIR}/../libdesktop"
        CACHE PATH "A libdesktop checkout to build instead of the pinned commit")
endif()

FetchContent_Declare(libdesktop
    GIT_REPOSITORY https://github.com/ZacWalk/libdesktop.git
    GIT_TAG        ${FAST_DRAW_LIBDESKTOP_REVISION}
    EXCLUDE_FROM_ALL)
FetchContent_MakeAvailable(libdesktop)

if(NOT DEFINED FETCHCONTENT_SOURCE_DIR_LIBFILES
   AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../libfiles/CMakeLists.txt")
    set(FETCHCONTENT_SOURCE_DIR_LIBFILES "${CMAKE_CURRENT_SOURCE_DIR}/../libfiles"
        CACHE PATH "A libfiles checkout to build instead of the pinned commit")
endif()

# fast-draw only encodes PNG. Keep the shared library's other codec recipes out
# of this build, including document codecs that fast-draw never opens.
set(LIBFILES_JPEG OFF)
set(LIBFILES_PNG ON)
set(LIBFILES_WEBP OFF)
set(LIBFILES_HEIF OFF)
set(LIBFILES_JXL OFF)
set(LIBFILES_RAW OFF)
set(LIBFILES_GIF OFF)
set(LIBFILES_TIFF OFF)

FetchContent_Declare(libfiles
    GIT_REPOSITORY https://github.com/ZacWalk/libfiles.git
    GIT_TAG        ${FAST_DRAW_LIBFILES_REVISION}
    EXCLUDE_FROM_ALL)
FetchContent_MakeAvailable(libfiles)

# ---------------------------------------------------------------------------
# Everything except the entry point lives in a static library so the tests can
# link against the same code the application uses.
# ---------------------------------------------------------------------------
add_library(fast-draw-lib STATIC
    src/canvas.cpp
    src/canvas.h
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
    src/print_backend.cpp
    src/print_backend.h
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
    libdesktop::core
    libfiles::libfiles
)

platform_add_app(fast-draw
    SOURCES
        src/main.cpp
    ICON        src/res/fast-draw.ico
    DESCRIPTION "fast-draw"
    OUTPUT_NAME fast-draw
)

target_link_libraries(fast-draw PRIVATE fast-draw-lib)

set_target_properties(fast-draw PROPERTIES OUTPUT_NAME "fast-draw")

add_subdirectory(tests)
