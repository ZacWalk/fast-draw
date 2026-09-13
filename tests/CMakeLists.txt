add_executable(fast-draw-tests
    test_main.cpp
    test_framework.h
    test_json.cpp
    test_geometry.cpp
    test_document.cpp
    test_render.cpp
    test_png.cpp
)

target_link_libraries(fast-draw-tests PRIVATE fast-draw-lib)

add_test(NAME fast-draw-tests COMMAND fast-draw-tests)
set_tests_properties(fast-draw-tests PROPERTIES LABELS fast-draw)
