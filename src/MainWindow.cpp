#include "MainWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QMessageBox>
#include <QMetaObject>
#include <QDialog>
#include <QPlainTextEdit>
#include <QDialogButtonBox>
#include <thread>
#include "Logger.h"
#include "output.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent) {
    setupUi();
    applyStyleSheet();

    // Route libusbip debug output to our Logger
    libusbip::set_debug_output([](std::string msg) {
        while (!msg.empty() && (msg.back() == '\n' || msg.back() == '\r')) {
            msg.pop_back();
        }
        Logger::instance().log("USBIP-CORE", msg);
    });

    refreshImportedDevices();
    updateStatus("Idle");
    Logger::instance().log("INFO", "MainWindow initialized.");
}

MainWindow::~MainWindow() {
}

void MainWindow::setupUi() {
    setWindowTitle("USBIP Client");
    resize(600, 450);

    auto* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    auto* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    mainLayout->setSpacing(10);

    // Connection Settings Group
    auto* settingsLayout = new QHBoxLayout();
    
    m_hostEdit = new QLineEdit(centralWidget);
    m_hostEdit->setPlaceholderText("Android Host IP");
    m_hostEdit->setText("192.168.1.100");
    
    m_portEdit = new QLineEdit(centralWidget);
    m_portEdit->setPlaceholderText("Port");
    m_portEdit->setText("3240");
    m_portEdit->setMaximumWidth(80);

    m_listButton = new QPushButton("List Devices", centralWidget);
    
    settingsLayout->addWidget(new QLabel("Host:", centralWidget));
    settingsLayout->addWidget(m_hostEdit);
    settingsLayout->addWidget(new QLabel("Port:", centralWidget));
    settingsLayout->addWidget(m_portEdit);
    settingsLayout->addWidget(m_listButton);
    
    mainLayout->addLayout(settingsLayout);

    // Device List
    m_deviceList = new QListWidget(centralWidget);
    mainLayout->addWidget(m_deviceList);

    // Progress Bar
    m_progressBar = new QProgressBar(centralWidget);
    m_progressBar->setRange(0, 0); // Indeterminate
    m_progressBar->setVisible(false);
    m_progressBar->setTextVisible(false);
    m_progressBar->setFixedHeight(4);
    mainLayout->addWidget(m_progressBar);

    // Action Buttons
    auto* buttonsLayout = new QHBoxLayout();
    m_connectButton = new QPushButton("Connect", centralWidget);
    m_connectButton->setEnabled(false);
    
    m_disconnectButton = new QPushButton("Disconnect", centralWidget);
    m_disconnectButton->setEnabled(false);

    m_debugLogButton = new QPushButton("Debug Log", centralWidget);

    buttonsLayout->addWidget(m_connectButton);
    buttonsLayout->addWidget(m_disconnectButton);
    buttonsLayout->addWidget(m_debugLogButton);
    mainLayout->addLayout(buttonsLayout);

    // Status Label
    m_statusLabel = new QLabel("Status: Idle", centralWidget);
    mainLayout->addWidget(m_statusLabel);

    // Connect Signals
    connect(m_listButton, &QPushButton::clicked, this, &MainWindow::handleListDevices);
    connect(m_connectButton, &QPushButton::clicked, this, &MainWindow::handleConnectDevice);
    connect(m_disconnectButton, &QPushButton::clicked, this, &MainWindow::handleDisconnectDevice);
    connect(m_debugLogButton, &QPushButton::clicked, this, &MainWindow::handleShowDebugLog);
    connect(m_deviceList, &QListWidget::itemSelectionChanged, this, &MainWindow::handleSelectionChanged);
}

void MainWindow::applyStyleSheet() {
    setStyleSheet(
        "QMainWindow {"
        "    background-color: #1E1E1E;"
        "}"
        "QWidget {"
        "    background-color: #1E1E1E;"
        "    color: #FFFFFF;"
        "    font-family: 'Segoe UI', sans-serif;"
        "    font-size: 12px;"
        "}"
        "QListWidget {"
        "    background-color: #2D2D2D;"
        "    border: 1px solid #3D3D3D;"
        "    border-radius: 4px;"
        "    padding: 5px;"
        "    color: #FFFFFF;"
        "}"
        "QListWidget::item {"
        "    padding: 8px;"
        "    border-bottom: 1px solid #3D3D3D;"
        "}"
        "QListWidget::item:selected {"
        "    background-color: #007ACC;"
        "    color: #FFFFFF;"
        "}"
        "QLineEdit {"
        "    background-color: #2D2D2D;"
        "    border: 1px solid #3D3D3D;"
        "    border-radius: 4px;"
        "    padding: 6px;"
        "    color: #FFFFFF;"
        "}"
        "QLineEdit:focus {"
        "    border: 1px solid #007ACC;"
        "}"
        "QPushButton {"
        "    background-color: #2D2D2D;"
        "    border: 1px solid #3D3D3D;"
        "    border-radius: 4px;"
        "    padding: 8px 16px;"
        "    color: #FFFFFF;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "    background-color: #3D3D3D;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #007ACC;"
        "}"
        "QPushButton:disabled {"
        "    background-color: #1A1A1A;"
        "    color: #808080;"
        "    border: 1px solid #2D2D2D;"
        "}"
        "QLabel {"
        "    color: #FFFFFF;"
        "}"
        "QProgressBar {"
        "    background-color: #2D2D2D;"
        "    border: none;"
        "}"
        "QProgressBar::chunk {"
        "    background-color: #007ACC;"
        "}"
    );
}

