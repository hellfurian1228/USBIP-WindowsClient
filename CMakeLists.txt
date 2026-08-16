cmake_minimum_required(VERSION 3.16)

project(USBIPClient CXX)

# Enforce target platforms strictly to Windows x64 and Windows ARM64
if(NOT WIN32)
    message(FATAL_ERROR "This project only supports Windows.")
endif()

if(CMAKE_SYSTEM_PROCESSOR STREQUAL "AMD64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "ARM64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
    # Valid target platform
else()
    # Fallback check using CMAKE_SIZEOF_VOID_P if CMAKE_SYSTEM_PROCESSOR is not populated as expected
    if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
        message(FATAL_ERROR "Target platform must be strictly Windows x64 or Windows ARM64. Detected processor: ${CMAKE_SYSTEM_PROCESSOR}")
    endif()
endif()

# Enforce C++20 (with C++23 features enabled via compiler options if needed, or standard set to 23 to support the original source files)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Dynamically link the C++ runtime (MD/MDd) to match Qt and prevent heap assertions
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

# Configure Qt 6 integration
if(NOT CMAKE_PREFIX_PATH)
    list(APPEND CMAKE_PREFIX_PATH "C:/Qt/6.11.1/msvc2022_64")
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Custom command to compile messages.mc using Message Compiler (mc.exe)
find_program(MC_EXE mc.exe)
if(NOT MC_EXE)
    set(MC_EXE mc.exe)
endif()

# Create the output directory for messages.h
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/gen/resources")

add_custom_command(
    OUTPUT 
        "${CMAKE_BINARY_DIR}/gen/resources/messages.h"
        "${CMAKE_BINARY_DIR}/gen/resources/messages.rc"
    COMMAND ${MC_EXE} -c -n -r "${CMAKE_BINARY_DIR}/gen/resources" -h "${CMAKE_BINARY_DIR}/gen/resources" "${CMAKE_CURRENT_SOURCE_DIR}/userspace/resources/messages.mc"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/userspace/resources/messages.mc"
    COMMENT "Compiling messages.mc"
)

# Create a custom target for the message compiler output
add_custom_target(generate_messages DEPENDS "${CMAKE_BINARY_DIR}/gen/resources/messages.h")

# Include the usbip_core subdirectory
add_subdirectory(usbip_core)

# Source files for the main executable
set(SOURCES
    src/main.cpp
    src/usbip_client.cpp
    src/usbip_client.h
    src/usbip_protocol.h
    src/UsbIpManager.cpp
    src/UsbIpManager.h
    src/MainWindow.cpp
    src/MainWindow.h
    src/Logger.cpp
    src/Logger.h
    src/DriverInstaller.cpp
    src/DriverInstaller.h
)

# Create the single executable
add_executable(USBIPClient ${SOURCES})

# Preprocessor definitions
target_compile_definitions(USBIPClient PRIVATE USBIP_EXPORTS)

# Enable high warning levels and treat warnings as errors
if(MSVC)
    target_compile_options(USBIPClient PRIVATE /W4 /WX)
endif()

# Embed UAC manifest to require Administrator privileges on MSVC
if(MSVC)
    target_link_options(USBIPClient PRIVATE "/MANIFESTUAC:level='requireAdministrator' uiAccess='false'")
endif()

# Include directories
target_include_directories(USBIPClient PRIVATE 
    src
    include
    "${CMAKE_BINARY_DIR}/gen"
)

# Link necessary libraries
target_link_libraries(USBIPClient PRIVATE
    usbip_core
    ws2_32
    Version
    CfgMgr32
    setupapi
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

# Add dependency on generated messages
add_dependencies(USBIPClient generate_messages)

# Find windeployqt tool
find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "C:/Qt/6.11.1/msvc2022_64/bin")
if(WINDEPLOYQT_EXECUTABLE)
    add_custom_command(TARGET USBIPClient POST_BUILD
        COMMAND ${WINDEPLOYQT_EXECUTABLE} $<TARGET_FILE:USBIPClient>
        COMMENT "Deploying Qt dependencies..."
    )
endif()

