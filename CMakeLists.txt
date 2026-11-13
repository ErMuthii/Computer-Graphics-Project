cmake_minimum_required(VERSION 3.20)
project(KitengeProject VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_EXTENSIONS OFF)

function(kitenge_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

add_library(kitenge_foundation src/canvas.cpp src/ppm.cpp)
target_include_directories(kitenge_foundation PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(kitenge_foundation PUBLIC cxx_std_17)
kitenge_warnings(kitenge_foundation)

add_executable(kitenge_cp1 src/main.cpp)
target_link_libraries(kitenge_cp1 PRIVATE kitenge_foundation)
kitenge_warnings(kitenge_cp1)

add_library(kitenge_polygon_clip src/polygon_clip.cpp)
target_link_libraries(kitenge_polygon_clip PUBLIC kitenge_foundation)
kitenge_warnings(kitenge_polygon_clip)

add_executable(polygon_clip_demo src/polygon_clip_demo.cpp)
target_link_libraries(polygon_clip_demo PRIVATE kitenge_polygon_clip)
kitenge_warnings(polygon_clip_demo)

add_library(kitenge_lines src/lines.cpp)
target_link_libraries(kitenge_lines PUBLIC kitenge_foundation)
kitenge_warnings(kitenge_lines)

add_executable(lines_demo src/lines_demo.cpp)
target_link_libraries(lines_demo PRIVATE kitenge_lines)
kitenge_warnings(lines_demo)

add_library(kitenge_curves src/curves.cpp)
target_link_libraries(kitenge_curves PUBLIC kitenge_lines)
kitenge_warnings(kitenge_curves)

add_executable(bezier_demo src/bezier_demo.cpp)
target_link_libraries(bezier_demo PRIVATE kitenge_curves)
kitenge_warnings(bezier_demo)

add_library(kitenge_circles src/circles.cpp)
target_link_libraries(kitenge_circles PUBLIC kitenge_foundation)
kitenge_warnings(kitenge_circles)

add_executable(circles_demo src/circles_demo.cpp)
target_link_libraries(circles_demo PRIVATE kitenge_circles)
kitenge_warnings(circles_demo)

add_library(kitenge_polygon_fill src/polygon_fill.cpp)
target_link_libraries(kitenge_polygon_fill PUBLIC kitenge_foundation)
kitenge_warnings(kitenge_polygon_fill)

# Pure C++ scene and state remain usable without a window or OpenGL dependency.
add_library(kitenge_studio_core src/studio.cpp)
target_link_libraries(kitenge_studio_core PUBLIC kitenge_polygon_fill
    kitenge_polygon_clip kitenge_lines kitenge_circles)
kitenge_warnings(kitenge_studio_core)
# Reusable repeat-pattern renderer behind the fabric and garment hooks.
add_library(kitenge_pattern src/pattern.cpp)
target_link_libraries(kitenge_pattern PUBLIC kitenge_studio_core)
kitenge_warnings(kitenge_pattern)
add_executable(pattern_demo src/pattern_demo.cpp)
target_link_libraries(pattern_demo PRIVATE kitenge_pattern kitenge_curves)
kitenge_warnings(pattern_demo)
# Pixel comparison of two PPM images (desktop capture against headless render).
add_executable(ppm_compare src/ppm_compare.cpp)
target_link_libraries(ppm_compare PRIVATE kitenge_foundation)
kitenge_warnings(ppm_compare)

add_executable(studio_render src/studio_render.cpp)
target_link_libraries(studio_render PRIVATE kitenge_pattern)
kitenge_warnings(studio_render)

option(KITENGE_BUILD_DESKTOP "Build the GLFW/OpenGL desktop starter" OFF)
if(KITENGE_BUILD_DESKTOP)
    find_package(OpenGL REQUIRED)
    find_package(glfw3 3.3 CONFIG REQUIRED)
    add_executable(kitenge_studio src/desktop_main.cpp)
    target_link_libraries(kitenge_studio PRIVATE kitenge_pattern glfw OpenGL::GL)
    if(NOT APPLE)
        find_package(GLEW REQUIRED)
        target_link_libraries(kitenge_studio PRIVATE GLEW::GLEW)
    endif()
    kitenge_warnings(kitenge_studio)
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(studio_tests tests/studio_tests.cpp)
    target_link_libraries(studio_tests PRIVATE kitenge_studio_core)
    kitenge_warnings(studio_tests)
    add_test(NAME studio COMMAND studio_tests)
    add_executable(pattern_tests tests/pattern_tests.cpp)
    target_link_libraries(pattern_tests PRIVATE kitenge_pattern kitenge_curves)
    kitenge_warnings(pattern_tests)
    add_test(NAME pattern COMMAND pattern_tests)
    add_executable(foundation_tests tests/foundation_tests.cpp)
    target_link_libraries(foundation_tests PRIVATE kitenge_foundation)
    kitenge_warnings(foundation_tests)
    add_test(NAME foundation COMMAND foundation_tests
        ${CMAKE_CURRENT_BINARY_DIR}/test-output/foundation.ppm)
    
    add_executable(polygon_fill_tests tests/polygon_fill_tests.cpp)
    target_link_libraries(polygon_fill_tests PRIVATE kitenge_polygon_fill)
    kitenge_warnings(polygon_fill_tests)
    add_test(NAME polygon_fill COMMAND polygon_fill_tests)

    add_executable(polygon_clip_tests tests/polygon_clip_tests.cpp)
    target_link_libraries(polygon_clip_tests PRIVATE kitenge_polygon_clip)
    kitenge_warnings(polygon_clip_tests)
    add_test(NAME polygon_clip COMMAND polygon_clip_tests)

    add_executable(line_tests tests/line_tests.cpp)
    target_link_libraries(line_tests PRIVATE kitenge_lines)
    kitenge_warnings(line_tests)
    add_test(NAME lines COMMAND line_tests)

    add_executable(curve_tests tests/curve_tests.cpp)
    target_link_libraries(curve_tests PRIVATE kitenge_curves)
    kitenge_warnings(curve_tests)
    add_test(NAME curves COMMAND curve_tests)

    add_executable(circle_tests tests/circle_tests.cpp)
    target_link_libraries(circle_tests PRIVATE kitenge_circles)
    kitenge_warnings(circle_tests)
    add_test(NAME circles COMMAND circle_tests)

    # Opens real windows, so it needs a graphical session; off by default.
    option(KITENGE_GUI_TESTS "Compare desktop captures with headless renders" OFF)
    if(KITENGE_BUILD_DESKTOP AND KITENGE_GUI_TESTS)
        set(kitenge_scenes
            "default|"
            "transformed|--motif,2,--palette,1,--angle,20,--scale,0.9"
            "reflected|--motif,1,--palette,2,--angle,-35,--scale,1.3,--pitch,52,60,--translate,17,-9,--reflect-x")
        foreach(scene IN LISTS kitenge_scenes)
            # "name|comma,separated,options": commas avoid splitting the list here.
            string(FIND "${scene}" "|" bar)
            string(SUBSTRING "${scene}" 0 ${bar} scene_name)
            math(EXPR start "${bar} + 1")
            string(SUBSTRING "${scene}" ${start} -1 scene_options)
            add_test(NAME desktop_matches_headless_${scene_name}
                COMMAND ${CMAKE_COMMAND}
                    -DSTUDIO=$<TARGET_FILE:kitenge_studio>
                    -DRENDER=$<TARGET_FILE:studio_render>
                    -DCOMPARE=$<TARGET_FILE:ppm_compare>
                    -DOUT=${CMAKE_CURRENT_BINARY_DIR}/test-output/desktop
                    -DNAME=${scene_name}
                    -DSCENE=${scene_options}
                    -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/desktop_matches_headless.cmake)
            set_tests_properties(desktop_matches_headless_${scene_name} PROPERTIES RUN_SERIAL TRUE)
        endforeach()
        add_test(NAME desktop_smoke COMMAND kitenge_studio --smoke-test
            --capture ${CMAKE_CURRENT_BINARY_DIR}/test-output/desktop/smoke.ppm)
        set_tests_properties(desktop_smoke PROPERTIES RUN_SERIAL TRUE)
    endif()
endif()
