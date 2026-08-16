#pragma once

#include <QString>

class DriverInstaller {
public:
    /**
     * @brief Checks if the USBIP_win2 UDE driver is currently installed and active on the system.
     * Uses Windows SetupAPI to query if the hardware ID 'ROOT\USBIP_WIN2\UDE' is present.
     * @return true if the driver is installed and active, false otherwise.
     */
    static bool isDriverInstalled();

    /**
     * @brief Installs the USBIP_win2 UDE and filter drivers.
     * Locates the 'driver' folder relative to the application executable path,
     * installs the certificate, and then installs both drivers silently.
     * @param appDirPath The directory path of the application executable.
     * @return true if all installation steps completed successfully, false otherwise.
     */
    static bool installDriver(const QString& appDirPath);
};
