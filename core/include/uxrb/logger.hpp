#pragma once

#include <filesystem>
#include <fstream>
#include <mutex>
#include <string_view>

namespace uxrb {

enum class LogLevel {
    debug,
    info,
    warning,
    error,
};

class Logger final {
public:
    static Logger& instance();

    void initialize();
    void write(LogLevel level, std::string_view message);

    void debug(std::string_view message);
    void info(std::string_view message);
    void warning(std::string_view message);
    void error(std::string_view message);

    [[nodiscard]] const std::filesystem::path& filePath() const noexcept;

private:
    Logger() = default;

    std::mutex mutex_;
    std::ofstream stream_;
    std::filesystem::path filePath_;
};

} // namespace uxrb

