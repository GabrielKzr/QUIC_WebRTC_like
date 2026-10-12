# Percorre apenas diretorios ja configurados pelo CMake, sem inferir testes
# por nome de arquivo ou protocolo. Propriedades do target habilitam o runner.
function(_patchbay_collect_targets directory output)
    get_property(targets DIRECTORY "${directory}" PROPERTY BUILDSYSTEM_TARGETS)
    get_property(subdirectories DIRECTORY "${directory}" PROPERTY SUBDIRECTORIES)
    foreach(subdirectory IN LISTS subdirectories)
        _patchbay_collect_targets("${subdirectory}" child_targets)
        list(APPEND targets ${child_targets})
    endforeach()
    set(${output} "${targets}" PARENT_SCOPE)
endfunction()

function(register_program_tests)
    cmake_parse_arguments(PARSE_ARGV 0 PROGRAMS ""
        "RUNNER;ARTIFACTS_DIRECTORY" "")
    if(PROGRAMS_UNPARSED_ARGUMENTS OR PROGRAMS_KEYWORDS_MISSING_VALUES)
        message(FATAL_ERROR "Invalid register_program_tests arguments")
    endif()
    foreach(required IN ITEMS RUNNER ARTIFACTS_DIRECTORY)
        if(NOT DEFINED PROGRAMS_${required} OR "${PROGRAMS_${required}}" STREQUAL "")
            message(FATAL_ERROR "register_program_tests requires ${required}")
        endif()
    endforeach()

    _patchbay_collect_targets("${PROJECT_SOURCE_DIR}/source" targets)
    set(supported_modes tcp udp)
    foreach(target IN LISTS targets)
        get_property(enabled TARGET "${target}" PROPERTY PATCHBAY_MODE SET)
        if(NOT enabled)
            continue()
        endif()
        get_target_property(type "${target}" TYPE)
        if(NOT type STREQUAL "EXECUTABLE")
            message(FATAL_ERROR "PATCHBAY_MODE requires an executable: ${target}")
        endif()
        get_target_property(mode "${target}" PATCHBAY_MODE)
        if(NOT mode IN_LIST supported_modes)
            message(FATAL_ERROR "Invalid PATCHBAY_MODE for ${target}: ${mode}")
        endif()
        get_target_property(timeout "${target}" PATCHBAY_TIMEOUT)
        if(NOT timeout MATCHES "^[1-9][0-9]*$")
            message(FATAL_ERROR "Set a positive PATCHBAY_TIMEOUT for ${target}")
        endif()
        get_target_property(name "${target}" PATCHBAY_TEST_NAME)
        if(NOT name)
            set(name "network_${target}")
        endif()
        get_target_property(labels "${target}" PATCHBAY_LABELS)
        if(NOT labels)
            set(labels network)
        endif()

        add_program_test(
            NAME "${name}"
            TARGET "${target}"
            RUNNER "${PROGRAMS_RUNNER}"
            ARGS "${mode}" "${PROJECT_SOURCE_DIR}/source"
                "${PROGRAMS_ARTIFACTS_DIRECTORY}/${name}"
            LABELS ${labels}
            TIMEOUT "${timeout}"
            RUN_SERIAL
        )
    endforeach()
endfunction()
