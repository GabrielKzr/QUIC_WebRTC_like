# ----------------------------------------------------------------------------
# add_protocol_lib(<lib_root>)
#
# Cria uma STATIC lib a partir de <lib_root>/src/*.c, expõe <lib_root>/inc/
# como include público e linka automaticamente contra udp_conn (HAL).
#
# Se existir <lib_root>/<libname>.meta.cmake, ele é incluído para ler
# variáveis de configuração extra. Variáveis suportadas (todas opcionais):
#
#   LIB_EXTRA_LIBS        — lista de libs/targets para target_link_libraries
#   LIB_EXTRA_INCLUDES    — lista de diretórios para target_include_directories
#   LIB_EXTRA_DEFINES     — lista de defines para target_compile_definitions
#   LIB_EXTRA_OPTIONS     — lista de flags para target_compile_options
# ----------------------------------------------------------------------------
function(add_protocol_lib lib_root)
    get_filename_component(_lib_name "${lib_root}" NAME)

    file(GLOB_RECURSE _sources "${lib_root}/src/*.c")
    if(NOT _sources)
        message(WARNING "[add_protocol_lib] '${_lib_name}': nenhum .c encontrado, pulando")
        return()
    endif()

    message(STATUS "[protocols] Adding lib: ${_lib_name}")
    add_library(${_lib_name} STATIC ${_sources})

    # Include público próprio
    set(_inc_dir "${lib_root}/inc")
    if(EXISTS "${_inc_dir}")
        target_include_directories(${_lib_name} PUBLIC "${_inc_dir}")
    endif()

    # Linka contra a HAL automaticamente (exceto ela mesma)
    if(NOT _lib_name STREQUAL "udp_conn")
        target_link_libraries(${_lib_name} PUBLIC udp_conn ${COMMON_DEPS})
    else()
        target_link_libraries(${_lib_name} PUBLIC ${COMMON_DEPS})
    endif()

    # Carrega .meta.cmake se existir — apenas sets de variáveis
    unset(LIB_EXTRA_LIBS)
    unset(LIB_EXTRA_INCLUDES)
    unset(LIB_EXTRA_DEFINES)
    unset(LIB_EXTRA_OPTIONS)

    set(_meta_file "${lib_root}/${_lib_name}.meta.cmake")
    if(EXISTS "${_meta_file}")
        message(STATUS "[protocols]   -> meta: ${_meta_file}")
        include("${_meta_file}")
    endif()

    if(DEFINED LIB_EXTRA_LIBS)
        target_link_libraries(${_lib_name} PUBLIC ${LIB_EXTRA_LIBS})
    endif()
    if(DEFINED LIB_EXTRA_INCLUDES)
        target_include_directories(${_lib_name} PUBLIC ${LIB_EXTRA_INCLUDES})
    endif()
    if(DEFINED LIB_EXTRA_DEFINES)
        target_compile_definitions(${_lib_name} PRIVATE ${LIB_EXTRA_DEFINES})
    endif()
    if(DEFINED LIB_EXTRA_OPTIONS)
        target_compile_options(${_lib_name} PRIVATE ${LIB_EXTRA_OPTIONS})
    endif()
endfunction()

# ----------------------------------------------------------------------------
# add_test_class(<class_name> <tests_root>)
#
# Varre <tests_root>/<class_name>/test_*/ procurando executáveis de teste.
# Cada subdiretório com um .c de mesmo nome vira um executável independente.
#
# Opções de build (passadas via -D):
#   -DBUILD_<CLASS>_TESTS=ON   builda toda a classe (ex: -DBUILD_WEBRTC_TESTS=ON)
#   -DTEST_NAME=<name>         builda apenas um teste específico (ex: -DTEST_NAME=test_chownat)
#                              (pode ser combinado com BUILD_<CLASS>_TESTS ou sozinho)
#
# .meta.cmake por teste — <test_dir>/<test_name>.meta.cmake
# Variáveis suportadas (todas opcionais):
#   TEST_EXTRA_LIBS        — libs/targets para linkar
#   TEST_EXTRA_INCLUDES    — diretórios de include adicionais
#   TEST_EXTRA_DEFINES     — defines de compilação
#   TEST_EXTRA_OPTIONS     — flags de compilação
# ----------------------------------------------------------------------------
function(add_test_class class_name tests_root)
    string(TOUPPER "${class_name}" _class_upper)
    set(_opt_name "BUILD_${_class_upper}_TESTS")
 
    # Garante que a opção exista com default OFF
    option(${_opt_name} "Build ${class_name} test class" OFF)
 
    # Resolve quais testes buildar
    set(_class_dir "${tests_root}/${class_name}")
 
    file(GLOB _test_dirs LIST_DIRECTORIES true "${_class_dir}/test_*")
 
    set(_tests_to_build)
    foreach(_dir IN LISTS _test_dirs)
        if(NOT IS_DIRECTORY "${_dir}")
            continue()
        endif()
 
        get_filename_component(_test_name "${_dir}" NAME)
 
        # Filtra por TEST_NAME se definido
        if(DEFINED TEST_NAME AND NOT "${TEST_NAME}" STREQUAL "${_test_name}")
            continue()
        endif()
 
        # Inclui se a classe toda foi pedida, ou se TEST_NAME bate exatamente
        if(${_opt_name} OR (DEFINED TEST_NAME AND "${TEST_NAME}" STREQUAL "${_test_name}"))
            list(APPEND _tests_to_build "${_dir}")
        endif()
    endforeach()
 
    if(NOT _tests_to_build)
        return()
    endif()
 
    message(STATUS "[tests/${class_name}] Building ${_class_upper} tests")
 
    foreach(_test_dir IN LISTS _tests_to_build)
        get_filename_component(_test_name "${_test_dir}" NAME)
 
        set(_src "${_test_dir}/${_test_name}.c")
        if(NOT EXISTS "${_src}")
            message(WARNING "[tests] '${_test_name}': source '${_src}' não encontrado, pulando")
            continue()
        endif()
 
        message(STATUS "[tests/${class_name}]   + ${_test_name}")
 
        add_executable(${_test_name} "${_src}")
 
        # .meta.cmake opcional
        unset(TEST_EXTRA_LIBS)
        unset(TEST_EXTRA_INCLUDES)
        unset(TEST_EXTRA_DEFINES)
        unset(TEST_EXTRA_OPTIONS)
 
        set(_meta "${_test_dir}/test.meta.cmake")
        if(EXISTS "${_meta}")
            message(STATUS "[tests/${class_name}]     -> meta: ${_meta}")
            include("${_meta}")
        endif()
 
        if(DEFINED TEST_EXTRA_LIBS)
            target_link_libraries(${_test_name} PRIVATE ${TEST_EXTRA_LIBS})
        endif()
        if(DEFINED TEST_EXTRA_INCLUDES)
            target_include_directories(${_test_name} PRIVATE ${TEST_EXTRA_INCLUDES})
        endif()
        if(DEFINED TEST_EXTRA_DEFINES)
            target_compile_definitions(${_test_name} PRIVATE ${TEST_EXTRA_DEFINES})
        endif()
        if(DEFINED TEST_EXTRA_OPTIONS)
            target_compile_options(${_test_name} PRIVATE ${TEST_EXTRA_OPTIONS})
        endif()
    endforeach()
endfunction()
