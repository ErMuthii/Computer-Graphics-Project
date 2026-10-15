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

include(CTest)
if(BUILD_TESTING)
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
endif()
