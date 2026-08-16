#include "Logger.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <filesystem>
#include <QCoreApplication>
#include <QDebug>
#include <QDateTime>

// Qt message handler callback
void qtMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg) {
    std::string level = "INFO";
    switch (type) {
        case QtDebugMsg: level = "DEBUG"; break;
        case QtInfoMsg: level = "INFO"; break;
        case QtWarningMsg: level = "WARNING"; break;
        case QtCriticalMsg: level = "CRITICAL"; break;
        case QtFatalMsg: level = "FATAL"; break;
    }
    std::string category = context.category ? context.category : "";
    std::string file = context.file ? context.file : "";
    std::string function = context.function ? context.function : "";
    
    std::ostringstream oss;
    if (!category.empty()) {
        oss << "[" << category << "] ";
    }
    oss << msg.toStdString();
    if (!file.empty()) {
        oss << " (at " << file << ":" << context.line << ", " << function << ")";
    }
    Logger::instance().log(level, oss.str());
}

Logger& Logger::instance() {
    static Logger inst;
    return inst;
}

Logger::Logger() {
}

Logger::~Logger() {
}

void Logger::init() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_initialized) return;

    // Set log file path to application directory
    QString appDir = QCoreApplication::applicationDirPath();
    m_logFilePath = (appDir + "/usbip_client.log").toStdString();

    // Clear previous log file
    std::ofstream ofs(m_logFilePath, std::ios::trunc);
    if (ofs.is_open()) {
        ofs << "=== USBIP Client Log Started ===\n";
    }

    // Install Qt message handler
    qInstallMessageHandler(qtMessageHandler);

    m_initialized = true;
}

void Logger::log(const std::string& level, const std::string& message) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Get current time
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    
    std::tm tm_now;
    localtime_s(&tm_now, &time_t_now);

    std::ostringstream oss;
    oss << std::put_time(&tm_now, "%Y-%m-%d %H:%M:%S") << "." 
        << std::setfill('0') << std::setw(3) << ms.count()
        << " [" << level << "] " << message;

    std::string formatted = oss.str();
    
    // Print to console/debugger
    OutputDebugStringA((formatted + "\n").c_str());
    std::cout << formatted << std::endl;

    // Add to history (limit to 1000 lines)
    m_history.push_back(formatted);
    if (m_history.size() > 1000) {
        m_history.erase(m_history.begin());
    }

    // Write to file
    if (!m_logFilePath.empty()) {
        std::ofstream ofs(m_logFilePath, std::ios::app);
        if (ofs.is_open()) {
            ofs << formatted << "\n";
        }
    }
}

void Logger::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_history.clear();
    if (!m_logFilePath.empty()) {
        std::ofstream ofs(m_logFilePath, std::ios::trunc);
        if (ofs.is_open()) {
            ofs << "=== USBIP Client Log Cleared ===\n";
        }
    }
}

std::vector<std::string> Logger::getLogHistory() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_history;
}

std::string Logger::getLogFilePath() const {
    return m_logFilePath;
}

void Logger::installCrashHandler() {
    SetUnhandledExceptionFilter(unhandledExceptionFilter);
}

LONG WINAPI Logger::unhandledExceptionFilter(struct _EXCEPTION_POINTERS* exceptionInfo) {
    Logger::instance().log("CRITICAL", "Application crashed! Generating crash log...");
    Logger::instance().writeCrashLog(exceptionInfo);
    
    // Show a message box to the user
    MessageBoxA(
        NULL,
        "The application has crashed. A detailed crash log has been generated in the application directory (crash_log.txt).",
        "Application Crash",
        MB_ICONERROR | MB_OK
    );

    return EXCEPTION_EXECUTE_HANDLER;
}

