#include "uxrb/logger.hpp"

#include <Windows.h>

#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace uxrb {
namespace {

const char* levelName(const LogLevel level) noexcept {
    switch (level) {
    case LogLevel::debug:
        return "DEBUG";
    case LogLevel::info:
        return "INFO";
    case LogLevel::warning:
        return "WARN";
    case LogLevel::error:
        return "ERROR";
    }

    return "UNKNOWN";
}

std::tm localTime(const std::time_t value) noexcept {
    std::tm result{};
    localtime_s(&result, &value);
    return result;
}

} // namespace

Logger& Logger::instance() {
    static Logger logger;
    return logger;
}

void Logger::initialize() {
    std::scoped_lock lock(mutex_);
    if (stream_.is_open()) {
        return;
    }

    try {
        const DWORD required = GetEnvironmentVariableW(L"LOCALAPPDATA", nullptr, 0);
        std::filesystem::path base;
        if (required > 0) {
            std::vector<wchar_t> localAppData(required, L'\0');
            if (GetEnvironmentVariableW(L"LOCALAPPDATA", localAppData.data(), required) > 0) {
                base = std::filesystem::path(localAppData.data());
            }
        }
        if (base.empty()) {
            base = std::filesystem::temp_directory_path();
        }
        const auto directory = base / "HaumeaVR" / "logs";
        std::filesystem::create_directories(directory);

        const auto now = std::chrono::system_clock::now();
        const auto timeValue = std::chrono::system_clock::to_time_t(now);
        const auto local = localTime(timeValue);

        std::ostringstream name;
        name << "haumeavr-" << std::put_time(&local, "%Y%m%d-%H%M%S") << ".log";
        filePath_ = directory / name.str();
        stream_.open(filePath_, std::ios::out | std::ios::app);
    } catch (const std::exception& exception) {
        std::cerr << "[WARN] Could not initialize file logging: " << exception.what() << '\n';
    }
}

void Logger::write(const LogLevel level, const std::string_view message) {
    const auto now = std::chrono::system_clock::now();
    const auto timeValue = std::chrono::system_clock::to_time_t(now);
    const auto local = localTime(timeValue);
    const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::ostringstream line;
    line << std::put_time(&local, "%Y-%m-%d %H:%M:%S")
         << '.' << std::setfill('0') << std::setw(3) << milliseconds.count()
         << " [" << levelName(level) << "] " << message << '\n';
    const std::string text = line.str();

    std::scoped_lock lock(mutex_);
    if (level == LogLevel::error || level == LogLevel::warning) {
        std::cerr << text;
    } else {
        std::cout << text;
    }

    if (stream_.is_open()) {
        stream_ << text;
        stream_.flush();
    }

    OutputDebugStringA(text.c_str());
}

void Logger::debug(const std::string_view message) {
    write(LogLevel::debug, message);
}

void Logger::info(const std::string_view message) {
    write(LogLevel::info, message);
}

void Logger::warning(const std::string_view message) {
    write(LogLevel::warning, message);
}

void Logger::error(const std::string_view message) {
    write(LogLevel::error, message);
}

const std::filesystem::path& Logger::filePath() const noexcept {
    return filePath_;
}

} // namespace uxrb
