#include "DriverInstaller.h"
#include "Logger.h"

#include <windows.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <newdev.h>
#include <tchar.h>

#include <vector>
#include <string>
#include <algorithm>

#include <QProcess>
#include <QDir>
#include <QFile>
#include <QStringList>
#include <QDebug>

// Link setupapi.lib, cfgmgr32.lib, and newdev.lib
#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "cfgmgr32.lib")
#pragma comment(lib, "newdev.lib")

namespace {
    /**
     * @brief Helper function to run a process silently, wait for it to finish, and log its output.
     * @param program The executable to run.
     * @param arguments The arguments to pass to the executable.
     * @param allowRebootCode If true, exit code 3010 (ERROR_SUCCESS_REBOOT_REQUIRED) is treated as success.
     * @return true if the process exited successfully, false otherwise.
     */
    bool runProcess(const QString& program, const QStringList& arguments, bool allowRebootCode = false) {
        Logger::instance().log("INFO", "Running command: " + program.toStdString() + " " + arguments.join(" ").toStdString());
        
        QProcess process;
        process.start(program, arguments);
        
        if (!process.waitForFinished(-1)) {
            Logger::instance().log("ERROR", "Failed to start or wait for process: " + program.toStdString());
            return false;
        }
        
        QByteArray stdOut = process.readAllStandardOutput();
        QByteArray stdErr = process.readAllStandardError();
        
        if (!stdOut.isEmpty()) {
            Logger::instance().log("INFO", program.toStdString() + " stdout: " + stdOut.trimmed().toStdString());
        }
        if (!stdErr.isEmpty()) {
            Logger::instance().log("WARN", program.toStdString() + " stderr: " + stdErr.trimmed().toStdString());
        }
        
        if (process.exitStatus() != QProcess::NormalExit) {
            Logger::instance().log("ERROR", "Process crashed: " + program.toStdString());
            return false;
        }
        
        int exitCode = process.exitCode();
        Logger::instance().log("INFO", "Process " + program.toStdString() + " exited with code " + std::to_string(exitCode));
        
        if (exitCode == 0) {
            return true;
        }
        if (allowRebootCode && exitCode == 3010) {
            Logger::instance().log("INFO", "Process " + program.toStdString() + " requires reboot (exit code 3010).");
            return true;
        }
        
        return false;
    }
}

bool DriverInstaller::isDriverInstalled() {
    Logger::instance().log("INFO", "Checking if USBIP_win2 UDE driver is installed...");
    
    HDEVINFO hDevInfo = SetupDiGetClassDevsW(nullptr, L"ROOT", nullptr, DIGCF_ALLCLASSES | DIGCF_PRESENT);
    if (hDevInfo == INVALID_HANDLE_VALUE) {
        Logger::instance().log("ERROR", "SetupDiGetClassDevsW failed.");
        return false;
    }
    
    SP_DEVINFO_DATA devInfoData;
    devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    
    bool found = false;
    for (DWORD i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &devInfoData); ++i) {
        DWORD regType = 0;
        DWORD requiredSize = 0;
        
        // Call first to get the buffer size
        SetupDiGetDeviceRegistryPropertyW(hDevInfo, &devInfoData, SPDRP_HARDWAREID, &regType, nullptr, 0, &requiredSize);
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER || requiredSize == 0) {
            continue;
        }
        
        std::vector<wchar_t> buffer(requiredSize / sizeof(wchar_t) + 1, L'\0');
        if (SetupDiGetDeviceRegistryPropertyW(hDevInfo, &devInfoData, SPDRP_HARDWAREID, &regType, reinterpret_cast<PBYTE>(buffer.data()), requiredSize, nullptr)) {
            // SPDRP_HARDWAREID is a REG_MULTI_SZ string list.
            const wchar_t* p = buffer.data();
            while (*p) {
                std::wstring hwid(p);
                // Convert to uppercase for case-insensitive comparison
                std::transform(hwid.begin(), hwid.end(), hwid.begin(), ::towupper);
                if (hwid == L"ROOT\\USBIP_WIN2\\UDE") {
                    found = true;
                    break;
                }
                p += hwid.length() + 1;
            }
        }
        if (found) {
            break;
        }
    }
    
    SetupDiDestroyDeviceInfoList(hDevInfo);
    
    if (found) {
        Logger::instance().log("INFO", "USBIP_win2 UDE driver is installed and active.");
    } else {
        Logger::instance().log("INFO", "USBIP_win2 UDE driver is NOT installed or NOT active.");
    }
    
    return found;
}

