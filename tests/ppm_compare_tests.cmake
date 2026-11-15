file(MAKE_DIRECTORY "${OUT}")
file(WRITE "${OUT}/expected.ppm" "P6\n1 1\n255\nABC")
file(WRITE "${OUT}/scaled.ppm" "P6\n2 2\n255\nABCABCABCABC")
file(WRITE "${OUT}/corrupt.ppm" "P6\n2 2\n255\nABCABCABCABD")
execute_process(COMMAND "${COMPARE}" "${OUT}/expected.ppm" "${OUT}/scaled.ppm"
    "${OUT}/diff.ppm" --integer-scale RESULT_VARIABLE scaled)
execute_process(COMMAND "${COMPARE}" "${OUT}/expected.ppm" "${OUT}/corrupt.ppm"
    "${OUT}/diff.ppm" --integer-scale RESULT_VARIABLE corrupt)
execute_process(COMMAND "${COMPARE}" "${OUT}/expected.ppm" "${OUT}/scaled.ppm"
    RESULT_VARIABLE strict)
if(NOT scaled EQUAL 0 OR NOT corrupt EQUAL 1 OR NOT strict EQUAL 1)
    message(FATAL_ERROR "Integer scale must check every pixel; strict mode must reject size changes")
endif()
