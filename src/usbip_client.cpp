#include "usbip_client.h"
#include <iostream>
#include <cstring>

#ifdef _WIN32
#pragma comment(lib, "Ws2_32.lib")
#endif

USBIPClient::USBIPClient() : client_socket_(INVALID_SOCKET), is_connected_(false) {
    InitializeSockets();
}

USBIPClient::~USBIPClient() {
    Disconnect();
    CleanupSockets();
}

bool USBIPClient::InitializeSockets() {
#ifdef _WIN32
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed with error: " << result << std::endl;
        return false;
    }
#endif
    return true;
}

void USBIPClient::CleanupSockets() {
#ifdef _WIN32
    WSACleanup();
#endif
}

bool USBIPClient::Connect(const std::string& host, const std::string& port) {
    if (is_connected_) {
        Disconnect();
    }

    struct addrinfo hints, *result = nullptr, *ptr = nullptr;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    int res = getaddrinfo(host.c_str(), port.c_str(), &hints, &result);
    if (res != 0) {
        std::cerr << "getaddrinfo failed with error: " << res << std::endl;
        return false;
    }

    for (ptr = result; ptr != nullptr; ptr = ptr->ai_next) {
        client_socket_ = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (client_socket_ == INVALID_SOCKET) {
            continue;
        }

        res = connect(client_socket_, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (res == SOCKET_ERROR) {
#ifdef _WIN32
            closesocket(client_socket_);
#else
            close(client_socket_);
#endif
            client_socket_ = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (client_socket_ == INVALID_SOCKET) {
        std::cerr << "Unable to connect to server!" << std::endl;
        return false;
    }

    is_connected_ = true;
    std::cout << "Connected to USBIP server at " << host << ":" << port << std::endl;
    return true;
}

void USBIPClient::Disconnect() {
    if (is_connected_) {
        if (client_socket_ != INVALID_SOCKET) {
#ifdef _WIN32
            closesocket(client_socket_);
#else
            close(client_socket_);
#endif
            client_socket_ = INVALID_SOCKET;
        }
        is_connected_ = false;
        std::cout << "Disconnected from server." << std::endl;
    }
}

// Helper to convert network byte order to host byte order for op_header
static void ntoh_op_header(op_header& header) {
    header.version = ntohs(header.version);
    header.command = static_cast<uint16_t>(ntohl(header.command));
    header.status = ntohl(header.status);
}

// Helper to convert host byte order to network byte order for op_header
static void hton_op_header(op_header& header) {
    header.version = htons(header.version);
    header.command = static_cast<uint16_t>(htonl(header.command));
    header.status = htonl(header.status);
}

bool USBIPClient::ListDevices() {
    if (!is_connected_) {
        std::cerr << "Not connected to a USBIP server." << std::endl;
        return false;
    }

    op_req_devlist req;
    req.header.version = USBIP_VERSION;
    req.header.command = static_cast<uint16_t>(USBIP_Command::OP_REQ_DEVLIST);
    req.header.status = 0;

    hton_op_header(req.header);

    int sent = send(client_socket_, reinterpret_cast<const char*>(&req), sizeof(req), 0);
    if (sent == SOCKET_ERROR) {
        std::cerr << "Failed to send OP_REQ_DEVLIST request." << std::endl;
        return false;
    }

    op_rep_devlist rep;
    int received = recv(client_socket_, reinterpret_cast<char*>(&rep), sizeof(rep), 0);
    if (received <= 0) {
        std::cerr << "Failed to receive OP_REP_DEVLIST response header." << std::endl;
        return false;
    }

    ntoh_op_header(rep.header);
    rep.ndev = ntohl(rep.ndev);

    if (rep.header.version != USBIP_VERSION || 
        rep.header.command != static_cast<uint32_t>(USBIP_Command::OP_REP_DEVLIST)) {
        std::cerr << "Invalid response header received." << std::endl;
        return false;
    }

    std::cout << "Found " << rep.ndev << " USB device(s):" << std::endl;

    for (uint32_t i = 0; i < rep.ndev; ++i) {
        usbip_usb_device dev;
        received = recv(client_socket_, reinterpret_cast<char*>(&dev), sizeof(dev), 0);
        if (received <= 0) {
            std::cerr << "Failed to receive device info." << std::endl;
            return false;
        }

        // Convert byte order for device fields
        dev.busnum = ntohl(dev.busnum);
        dev.devnum = ntohl(dev.devnum);
        dev.speed = ntohl(dev.speed);
        dev.idVendor = ntohs(dev.idVendor);
        dev.idProduct = ntohs(dev.idProduct);
        dev.bcdDevice = ntohs(dev.bcdDevice);

        std::cout << " - Bus ID: " << dev.busid << " (" << dev.path << ")" << std::endl;
        std::cout << "   Vendor ID: " << std::hex << dev.idVendor 
                  << ", Product ID: " << dev.idProduct << std::dec << std::endl;
        std::cout << "   Class: " << (int)dev.bDeviceClass 
                  << ", SubClass: " << (int)dev.bDeviceSubClass 
                  << ", Protocol: " << (int)dev.bDeviceProtocol << std::endl;

        // Read interfaces
        for (uint8_t j = 0; j < dev.bNumInterfaces; ++j) {
            usbip_usb_interface usb_interface;
            received = recv(client_socket_, reinterpret_cast<char*>(&usb_interface), sizeof(usb_interface), 0);
            if (received <= 0) {
                std::cerr << "Failed to receive interface info." << std::endl;
                return false;
            }
        }
    }

    return true;
}