void Logger::writeCrashLog(struct _EXCEPTION_POINTERS* exceptionInfo) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    QString appDir = QCoreApplication::applicationDirPath();
    std::string crashPath = (appDir + "/crash_log.txt").toStdString();

    std::ofstream ofs(crashPath, std::ios::trunc);
    if (!ofs.is_open()) return;

    ofs << "=========================================\n";
    ofs << "           APPLICATION CRASH LOG         \n";
    ofs << "=========================================\n\n";

    // Time
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::tm tm_now;
    localtime_s(&tm_now, &time_t_now);
    ofs << "Crash Time: " << std::put_time(&tm_now, "%Y-%m-%d %H:%M:%S") << "\n";

    // Exception details
    if (exceptionInfo && exceptionInfo->ExceptionRecord) {
        auto* rec = exceptionInfo->ExceptionRecord;
        ofs << "Exception Code: 0x" << std::hex << rec->ExceptionCode << std::dec << " ";
        switch (rec->ExceptionCode) {
            case EXCEPTION_ACCESS_VIOLATION:
                ofs << "(Access Violation)\n";
                if (rec->NumberParameters >= 2) {
                    ofs << "  Operation: " << (rec->ExceptionInformation[0] ? "Write" : "Read") << "\n";
                    ofs << "  Address: 0x" << std::hex << rec->ExceptionInformation[1] << std::dec << "\n";
                }
                break;
            case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: ofs << "(Array Bounds Exceeded)\n"; break;
            case EXCEPTION_BREAKPOINT: ofs << "(Breakpoint)\n"; break;
            case EXCEPTION_DATATYPE_MISALIGNMENT: ofs << "(Datatype Misalignment)\n"; break;
            case EXCEPTION_FLT_DENORMAL_OPERAND: ofs << "(Float Denormal Operand)\n"; break;
            case EXCEPTION_FLT_DIVIDE_BY_ZERO: ofs << "(Float Divide By Zero)\n"; break;
            case EXCEPTION_FLT_INEXACT_RESULT: ofs << "(Float Inexact Result)\n"; break;
            case EXCEPTION_FLT_INVALID_OPERATION: ofs << "(Float Invalid Operation)\n"; break;
            case EXCEPTION_FLT_OVERFLOW: ofs << "(Float Overflow)\n"; break;
            case EXCEPTION_FLT_STACK_CHECK: ofs << "(Float Stack Check)\n"; break;
            case EXCEPTION_FLT_UNDERFLOW: ofs << "(Float Underflow)\n"; break;
            case EXCEPTION_INT_DIVIDE_BY_ZERO: ofs << "(Integer Divide By Zero)\n"; break;
            case EXCEPTION_INT_OVERFLOW: ofs << "(Integer Overflow)\n"; break;
            case EXCEPTION_PRIV_INSTRUCTION: ofs << "(Privileged Instruction)\n"; break;
            case EXCEPTION_IN_PAGE_ERROR: ofs << "(In Page Error)\n"; break;
            case EXCEPTION_ILLEGAL_INSTRUCTION: ofs << "(Illegal Instruction)\n"; break;
            case EXCEPTION_NONCONTINUABLE_EXCEPTION: ofs << "(Noncontinuable Exception)\n"; break;
            case EXCEPTION_STACK_OVERFLOW: ofs << "(Stack Overflow)\n"; break;
            case STATUS_HEAP_CORRUPTION: ofs << "(Heap Corruption)\n"; break;
            default: ofs << "(Unknown Exception)\n"; break;
        }
        ofs << "Exception Address: 0x" << std::hex << rec->ExceptionAddress << std::dec << "\n";
    } else {
        ofs << "No exception record available.\n";
    }

    // System info
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    ofs << "\nSystem Information:\n";
    ofs << "  Processor Architecture: ";
    switch (sysInfo.wProcessorArchitecture) {
        case PROCESSOR_ARCHITECTURE_AMD64: ofs << "x64\n"; break;
        case PROCESSOR_ARCHITECTURE_ARM64: ofs << "ARM64\n"; break;
        case PROCESSOR_ARCHITECTURE_INTEL: ofs << "x86\n"; break;
        default: ofs << "Unknown\n"; break;
    }
    ofs << "  Number of Processors: " << sysInfo.dwNumberOfProcessors << "\n";

    // Log history leading up to crash
    ofs << "\n=========================================\n";
    ofs << "            RECENT LOG HISTORY           \n";
    ofs << "=========================================\n";
    for (const auto& line : m_history) {
        ofs << line << "\n";
    }
    ofs << "\n=== End of Crash Log ===\n";
}
