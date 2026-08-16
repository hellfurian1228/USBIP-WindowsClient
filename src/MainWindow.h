#pragma once

#include <QMainWindow>
#include <QLineEdit>
#include <QPushButton>
#include <QListWidget>
#include <QLabel>
#include <QProgressBar>
#include <map>
#include <string>
#include <vector>
#include "UsbIpManager.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void handleListDevices();
    void handleConnectDevice();
    void handleDisconnectDevice();
    void handleSelectionChanged();
    void handleShowDebugLog();

private:
    void setupUi();
    void applyStyleSheet();
    void updateStatus(const QString& message, bool active = false);
    void showError(const QString& message, DWORD errorCode);
    void refreshImportedDevices();

    // UI Elements
    QLineEdit* m_hostEdit;
    QLineEdit* m_portEdit;
    QPushButton* m_listButton;
    QListWidget* m_deviceList;
    QPushButton* m_connectButton;
    QPushButton* m_disconnectButton;
    QPushButton* m_debugLogButton;
    QLabel* m_statusLabel;
    QProgressBar* m_progressBar;

    // Backend Manager
    UsbIpManager m_manager;

    // Map of busid to active port number
    std::map<std::string, int> m_activeConnections;
    
    // List of currently displayed devices
    std::vector<UsbIpDevice> m_displayedDevices;
};