bool DriverInstaller::installDriver(const QString& appDirPath) {
    Logger::instance().log("INFO", "Starting driver installation process...");
    
    // Locate the 'driver' folder relative to the application executable path
    QString driverDir = QDir::toNativeSeparators(QDir(appDirPath).filePath("driver"));
    
    Logger::instance().log("INFO", "Driver directory: " + driverDir.toStdString());
    
    // Scan the 'driver' directory for any file ending with '.cer' or '.pfx'
    QDir dir(driverDir);
    QStringList filters;
    filters << "*.cer" << "*.pfx";
    QFileInfoList certFiles = dir.entryInfoList(filters, QDir::Files);
    
    if (certFiles.isEmpty()) {
        qCritical() << "Error: No certificate file (.cer or .pfx) found in driver directory:" << driverDir;
        Logger::instance().log("ERROR", "No certificate file (.cer or .pfx) found in driver directory: " + driverDir.toStdString());
        return false;
    }
    
    QFileInfo certFileInfo = certFiles.first();
    QString certPath = QDir::toNativeSeparators(certFileInfo.absoluteFilePath());
    QString certExtension = certFileInfo.suffix().toLower();
    
    qDebug() << "Found certificate file:" << certPath << "with extension:" << certExtension;
    Logger::instance().log("INFO", "Found certificate file: " + certPath.toStdString() + " (" + certExtension.toStdString() + ")");
    
    QString udeInfPath = QDir::toNativeSeparators(QDir(driverDir).filePath("usbip2_ude.inf"));
    QString filterInfPath = QDir::toNativeSeparators(QDir(driverDir).filePath("usbip2_filter.inf"));
    
    Logger::instance().log("INFO", "UDE INF path: " + udeInfPath.toStdString());
    Logger::instance().log("INFO", "Filter INF path: " + filterInfPath.toStdString());
    
    // Verify that driver INF files exist (Removed usbip.exe check entirely)
    if (!QFile::exists(udeInfPath)) {
        qCritical() << "Error: UDE driver INF file not found:" << udeInfPath;
        Logger::instance().log("ERROR", "UDE driver INF file not found: " + udeInfPath.toStdString());
        return false;
    }
    if (!QFile::exists(filterInfPath)) {
        qCritical() << "Error: Filter driver INF file not found:" << filterInfPath;
        Logger::instance().log("ERROR", "Filter driver INF file not found: " + filterInfPath.toStdString());
        return false;
    }
    
    // 1. Install the certificate silently
    bool certInstalled = false;
    if (certExtension == "pfx") {
        QStringList certArgs;
        certArgs << "-f" << "-p" << "usbip" << "-importpfx" << certPath;
        certInstalled = runProcess("certutil.exe", certArgs, false);
    } else if (certExtension == "cer") {
        QStringList certArgs;
        certArgs << "-add" << certPath << "-s" << "-r" << "localMachine" << "root";
        certInstalled = runProcess("certmgr.exe", certArgs, false);
    }
    
    if (!certInstalled) {
        qCritical() << "Error: Certificate installation failed for:" << certPath;
        Logger::instance().log("ERROR", "Certificate installation failed.");
        return false;
    }
    Logger::instance().log("INFO", "Certificate installed successfully.");
    
    // 2. Natively create the virtual hardware node and attach the driver
    Logger::instance().log("INFO", "Extracting Class GUID and creating virtual hardware node natively...");

    std::wstring infPathW = udeInfPath.toStdWString();

    // STEP A: Use the standard Windows USB Class GUID directly
    GUID classGuid = { 0x36fc9e60, 0xc465, 0x11cf, { 0x80, 0x56, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 } };
    wchar_t className[] = L"USB";

    // STEP B: Create a device info list strictly for this class
    HDEVINFO DeviceInfoSet = SetupDiCreateDeviceInfoList(&classGuid, nullptr);
    if (DeviceInfoSet == INVALID_HANDLE_VALUE) {
        qCritical() << "Error: SetupDiCreateDeviceInfoList failed:" << GetLastError();
        return false;
    }

    SP_DEVINFO_DATA DeviceInfoData;
    ZeroMemory(&DeviceInfoData, sizeof(SP_DEVINFO_DATA));
    DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    // STEP C: Create the device node
    if (!SetupDiCreateDeviceInfoW(DeviceInfoSet,
                                  className,
                                  &classGuid,
                                  nullptr,
                                  nullptr,
                                  DICD_GENERATE_ID,
                                  &DeviceInfoData)) {
        qCritical() << "Error: SetupDiCreateDeviceInfoW failed:" << GetLastError();
        SetupDiDestroyDeviceInfoList(DeviceInfoSet);
        return false;
    }

    // STEP D: Set the Hardware ID so the INF knows to attach here
    const wchar_t hwid[] = L"ROOT\\USBIP_WIN2\\UDE\0";
    if (!SetupDiSetDeviceRegistryPropertyW(DeviceInfoSet,
                                           &DeviceInfoData,
                                           SPDRP_HARDWAREID,
                                           (const BYTE*)hwid,
                                           sizeof(hwid))) {
        qCritical() << "Error: SetupDiSetDeviceRegistryPropertyW failed:" << GetLastError();
        SetupDiDestroyDeviceInfoList(DeviceInfoSet);
        return false;
    }

    // STEP E: Register the virtual device into the Windows Device tree
    if (!SetupDiCallClassInstaller(DIF_REGISTERDEVICE, DeviceInfoSet, &DeviceInfoData)) {
        qCritical() << "Error: SetupDiCallClassInstaller failed:" << GetLastError();
        SetupDiDestroyDeviceInfoList(DeviceInfoSet);
        return false;
    }

    Logger::instance().log("INFO", "Virtual node created. Binding INF driver package...");

    // STEP F: Point Windows to the INF to install the drivers onto the new node
    BOOL bRebootRequired = FALSE;
    if (!UpdateDriverForPlugAndPlayDevicesW(nullptr,
                                            L"ROOT\\USBIP_WIN2\\UDE",
                                            infPathW.c_str(),
                                            INSTALLFLAG_FORCE | INSTALLFLAG_NONINTERACTIVE,
                                            &bRebootRequired)) {
        DWORD err = GetLastError();
        qCritical() << "Error: UpdateDriverForPlugAndPlayDevicesW failed. Code:" << err;
        Logger::instance().log("ERROR", "UpdateDriverForPlugAndPlayDevicesW failed. Code: " + std::to_string(err));
        SetupDiDestroyDeviceInfoList(DeviceInfoSet);
        return false;
    }

    SetupDiDestroyDeviceInfoList(DeviceInfoSet);
    Logger::instance().log("INFO", "USBIP_win2 UDE driver installed natively and successfully.");
    return true;
}