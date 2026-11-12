# Checks that the OpenGL window shows exactly the canvas the headless renderer
# produces, at equal state and pattern settings.
#
# Run by CTest when KITENGE_GUI_TESTS=ON (needs a graphical session):
#   cmake -DSTUDIO=... -DRENDER=... -DCOMPARE=... -DOUT=... -DNAME=... -DSCENE=a,b,c -P this file
# SCENE is a comma-separated list of scene options passed to both programs.

foreach(var STUDIO RENDER COMPARE OUT NAME)
    if(NOT DEFINED ${var})
        message(FATAL_ERROR "${var} is not set")
    endif()
endforeach()
string(REPLACE "," ";" SCENE "${SCENE}")
file(MAKE_DIRECTORY "${OUT}")
set(desktop "${OUT}/${NAME}-desktop.ppm")
set(headless "${OUT}/${NAME}-headless.ppm")
set(diff "${OUT}/${NAME}-difference.ppm")
file(REMOVE "${desktop}" "${headless}" "${diff}")

execute_process(COMMAND "${STUDIO}" --capture "${desktop}" --exit-after-capture ${SCENE}
                RESULT_VARIABLE desktop_result OUTPUT_VARIABLE desktop_output ERROR_VARIABLE desktop_error)
if(NOT desktop_result EQUAL 0 OR NOT EXISTS "${desktop}")
    message(FATAL_ERROR "Desktop capture failed (${desktop_result}):\n${desktop_output}${desktop_error}")
endif()

execute_process(COMMAND "${RENDER}" "${headless}" ${SCENE}
                RESULT_VARIABLE render_result OUTPUT_VARIABLE render_output ERROR_VARIABLE render_error)
if(NOT render_result EQUAL 0)
    message(FATAL_ERROR "Headless render failed (${render_result}):\n${render_output}${render_error}")
endif()

execute_process(COMMAND "${COMPARE}" "${headless}" "${desktop}" "${diff}"
                RESULT_VARIABLE compare_result OUTPUT_VARIABLE compare_output ERROR_VARIABLE compare_error)
message(STATUS "${NAME}: ${compare_output}${compare_error}")
if(NOT compare_result EQUAL 0)
    message(FATAL_ERROR "Desktop frame differs from the headless render; see ${diff}")
endif()
