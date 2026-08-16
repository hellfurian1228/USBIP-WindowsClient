#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include "win_socket.h"
#include <string>
#include <vector>
#include <mutex>
#include <windows.h>
#include "remote.h"
#include "vhci.h"

// Structure to hold a USB device and its interfaces
struct UsbIpDevice {
    usbip::usb_device device;
    std::vector<usbip::usb_interface> interfaces;
};

class UsbIpManager {
public:
    UsbIpManager();
    ~UsbIpManager();

    // Thread-safe method to connect to a remote USBIP server and attach a device.
    // Returns 0 (ERROR_SUCCESS) on success, or a Windows error code on failure.
    DWORD Connect(
        const std::string& hostname,
        const std::string& service,
        const std::string& busid,
        const std::string& serial,
        bool once,
        int& outPort
    );

    // Thread-safe method to detach a device from a specific port.
    // Returns 0 (ERROR_SUCCESS) on success, or a Windows error code on failure.
    DWORD Disconnect(int port);

    // Thread-safe method to list available USB devices on a remote host.
    // Returns 0 (ERROR_SUCCESS) on success, or a Windows error code on failure.
    DWORD ListDevices(
        const std::string& hostname,
        const std::string& service,
        std::vector<UsbIpDevice>& outDevices
    );

private:
    std::mutex m_mutex;
    usbip::InitWinSock2 m_ws2;
};
