# --------------------------------------------------------
# BUILD CONFIGURATION
# --------------------------------------------------------

# NASM configuration
enable_language(ASM_NASM)
set(CMAKE_ASM_NASM_FLAGS -felf64)
set(CMAKE_ASM_NASM_COMPILE_OBJECT "<CMAKE_ASM_NASM_COMPILER> <INCLUDES> ${CMAKE_ASM_NASM_FLAGS} -o <OBJECT> <SOURCE>")

# SGX config
set(SGX_DEBUG_MODE 1)
set(SGX_SDK_PATH /opt/intel/sgxsdk)
set(SGX_SDK_LIB_PATH ${SGX_SDK_PATH}/lib64)
set(SGX_SDK_ENCLAVE_SIGNER ${SGX_SDK_PATH}/bin/x64/sgx_sign)
set(SGX_SDK_ENCLAVE_EDGER8R ${SGX_SDK_PATH}/bin/x64/sgx_edger8r)
set(SGX_C_GLOBAL_FLAGS -m64)
set(THREADS_PREFER_PTHREAD_FLAG ON)

# WAMR-related config
set(SGX_WAMR_SOURCE_FILES SGXWAMRWasmModule.cpp sgx_wamr_native_symbols.cpp)
file(GLOB SGX_WAMR_ASM_SOURCE_FILES SGXWAMRWasmModule.S)
set(SGX_WAMR_ENCLAVE_SOURCE_FILES sgx_wamr_enclave.cpp sgx_wamr_native_symbols_wrapper.cpp)
set(SGX_WAMR_URTS_OBJECTS ${CMAKE_CURRENT_BINARY_DIR}/sgx_wamr_enclave_u.o)
set(SGX_WAMR_ENCLAVE_TRTS_OBJECTS ${CMAKE_CURRENT_BINARY_DIR}/sgx_wamr_enclave_t.o)
set_source_files_properties(${SGX_WAMR_ASM_SOURCE_FILES} PROPERTIES LANGUAGE ASM_NASM)

