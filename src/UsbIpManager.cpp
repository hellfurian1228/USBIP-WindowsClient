#include "UsbIpManager.h"
#include "win_socket.h"
#include "Logger.h"
#include <mutex>
#include <vector>
#include <string>
#include <windows.h>

UsbIpManager::UsbIpManager() {
}

UsbIpManager::~UsbIpManager() {
}

DWORD UsbIpManager::Connect(
    const std::string& hostname,
    const std::string& service,
    const std::string& busid,
    const std::string& serial,
    bool once,
    int& outPort
) {
    std::lock_guard<std::mutex> lock(m_mutex);
    Logger::instance().log("INFO", "UsbIpManager::Connect: hostname=" + hostname + ", service=" + service + ", busid=" + busid);

    // Open the VHCI driver handle
    auto dev = usbip::vhci::open();
    if (!dev) {
        DWORD err = GetLastError();
        Logger::instance().log("ERROR", "UsbIpManager::Connect: vhci::open failed. Error: " + std::to_string(err));
        return err;
    }

    // Prepare attach arguments
    usbip::vhci::attach_args args;
    args.location.hostname = hostname;
    args.location.service = service;
    args.location.busid = busid;
    args.serial = serial;
    args.once = once;

    // Call attach
    int port = usbip::vhci::attach(dev.get(), args);
    if (port <= 0) {
        DWORD err = GetLastError();
        Logger::instance().log("ERROR", "UsbIpManager::Connect: vhci::attach failed. Error: " + std::to_string(err));
        return err;
    }

    outPort = port;
    Logger::instance().log("INFO", "UsbIpManager::Connect: Successfully attached to port " + std::to_string(port));
    return ERROR_SUCCESS;
}

DWORD UsbIpManager::Disconnect(int port) {
    std::lock_guard<std::mutex> lock(m_mutex);
    Logger::instance().log("INFO", "UsbIpManager::Disconnect: port=" + std::to_string(port));

    // Open the VHCI driver handle
    auto dev = usbip::vhci::open();
    if (!dev) {
        DWORD err = GetLastError();
        Logger::instance().log("ERROR", "UsbIpManager::Disconnect: vhci::open failed. Error: " + std::to_string(err));
        return err;
    }

    // Call detach
    if (!usbip::vhci::detach(dev.get(), port)) {
        DWORD err = GetLastError();
        Logger::instance().log("ERROR", "UsbIpManager::Disconnect: vhci::detach failed. Error: " + std::to_string(err));
        return err;
    }

    Logger::instance().log("INFO", "UsbIpManager::Disconnect: Successfully detached port " + std::to_string(port));
    return ERROR_SUCCESS;
}

DWORD UsbIpManager::ListDevices(
    const std::string& hostname,
    const std::string& service,
    std::vector<UsbIpDevice>& outDevices
) {
    std::lock_guard<std::mutex> lock(m_mutex);
    Logger::instance().log("INFO", "UsbIpManager::ListDevices: hostname=" + hostname + ", service=" + service);

    // Connect to the remote host
    auto sock = usbip::connect(hostname.c_str(), service.c_str());
    if (!sock) {
        DWORD err = WSAGetLastError();
        Logger::instance().log("ERROR", "UsbIpManager::ListDevices: usbip::connect failed. WSAError: " + std::to_string(err));
        return err;
    }

    std::vector<UsbIpDevice> devices;

    auto on_dev_cnt = [](int count) {
        Logger::instance().log("INFO", "UsbIpManager::ListDevices: Host reported " + std::to_string(count) + " exportable device(s).");
    };

    auto on_dev = [&devices](int idx, const usbip::usb_device& dev) {
        Logger::instance().log("INFO", "UsbIpManager::ListDevices: Device[" + std::to_string(idx) + "]: busid=" + dev.busid + ", path=" + dev.path + ", speed=" + std::to_string(dev.speed));
        UsbIpDevice d;
        d.device = dev;
        devices.push_back(std::move(d));
    };

    auto on_intf = [&devices](int dev_idx, const usbip::usb_device& dev, int idx, const usbip::usb_interface& intf) {
        (void)dev;
        Logger::instance().log("INFO", "UsbIpManager::ListDevices: Interface[" + std::to_string(idx) + "] for Device[" + std::to_string(dev_idx) + "]: class=" + std::to_string(intf.bInterfaceClass));
        if (dev_idx >= 0 && dev_idx < static_cast<int>(devices.size())) {
            devices[dev_idx].interfaces.push_back(intf);
        }
    };

    if (!usbip::enum_exportable_devices(sock.get(), on_dev, on_intf, on_dev_cnt)) {
        DWORD err = GetLastError();
        Logger::instance().log("ERROR", "UsbIpManager::ListDevices: enum_exportable_devices failed. Error: " + std::to_string(err));
        return err;
    }

    outDevices = std::move(devices);
    Logger::instance().log("INFO", "UsbIpManager::ListDevices: Successfully listed " + std::to_string(outDevices.size()) + " device(s).");
    return ERROR_SUCCESS;
}
