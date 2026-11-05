cmake_minimum_required(VERSION 3.16)
project(gm_bytecode LANGUAGES CXX VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# GMod binary-module targets: linux32, linux64, win32, win64
set(GM_BYTECODE_TARGET "auto" CACHE STRING
    "GMod module target: auto, linux32, linux64, win32, win64")
set_property(CACHE GM_BYTECODE_TARGET PROPERTY STRINGS
    auto linux32 linux64 win32 win64)

if(GM_BYTECODE_TARGET STREQUAL "auto")
    if(WIN32 OR CMAKE_SYSTEM_NAME STREQUAL "Windows")
        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
            set(GM_BYTECODE_TARGET_RESOLVED "win64")
        else()
            set(GM_BYTECODE_TARGET_RESOLVED "win32")
        endif()
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux" OR CMAKE_SYSTEM_NAME MATCHES ".*Linux.*")
        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
            set(GM_BYTECODE_TARGET_RESOLVED "linux64")
        else()
            set(GM_BYTECODE_TARGET_RESOLVED "linux32")
        endif()
    else()
        message(FATAL_ERROR
            "Unsupported host '${CMAKE_SYSTEM_NAME}'. "
            "Set -DGM_BYTECODE_TARGET=linux32|linux64|win32|win64")
    endif()
else()
    set(GM_BYTECODE_TARGET_RESOLVED "${GM_BYTECODE_TARGET}")
endif()

if(GM_BYTECODE_TARGET_RESOLVED STREQUAL "linux32")
    set(GM_BYTECODE_PLATFORM_SUFFIX "linux")
    set(GM_BYTECODE_IS_WINDOWS FALSE)
elseif(GM_BYTECODE_TARGET_RESOLVED STREQUAL "linux64")
    set(GM_BYTECODE_PLATFORM_SUFFIX "linux64")
    set(GM_BYTECODE_IS_WINDOWS FALSE)
elseif(GM_BYTECODE_TARGET_RESOLVED STREQUAL "win32")
    set(GM_BYTECODE_PLATFORM_SUFFIX "win32")
    set(GM_BYTECODE_IS_WINDOWS TRUE)
elseif(GM_BYTECODE_TARGET_RESOLVED STREQUAL "win64")
    set(GM_BYTECODE_PLATFORM_SUFFIX "win64")
    set(GM_BYTECODE_IS_WINDOWS TRUE)
else()
    message(FATAL_ERROR
        "Invalid GM_BYTECODE_TARGET='${GM_BYTECODE_TARGET_RESOLVED}'. "
        "Expected auto, linux32, linux64, win32, or win64")
endif()

include(CheckIPOSupported)
check_ipo_supported(RESULT bytecode_ipo_supported OUTPUT bytecode_ipo_error)

function(add_bytecode_module prefix is_client)
    set(target_name "bytecode_${prefix}")
    set(module_name "${prefix}_bytecode_${GM_BYTECODE_PLATFORM_SUFFIX}")

    message(STATUS "bytecode module: ${module_name}.dll")

    add_library(${target_name} SHARED
        src/bytecode.cpp
        src/lua_api.cpp
    )

    target_include_directories(${target_name} PRIVATE src)
    target_compile_definitions(${target_name} PRIVATE
        "GM_BYTECODE_TARGET_${GM_BYTECODE_TARGET_RESOLVED}=1"
    )

    if(is_client)
        target_compile_definitions(${target_name} PRIVATE GM_BYTECODE_CLIENT=1)
    else()
        target_compile_definitions(${target_name} PRIVATE GM_BYTECODE_SERVER=1)
    endif()

    set_target_properties(${target_name} PROPERTIES
        PREFIX ""
        OUTPUT_NAME "${module_name}"
        SUFFIX ".dll"
    )

    if(GM_BYTECODE_IS_WINDOWS)
        if(MSVC)
            set_property(TARGET ${target_name} PROPERTY
                MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
        elseif(MINGW OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            target_link_options(${target_name} PRIVATE
                -static-libgcc
                -static-libstdc++
                -static
            )
        endif()
    else()
        set_target_properties(${target_name} PROPERTIES
            CXX_VISIBILITY_PRESET hidden
            VISIBILITY_INLINES_HIDDEN ON
        )
        target_link_libraries(${target_name} PRIVATE ${CMAKE_DL_LIBS})
    endif()

    if(NOT MSVC)
        target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    if(bytecode_ipo_supported)
        set_property(TARGET ${target_name} PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
    endif()

    install(TARGETS ${target_name}
        RUNTIME DESTINATION lib
        LIBRARY DESTINATION lib
    )
endfunction()

# Server (gmsv_) and client (gmcl_) modules share the same sources.
add_bytecode_module(gmsv FALSE)
add_bytecode_module(gmcl TRUE)
