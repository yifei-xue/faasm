include_directories(
        ${FAASM_INCLUDE_DIR}/state
        ${FAASM_WAVM_SOURCE_DIR}
)

file(GLOB HEADERS "${FAASM_INCLUDE_DIR}/state/*.h")

set(LIB_FILES
        InMemoryStateKeyValue.cpp
        State.cpp
        StateKeyValue.cpp
        StateMessage.cpp
        StateServer.cpp
        RedisStateKeyValue.cpp
        ${HEADERS}
    )

faasm_private_lib(state "${LIB_FILES}")
target_link_libraries(state proto tcp redis)
