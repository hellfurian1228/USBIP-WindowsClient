# USBIPClient

A C++ Windows command-line client for the USBIP (USB over IP) protocol. This client connects to a USBIP server and queries the list of exported USB devices.

## Features

- TCP/IP connection to a USBIP server.
- Query and list exported USB devices (`OP_REQ_DEVLIST` / `OP_REP_DEVLIST`).
- Cross-platform socket abstraction (Winsock on Windows, standard POSIX sockets on Linux).

## Project Structure

- [CMakeLists.txt](CMakeLists.txt) - CMake configuration file.
- [src/main.cpp](src/main.cpp) - Entry point and command-line interface.
- [src/usbip_client.h](src/usbip_client.h) - USBIPClient class declaration.
- [src/usbip_client.cpp](src/usbip_client.cpp) - USBIPClient class implementation.
- [src/usbip_protocol.h](src/usbip_protocol.h) - USBIP protocol structures and constants.

## Requirements

- Windows 10/11 or Linux.
- Visual Studio 2022/2026 (with C++ CMake tools) or GCC/Clang with CMake.

## Building the Project

### Using VS Code Tasks

1. Open the command palette (`Ctrl+Shift+P`).
2. Run `Tasks: Run Build Task` and select `CMake Build`.

### Using Command Line

```powershell
# Configure the project
& "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" -B build -S .

# Build the project
& "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Debug
```

## Usage

Run the compiled executable with the host and port of the USBIP server:

```cmd
build\Debug\USBIPClient.exe <host> <port>
```

Example:
```cmd
build\Debug\USBIPClient.exe 192.168.1.100 3240
```