void MainWindow::updateStatus(const QString& message, bool active) {
    if (active) {
        m_statusLabel->setText(QString("Status: <span style='color: #007ACC; font-weight: bold;'>%1</span>").arg(message));
    } else {
        m_statusLabel->setText(QString("Status: %1").arg(message));
    }
}

void MainWindow::showError(const QString& message, DWORD errorCode) {
    Logger::instance().log("ERROR", message.toStdString() + " Windows Error Code: " + std::to_string(errorCode));
    QMessageBox::critical(
        this,
        "Error",
        QString("%1\nWindows Error Code: %2 (0x%3)")
            .arg(message)
            .arg(errorCode)
            .arg(QString::number(errorCode, 16).toUpper())
    );
}

void MainWindow::refreshImportedDevices() {
    m_activeConnections.clear();
    auto dev = usbip::vhci::open();
    if (dev) {
        auto imported = usbip::vhci::get_imported_devices(dev.get());
        if (imported) {
            for (const auto& d : *imported) {
                m_activeConnections[d.location.busid] = d.port;
            }
        }
    }
}

void MainWindow::handleListDevices() {
    QString host = m_hostEdit->text().trimmed();
    QString port = m_portEdit->text().trimmed();

    if (host.isEmpty() || port.isEmpty()) {
        QMessageBox::warning(this, "Input Error", "Please enter both Host IP and Port.");
        return;
    }

    Logger::instance().log("INFO", "Listing devices from host: " + host.toStdString() + ":" + port.toStdString());

    m_listButton->setEnabled(false);
    m_progressBar->setVisible(true);
    updateStatus("Listing devices...");

    std::thread([this, hostStr = host.toStdString(), portStr = port.toStdString()]() {
        std::vector<UsbIpDevice> devices;
        DWORD err = m_manager.ListDevices(hostStr, portStr, devices);

        QMetaObject::invokeMethod(this, [this, err, devices = std::move(devices)]() mutable {
            m_listButton->setEnabled(true);
            m_progressBar->setVisible(false);

            if (err == ERROR_SUCCESS) {
                Logger::instance().log("INFO", "Successfully listed " + std::to_string(devices.size()) + " device(s).");
                m_displayedDevices = std::move(devices);
                m_deviceList->clear();
                refreshImportedDevices();

                for (const auto& dev : m_displayedDevices) {
                    QString speedStr;
                    switch (dev.device.speed) {
                        case UsbLowSpeed: speedStr = "Low Speed"; break;
                        case UsbFullSpeed: speedStr = "Full Speed"; break;
                        case UsbHighSpeed: speedStr = "High Speed"; break;
                        case UsbSuperSpeed: speedStr = "Super Speed"; break;
                        default: speedStr = "Unknown Speed"; break;
                    }

                    QString text = QString("Bus ID: %1 | %2 (%3:%4)\nSpeed: %5 | Interfaces: %6")
                        .arg(QString::fromStdString(dev.device.busid))
                        .arg(QString::fromStdString(dev.device.path))
                        .arg(QString::number(dev.device.idVendor, 16).toUpper(), 4, QChar('0'))
                        .arg(QString::number(dev.device.idProduct, 16).toUpper(), 4, QChar('0'))
                        .arg(speedStr)
                        .arg(dev.device.bNumInterfaces);

                    auto it = m_activeConnections.find(dev.device.busid);
                    if (it != m_activeConnections.end()) {
                        text += QString(" [Connected on Port %1]").arg(it->second);
                    }

                    auto* item = new QListWidgetItem(text, m_deviceList);
                    if (it != m_activeConnections.end()) {
                        item->setForeground(QColor("#007ACC"));
                    }
                }

                updateStatus(QString("Found %1 device(s)").arg(m_displayedDevices.size()));
            } else {
                updateStatus("Failed to list devices");
                showError("Failed to list devices from host.", err);
            }
        }, Qt::QueuedConnection);
    }).detach();
}

