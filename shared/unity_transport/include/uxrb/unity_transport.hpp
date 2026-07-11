#pragma once

#include <Windows.h>

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace uxrb::unity_transport {

inline constexpr std::uint32_t magic = 0x54525855U;  // UXRT
inline constexpr std::uint32_t version = 1;
inline constexpr std::size_t eyeCount = 2;
inline constexpr std::size_t textureNameCharacters = 128;

struct EyePose final {
    float position[3]{};
    float orientation[4]{0.0F, 0.0F, 0.0F, 1.0F};
    float fov[4]{};
};

struct StereoFrame final {
    std::uint64_t frameId{};
    std::int64_t predictedDisplayTimeNanoseconds{};
    std::uint32_t recommendedWidth{};
    std::uint32_t recommendedHeight{};
    std::uint32_t flags{};
    std::uint32_t reserved{};
    EyePose eyes[eyeCount]{};
};

struct EyeTexture final {
    std::uint32_t width{};
    std::uint32_t height{};
    std::uint32_t dxgiFormat{};
    std::uint32_t generation{};
    wchar_t sharedName[textureNameCharacters]{};
};

struct PluginState final {
    std::uint32_t processId{};
    std::uint32_t readyEyeMask{};
    std::uint64_t submittedFrameId{};
    std::uint64_t copiedFrameId{};
    std::uint32_t lastError{};
    std::uint32_t droppedFrames{};
    EyeTexture eyes[eyeCount]{};
};

class Mapping final {
public:
    Mapping() = default;
    ~Mapping();

    Mapping(const Mapping&) = delete;
    Mapping& operator=(const Mapping&) = delete;
    Mapping(Mapping&& other) noexcept;
    Mapping& operator=(Mapping&& other) noexcept;

    [[nodiscard]] bool createOwner(std::wstring_view name, std::string& error);
    [[nodiscard]] bool openClient(std::wstring_view name, std::string& error);
    void close() noexcept;

    [[nodiscard]] bool valid() const noexcept;
    [[nodiscard]] bool owner() const noexcept;
    [[nodiscard]] std::wstring name() const;

    [[nodiscard]] bool publishFrame(const StereoFrame& frame) noexcept;
    [[nodiscard]] bool readFrame(StereoFrame& frame) const noexcept;
    [[nodiscard]] bool publishPluginState(const PluginState& state) noexcept;
    [[nodiscard]] bool readPluginState(PluginState& state) const noexcept;
    void requestShutdown() noexcept;
    [[nodiscard]] bool shutdownRequested() const noexcept;

    [[nodiscard]] static bool validName(std::wstring_view name) noexcept;
    [[nodiscard]] static std::wstring environmentName();

private:
    struct SharedState;
    [[nodiscard]] bool mapHandle(HANDLE handle, bool isOwner, std::wstring_view name, std::string& error);
    void moveFrom(Mapping&& other) noexcept;

    HANDLE handle_{};
    SharedState* state_{};
    bool owner_{};
    std::wstring name_;
};

}  // namespace uxrb::unity_transport
