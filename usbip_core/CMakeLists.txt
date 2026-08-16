cmake_minimum_required(VERSION 3.15)

project(usbip_core CXX)

# Enforce C++20 (with C++23 features enabled via compiler options if needed, or standard set to 23 to support the original source files)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files
set(USBIP_CORE_SOURCES
    src/device_speed.cpp
    src/device_speed.h
    src/dllmain.cpp
    src/file_ver.cpp
    src/file_ver.h
    src/format_message.cpp
    src/hkey.h
    src/last_error.h
    src/op_common.h
    src/output.cpp
    src/output.h
    src/persistent.cpp
    src/proto_op.cpp
    src/remote.cpp
    src/setupapi.h
    src/strconv.cpp
    src/strconv.h
    src/usb_ids.cpp
    src/usb_ids.h
    src/vhci.cpp
    src/win_socket.cpp
)

# Headers in the parent directory
set(USBIP_CORE_HEADERS
    dllspec.h
    format_message.h
    generic_handle.h
    generic_handle_ex.h
    output.h
    persistent.h
    remote.h
    resource.h
    vhci.h
    win_handle.h
    win_socket.h
)

add_library(usbip_core STATIC ${USBIP_CORE_SOURCES} ${USBIP_CORE_HEADERS})

# Preprocessor definitions
target_compile_definitions(usbip_core PRIVATE 
    USBIP_EXPORTS
    _WINDOWS
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    UNICODE
    _UNICODE
)

# Include directories
target_include_directories(usbip_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/gen
)
target_include_directories(usbip_core PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Enable high warning levels and treat warnings as errors
if(MSVC)
    target_compile_options(usbip_core PRIVATE /W4 /WX)
endif()

# Link libraries
target_link_libraries(usbip_core PUBLIC
    ws2_32
    Version
    CfgMgr32
)

# Add dependency on generated messages
add_dependencies(usbip_core generate_messages)