# Debug message
message("-- SGX-WAMR Configuration:
 FAASM_SGX_WAMR_HEAP_SIZE: ${FAASM_SGX_WAMR_HEAP_SIZE}KB
 FAASM_SGX_WAMR_MODULE_ERROR_BUFFER_SIZE: ${FAASM_SGX_WAMR_MODULE_ERROR_BUFFER_SIZE}B
 FAASM_SGX_WAMR_INSTANCE_DEFAULT_HEAP_SIZE: ${FAASM_SGX_WAMR_INSTANCE_DEFAULT_HEAP_SIZE}B
 FAASM_SGX_WAMR_INSTANCE_DEFAULT_STACK_SIZE: ${FAASM_SGX_WAMR_INSTANCE_DEFAULT_STACK_SIZE}B")

add_definitions(
        -DFAASM_SGX_WAMR_HEAP_SIZE=${FAASM_SGX_WAMR_HEAP_SIZE}
        -DFAASM_SGX_WAMR_MODULE_ERROR_BUFFER_SIZE=${FAASM_SGX_WAMR_MODULE_ERROR_BUFFER_SIZE}
        -DFAASM_SGX_WAMR_INSTANCE_DEFAULT_HEAP_SIZE=${FAASM_SGX_WAMR_INSTANCE_DEFAULT_HEAP_SIZE}
        -DFAASM_SGX_WAMR_INSTANCE_DEFAULT_STACK_SIZE=${FAASM_SGX_WAMR_INSTANCE_DEFAULT_STACK_SIZE}
)

set(SGX_WAMR_ENCLAVE_C_FLAGS ${SGX_C_GLOBAL_FLAGS}
        -nostdinc
        -fvisibility=hidden
        -fpie
        -ffunction-sections
        -fdata-sections
        -fstack-protector-strong
        )

# --------------------------------------------------------
# INITIAL CHECKS
# --------------------------------------------------------

# Check for SGX SDK
find_package(Threads REQUIRED)
if (EXISTS ${SGX_SDK_PATH})
    message(STATUS "Found SGX-SDK: TRUE")
else ()
    message(STATUS "Found SGX-SDK: FALSE")
    message(FATAL_ERROR "SGX-SDK not installed in ${SGX_SDK_PATH}")
endif ()

# Check for SGX simulation
if (SGX_SIM_MODE)
    message(STATUS "Enclave will be build in simulation mode")
elseif (NOT SGX_SIM_MODE)
    message(STATUS "Enclave will be build in hardware mode")
endif ()

# Check for SGX debug
if (SGX_DEBUG_MODE)
    message(STATUS "Enclave will be build with debug information and -O0 optimization")
    set(SGX_C_GLOBAL_FLAGS ${SGX_C_GLOBAL_FLAGS} -O0 -g)
    set(SGX_WAMR_C_FLAGS ${SGX_C_GLOBAL_FLAGS} -fPIC -Wno-attributes -DDEBUG -UNDEBUG -UEDEBUG)
else ()
    message(STATUS "Enclave will be build without debug information and -O2 optimization")
    set(SGX_C_GLOBAL_FLAGS ${SGX_C_GLOBAL_FLAGS} -O2)
    set(SGX_WAMR_C_FLAGS ${SGX_C_GLOBAL_FLAGS} -fPIC -Wno-attributes)
endif ()

# --------------------------------------------------------
# ENCLAVE BUILD
# --------------------------------------------------------

# SGX WAMR enclave library
add_library(sgx_wamr_enclave SHARED ${SGX_WAMR_ENCLAVE_SOURCE_FILES})

set_target_properties(sgx_wamr_enclave PROPERTIES PREFIX "")
target_include_directories(sgx_wamr_enclave PRIVATE
        ${SGX_SDK_PATH}/include
        ${SGX_SDK_PATH}/include/tlibc
        ${SGX_SDK_PATH}/include/libcxx
        ${WAMR_ROOT_DIR}/core
        ${WAMR_ROOT_DIR}/core/shared/utils
        ${WAMR_ROOT_DIR}/core/shared/platform/linux-sgx
        )

target_link_directories(sgx_wamr_enclave PRIVATE ${SGX_SDK_LIB_PATH})
target_compile_options(sgx_wamr_enclave PRIVATE ${SGX_WAMR_ENCLAVE_C_FLAGS} -std=c++11 -nostdinc++)

target_link_options(sgx_wamr_enclave PRIVATE
        ${SGX_WAMR_ENCLAVE_TRTS_OBJECTS} ${SGX_C_GLOBAL_FLAGS}
        -Wl,--no-undefined -nostdlib -nodefaultlibs -nostartfiles -Bstatic -Bsymbolic
        -Wl,-pie,-eenclave_entry
        -Wl,--export-dynamic
        -Wl,--defsym,__ImageBase=0
        -Wl,--gc-sections
        -Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/sgx_wamr_enclave.lds
        )

if (SGX_SIM_MODE)
    target_link_libraries(sgx_wamr_enclave
            -Wl,--whole-archive sgx_trts_sim
            -Wl,--no-whole-archive
            -Wl,--start-group sgx_tstdc
            sgx_tcxx
            sgx_tcrypto
            sgx_tservice_sim
            wamr
            -Wl,--end-group
            )
elseif (NOT SGX_SIM_MODE)
    target_link_libraries(sgx_wamr_enclave
            -Wl,--whole-archive sgx_trts
            -Wl,--no-whole-archive
            -Wl,--start-group
            sgx_tstdc
            sgx_tcxx
            sgx_tcrypto
            sgx_tservice
            wamr
            -Wl,--end-group
            )
endif ()

# --------------------------------------------------------
# SGX WAMR SUPPORT
# --------------------------------------------------------

# SGX WAMR library
add_library(sgx_wamr STATIC ${SGX_WAMR_SOURCE_FILES} ${SGX_WAMR_ASM_SOURCE_FILES})
add_dependencies(sgx_wamr sgx_wamr_enclave storage wasm)

target_include_directories(sgx_wamr PUBLIC ${SGX_SDK_PATH}/include)
target_link_directories(sgx_wamr INTERFACE ${SGX_SDK_LIB_PATH})
target_compile_options(sgx_wamr PRIVATE ${SGX_WAMR_C_FLAGS} -std=c++11)

# Must use this fancy hack because sgx_wamr_enclave_u.o will be generated during sgx_wamr build process
set_target_properties(sgx_wamr PROPERTIES STATIC_LIBRARY_FLAGS ${SGX_WAMR_URTS_OBJECTS})

# Simulation-specific linking
if (SGX_SIM_MODE)
    target_link_libraries(sgx_wamr
            sgx_urts_sim
            sgx_uae_service_sim
            Threads::Threads
            storage
            wasm
            )
else (NOT SGX_SIM_MODE)
    target_link_libraries(sgx_wamr
            sgx_urts
            sgx_uae_service
            Threads::Threads
            storage
            wasm
            )
endif ()

# SGX runner
include_directories(${FAASM_INCLUDE_DIR}/sgx)
add_executable(sgx_runner sgx_runner.cpp)

target_compile_options(sgx_runner PRIVATE -O3 -fpic -Wno-attributes)
target_link_libraries(sgx_runner PRIVATE sgx_wamr)

# --------------------------------------------------------
# CUSTOM COMMANDS
# --------------------------------------------------------

# WAMR enclave
add_custom_command(TARGET sgx_wamr_enclave PRE_BUILD
        COMMAND ${SGX_SDK_ENCLAVE_EDGER8R} --trusted sgx_wamr_enclave.edl
        --search-path /usr/local/code/faasm/src/sgx
        --search-path ${SGX_SDK_PATH}/include
        )

add_custom_command(TARGET sgx_wamr_enclave PRE_BUILD
        COMMAND gcc ${SGX_WAMR_ENCLAVE_C_FLAGS}
        -I${SGX_SDK_PATH}/include
        -I${SGX_SDK_PATH}/include/tlibc
        -c sgx_wamr_enclave_t.c
        -o sgx_wamr_enclave_t.o
        )

add_custom_command(TARGET sgx_wamr_enclave POST_BUILD
        COMMAND ${SGX_SDK_ENCLAVE_SIGNER} sign
        -key ${CMAKE_CURRENT_SOURCE_DIR}/sgx_wamr_enclave.pem
        -enclave ${CMAKE_BINARY_DIR}/lib/sgx_wamr_enclave.so
        -out ${CMAKE_BINARY_DIR}/bin/sgx_wamr_enclave.sign.so
        -config ${CMAKE_CURRENT_SOURCE_DIR}/sgx_wamr_enclave.config
        )

# SGX-WAMR
add_custom_command(TARGET sgx_wamr PRE_BUILD
        COMMAND ${SGX_SDK_ENCLAVE_EDGER8R} --untrusted sgx_wamr_enclave.edl
        --search-path /usr/local/code/faasm/src/sgx
        --search-path ${SGX_SDK_PATH}/include
        )

add_custom_command(TARGET sgx_wamr PRE_BUILD
        COMMAND gcc ${SGX_WAMR_C_FLAGS}
        -I${SGX_SDK_PATH}/include
        -c sgx_wamr_enclave_u.c
        -o sgx_wamr_enclave_u.o
        )
