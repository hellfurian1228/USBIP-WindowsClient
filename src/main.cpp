#include <QApplication>
#include <QMessageBox>
#include "MainWindow.h"
#include "Logger.h"
#include "DriverInstaller.h"

int main(int argc, char* argv[]) {
    Logger::installCrashHandler();
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    QApplication app(argc, argv);
    Logger::instance().init();
    Logger::instance().log("INFO", "USBIP Client application started.");

    if (!DriverInstaller::isDriverInstalled()) {
        Logger::instance().log("INFO", "Driver not found. Attempting installation...");
        if (DriverInstaller::installDriver(QCoreApplication::applicationDirPath())) {
            Logger::instance().log("INFO", "Driver installed successfully.");
            QMessageBox::information(nullptr, "Driver Installed", 
                "The USBIP drivers have been successfully installed on your system.");
        } else {
            Logger::instance().log("ERROR", "Driver installation failed.");
            QMessageBox::warning(nullptr, "Driver Installation Failed", 
                "The USBIP drivers could not be installed. Please ensure you run this application as Administrator.");
        }
    }

    MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}
