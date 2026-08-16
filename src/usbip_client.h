#pragma once

#include <string>
#include <vector>
#include "usbip_protocol.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
typedef int SOCKET;
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif

class USBIPClient {
public:
    USBIPClient();
    ~USBIPClient();

    bool Connect(const std::string& host, const std::string& port);
    void Disconnect();
    bool ListDevices();

private:
    SOCKET client_socket_;
    bool is_connected_;

    bool InitializeSockets();
    void CleanupSockets();
};
