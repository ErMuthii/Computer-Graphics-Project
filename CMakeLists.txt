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

include(CTest)
if(BUILD_TESTING)
    add_executable(foundation_tests tests/foundation_tests.cpp)
    target_link_libraries(foundation_tests PRIVATE kitenge_foundation)
    kitenge_warnings(foundation_tests)
    add_test(NAME foundation COMMAND foundation_tests
        ${CMAKE_CURRENT_BINARY_DIR}/test-output/foundation.ppm)
endif()
