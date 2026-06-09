# ----------------------------------------------------------------------------
# HAL — udp_conn (deve ser buildada antes das implementações)
# ----------------------------------------------------------------------------
add_protocol_lib("${CMAKE_CURRENT_SOURCE_DIR}/protocols/udp_conn")

# ----------------------------------------------------------------------------
# Implementações — cada subdiretório de protocols/ que não seja udp_conn
# ----------------------------------------------------------------------------
set(_protocol_impls)

file(GLOB _proto_dirs LIST_DIRECTORIES true "${CMAKE_CURRENT_SOURCE_DIR}/protocols/*")
foreach(_dir IN LISTS _proto_dirs)
    get_filename_component(_name "${_dir}" NAME)
    if(_name STREQUAL "udp_conn" OR NOT IS_DIRECTORY "${_dir}")
        continue()
    endif()

    add_protocol_lib("${_dir}")
    list(APPEND _protocol_impls ${_name})
endforeach()

# ----------------------------------------------------------------------------
# Target agregador — 'protocols'
# Linka todas as implementações; quem linkar contra 'protocols' puxa tudo.
# Usa $<TARGET_OBJECTS:...> pra não precisar de um .c dummy.
# ----------------------------------------------------------------------------
message(STATUS "[protocols] Aggregator target 'protocols' <- ${_protocol_impls}")

set(_all_objects)
foreach(_impl IN LISTS _protocol_impls)
    list(APPEND _all_objects $<TARGET_OBJECTS:${_impl}>)
endforeach()

add_library(protocols STATIC
    $<TARGET_OBJECTS:udp_conn>
    ${_all_objects}
)

target_link_libraries(protocols PUBLIC udp_conn ${_protocol_impls})