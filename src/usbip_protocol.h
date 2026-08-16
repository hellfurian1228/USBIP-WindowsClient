#pragma once

#include <cstdint>

#pragma pack(push, 1)

// USBIP Protocol Version
constexpr uint16_t USBIP_VERSION = 0x0111;

// USBIP Commands
enum class USBIP_Command : uint32_t {
    OP_REQ_DEVLIST = 0x80050001,
    OP_REP_DEVLIST = 0x00050001,
    OP_REQ_IMPORT  = 0x80050003,
    OP_REP_IMPORT  = 0x00050003
};

// Header for OP_REQ commands
struct op_header {
    uint16_t version;
    uint16_t command;
    uint32_t status;
} ;

// OP_REQ_DEVLIST request
struct op_req_devlist {
    op_header header;
} ;

// OP_REP_DEVLIST response header
struct op_rep_devlist {
    op_header header;
    uint32_t ndev;
} ;

// USB Device description in OP_REP_DEVLIST
struct usbip_usb_device {
    char path[256];
    char busid[32];
    uint32_t busnum;
    uint32_t devnum;
    uint32_t speed;
    uint16_t idVendor;
    uint16_t idProduct;
    uint16_t bcdDevice;
    uint8_t bDeviceClass;
    uint8_t bDeviceSubClass;
    uint8_t bDeviceProtocol;
    uint8_t bConfigurationValue;
    uint8_t bNumConfigurations;
    uint8_t bNumInterfaces;
} ;

// USB Interface description in OP_REP_DEVLIST
struct usbip_usb_interface {
    uint8_t bInterfaceClass;
    uint8_t bInterfaceSubClass;
    uint8_t bInterfaceProtocol;
    uint8_t padding;
} ;

#pragma pack(pop)
