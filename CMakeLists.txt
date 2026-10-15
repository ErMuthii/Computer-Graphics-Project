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

include(CTest)
if(BUILD_TESTING)
    add_executable(foundation_tests tests/foundation_tests.cpp)
    target_link_libraries(foundation_tests PRIVATE kitenge_foundation)
    kitenge_warnings(foundation_tests)
    add_test(NAME foundation COMMAND foundation_tests
        ${CMAKE_CURRENT_BINARY_DIR}/test-output/foundation.ppm)
    
    add_library(kitenge_polygon_fill src/polygon_fill.cpp)
    target_link_libraries(kitenge_polygon_fill PUBLIC kitenge_foundation)
    kitenge_warnings(kitenge_polygon_fill)

    add_executable(polygon_fill_tests tests/polygon_fill_tests.cpp)
    target_link_libraries(polygon_fill_tests PRIVATE kitenge_polygon_fill)
    kitenge_warnings(polygon_fill_tests)
    add_test(NAME polygon_fill COMMAND polygon_fill_tests)

    add_executable(polygon_clip_tests tests/polygon_clip_tests.cpp)
    target_link_libraries(polygon_clip_tests PRIVATE kitenge_polygon_clip)
    kitenge_warnings(polygon_clip_tests)
    add_test(NAME polygon_clip COMMAND polygon_clip_tests)
endif()
