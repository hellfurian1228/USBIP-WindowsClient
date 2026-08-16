#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <windows.h>

class Logger {
public:
    static Logger& instance();

    void init();
    void log(const std::string& level, const std::string& message);
    void clear();
    std::vector<std::string> getLogHistory();
    std::string getLogFilePath() const;

    // Crash handler setup
    static void installCrashHandler();

private:
    Logger();
    ~Logger();

    static LONG WINAPI unhandledExceptionFilter(struct _EXCEPTION_POINTERS* exceptionInfo);
    void writeCrashLog(struct _EXCEPTION_POINTERS* exceptionInfo);

    std::mutex m_mutex;
    std::vector<std::string> m_history;
    std::string m_logFilePath;
    bool m_initialized = false;
};