void MainWindow::handleConnectDevice() {
    int row = m_deviceList->currentRow();
    if (row < 0 || row >= static_cast<int>(m_displayedDevices.size())) {
        return;
    }

    const auto& dev = m_displayedDevices[row];
    QString host = m_hostEdit->text().trimmed();
    QString port = m_portEdit->text().trimmed();

    Logger::instance().log("INFO", "Connecting to device with Bus ID: " + dev.device.busid + " on host: " + host.toStdString() + ":" + port.toStdString());

    m_connectButton->setEnabled(false);
    m_progressBar->setVisible(true);
    updateStatus("Connecting device...");

    std::thread([this, hostStr = host.toStdString(), portStr = port.toStdString(), busid = dev.device.busid]() {
        int outPort = 0;
        DWORD err = m_manager.Connect(hostStr, portStr, busid, "", true, outPort);

        QMetaObject::invokeMethod(this, [this, err, busid, outPort]() {
            m_progressBar->setVisible(false);
            if (err == ERROR_SUCCESS) {
                Logger::instance().log("INFO", "Successfully connected device " + busid + " on Port " + std::to_string(outPort));
                updateStatus(QString("Connected on Port %1").arg(outPort), true);
                refreshImportedDevices();
                handleListDevices(); // Refresh list to show connected status
            } else {
                Logger::instance().log("ERROR", "Failed to connect device " + busid + ". Error: " + std::to_string(err));
                updateStatus("Connection failed");
                showError("Failed to connect to USB device.", err);
                handleSelectionChanged();
            }
        }, Qt::QueuedConnection);
    }).detach();
}

void MainWindow::handleDisconnectDevice() {
    int row = m_deviceList->currentRow();
    if (row < 0 || row >= static_cast<int>(m_displayedDevices.size())) {
        return;
    }

    const auto& dev = m_displayedDevices[row];
    auto it = m_activeConnections.find(dev.device.busid);
    if (it == m_activeConnections.end()) {
        return;
    }

    int port = it->second;

    Logger::instance().log("INFO", "Disconnecting device with Bus ID: " + dev.device.busid + " from Port: " + std::to_string(port));

    m_disconnectButton->setEnabled(false);
    m_progressBar->setVisible(true);
    updateStatus("Disconnecting device...");

    std::thread([this, port]() {
        DWORD err = m_manager.Disconnect(port);

        QMetaObject::invokeMethod(this, [this, err, port]() {
            m_progressBar->setVisible(false);
            if (err == ERROR_SUCCESS) {
                Logger::instance().log("INFO", "Successfully disconnected Port " + std::to_string(port));
                updateStatus("Disconnected successfully");
                refreshImportedDevices();
                handleListDevices(); // Refresh list
            } else {
                Logger::instance().log("ERROR", "Failed to disconnect Port " + std::to_string(port) + ". Error: " + std::to_string(err));
                updateStatus("Disconnect failed");
                showError("Failed to disconnect USB device.", err);
                handleSelectionChanged();
            }
        }, Qt::QueuedConnection);
    }).detach();
}

void MainWindow::handleSelectionChanged() {
    int row = m_deviceList->currentRow();
    if (row < 0 || row >= static_cast<int>(m_displayedDevices.size())) {
        m_connectButton->setEnabled(false);
        m_disconnectButton->setEnabled(false);
        return;
    }

    const auto& dev = m_displayedDevices[row];
    auto it = m_activeConnections.find(dev.device.busid);
    if (it != m_activeConnections.end()) {
        m_connectButton->setEnabled(false);
        m_disconnectButton->setEnabled(true);
    } else {
        m_connectButton->setEnabled(true);
        m_disconnectButton->setEnabled(false);
    }
}

void MainWindow::handleShowDebugLog() {
    class LogDialog : public QDialog {
    public:
        explicit LogDialog(QWidget* parent = nullptr) : QDialog(parent) {
            setWindowTitle("Debug Log");
            resize(750, 500);

            auto* layout = new QVBoxLayout(this);

            auto* textEdit = new QPlainTextEdit(this);
            textEdit->setReadOnly(true);
            textEdit->setLineWrapMode(QPlainTextEdit::NoWrap);
            
            QFont font("Courier New", 10);
            textEdit->setFont(font);

            auto history = Logger::instance().getLogHistory();
            QString logText;
            for (const auto& line : history) {
                logText += QString::fromStdString(line) + "\n";
            }
            textEdit->setPlainText(logText);
            textEdit->moveCursor(QTextCursor::End);

            layout->addWidget(textEdit);

            auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
            connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
            
            auto* clearButton = buttonBox->addButton("Clear Log", QDialogButtonBox::ActionRole);
            connect(clearButton, &QPushButton::clicked, this, [textEdit]() {
                Logger::instance().clear();
                textEdit->clear();
            });

            layout->addWidget(buttonBox);
        }
    };

    LogDialog dialog(this);
    dialog.exec();
}
