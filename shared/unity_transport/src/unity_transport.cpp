#include "uxrb/unity_transport.hpp"

#include <algorithm>
#include <cstring>
#include <utility>

namespace uxrb::unity_transport {

struct alignas(64) Mapping::SharedState final {
    std::uint32_t stateMagic{};
    std::uint32_t stateVersion{};
    std::uint32_t stateSize{};
    std::uint32_t ownerProcessId{};
    alignas(8) volatile LONG frameSequence{};
    LONG framePadding{};
    StereoFrame frame{};
    alignas(8) volatile LONG pluginSequence{};
    LONG pluginPadding{};
    PluginState plugin{};
    alignas(8) volatile LONG shutdown{};
};

namespace {

[[nodiscard]] LONG readSequence(volatile LONG* value) noexcept {
    return InterlockedCompareExchange(value, 0, 0);
}

template <typename T>
void publish(volatile LONG* sequence, T& destination, const T& source) noexcept {
    InterlockedIncrement(sequence);
    MemoryBarrier();
    std::memcpy(&destination, &source, sizeof(T));
    MemoryBarrier();
    InterlockedIncrement(sequence);
}

template <typename T>
[[nodiscard]] bool readConsistent(volatile LONG* sequence, const T& source, T& destination) noexcept {
    for (int attempt = 0; attempt < 8; ++attempt) {
        const LONG before = readSequence(sequence);
        if ((before & 1) != 0) {
            YieldProcessor();
            continue;
        }
        MemoryBarrier();
        std::memcpy(&destination, &source, sizeof(T));
        MemoryBarrier();
        const LONG after = readSequence(sequence);
        if (before == after && (after & 1) == 0) return true;
    }
    return false;
}

}  // namespace

Mapping::~Mapping() {
    close();
}

Mapping::Mapping(Mapping&& other) noexcept {
    moveFrom(std::move(other));
}

Mapping& Mapping::operator=(Mapping&& other) noexcept {
    if (this != &other) {
        close();
        moveFrom(std::move(other));
    }
    return *this;
}

bool Mapping::createOwner(const std::wstring_view name, std::string& error) {
    close();
    if (!validName(name)) {
        error = "Unity transport name is invalid.";
        return false;
    }

    const std::wstring ownedName(name);
    HANDLE handle = CreateFileMappingW(
        INVALID_HANDLE_VALUE,
        nullptr,
        PAGE_READWRITE,
        0,
        static_cast<DWORD>(sizeof(SharedState)),
        ownedName.c_str());
    if (handle == nullptr) {
        error = "CreateFileMappingW failed for the Unity transport.";
        return false;
    }
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(handle);
        error = "Unity transport name already exists; refusing to attach to an untrusted mapping.";
        return false;
    }
    if (!mapHandle(handle, true, name, error)) return false;

    std::memset(state_, 0, sizeof(SharedState));
    state_->stateMagic = magic;
    state_->stateVersion = version;
    state_->stateSize = sizeof(SharedState);
    state_->ownerProcessId = GetCurrentProcessId();
    MemoryBarrier();
    return true;
}

bool Mapping::openClient(const std::wstring_view name, std::string& error) {
    close();
    if (!validName(name)) {
        error = "Unity transport name is invalid.";
        return false;
    }
    const std::wstring ownedName(name);
    HANDLE handle = OpenFileMappingW(FILE_MAP_READ | FILE_MAP_WRITE, FALSE, ownedName.c_str());
    if (handle == nullptr) {
        error = "OpenFileMappingW could not find the Unity transport owner.";
        return false;
    }
    if (!mapHandle(handle, false, name, error)) return false;
    MemoryBarrier();
    if (state_->stateMagic != magic || state_->stateVersion != version || state_->stateSize != sizeof(SharedState)) {
        close();
        error = "Unity transport ABI validation failed.";
        return false;
    }
    return true;
}

void Mapping::close() noexcept {
    if (state_ != nullptr) UnmapViewOfFile(state_);
    if (handle_ != nullptr) CloseHandle(handle_);
    handle_ = nullptr;
    state_ = nullptr;
    owner_ = false;
    name_.clear();
}

bool Mapping::valid() const noexcept {
    return handle_ != nullptr && state_ != nullptr;
}

bool Mapping::owner() const noexcept {
    return valid() && owner_;
}

std::wstring Mapping::name() const {
    return name_;
}

bool Mapping::publishFrame(const StereoFrame& frame) noexcept {
    if (!valid() || !owner_) return false;
    publish(&state_->frameSequence, state_->frame, frame);
    return true;
}

bool Mapping::readFrame(StereoFrame& frame) const noexcept {
    return valid() && readConsistent(&state_->frameSequence, state_->frame, frame);
}

bool Mapping::publishPluginState(const PluginState& pluginState) noexcept {
    if (!valid() || owner_) return false;
    publish(&state_->pluginSequence, state_->plugin, pluginState);
    return true;
}

bool Mapping::readPluginState(PluginState& pluginState) const noexcept {
    return valid() && readConsistent(&state_->pluginSequence, state_->plugin, pluginState);
}

void Mapping::requestShutdown() noexcept {
    if (valid() && owner_) InterlockedExchange(&state_->shutdown, 1);
}

bool Mapping::shutdownRequested() const noexcept {
    return valid() && readSequence(&state_->shutdown) != 0;
}

bool Mapping::validName(const std::wstring_view name) noexcept {
    constexpr std::wstring_view prefix = L"Local\\UXRB-Unity-";
    if (!name.starts_with(prefix) || name.size() <= prefix.size() || name.size() > 120) return false;
    return std::ranges::all_of(name.substr(prefix.size()), [](const wchar_t character) {
        return (character >= L'a' && character <= L'z') || (character >= L'A' && character <= L'Z') ||
            (character >= L'0' && character <= L'9') || character == L'-';
    });
}

std::wstring Mapping::environmentName() {
    std::array<wchar_t, 128> value{};
    const DWORD length = GetEnvironmentVariableW(
        L"UXRB_UNITY_TRANSPORT",
        value.data(),
        static_cast<DWORD>(value.size()));
    if (length == 0 || length >= value.size()) return {};
    const std::wstring result(value.data(), length);
    return validName(result) ? result : std::wstring{};
}

bool Mapping::mapHandle(HANDLE handle, const bool isOwner, const std::wstring_view name, std::string& error) {
    void* view = MapViewOfFile(handle, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, sizeof(SharedState));
    if (view == nullptr) {
        CloseHandle(handle);
        error = "MapViewOfFile failed for the Unity transport.";
        return false;
    }
    handle_ = handle;
    state_ = static_cast<SharedState*>(view);
    owner_ = isOwner;
    name_ = name;
    error.clear();
    return true;
}

void Mapping::moveFrom(Mapping&& other) noexcept {
    handle_ = std::exchange(other.handle_, nullptr);
    state_ = std::exchange(other.state_, nullptr);
    owner_ = std::exchange(other.owner_, false);
    name_ = std::move(other.name_);
}

}  // namespace uxrb::unity_transport
