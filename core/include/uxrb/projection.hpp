#pragma once

#include "uxrb/openxr_platform.hpp"

#include <DirectXMath.h>

#include <string>

namespace uxrb {

[[nodiscard]] DirectX::XMMATRIX createProjectionMatrix(
    const XrFovf& fov,
    float nearPlane,
    float farPlane);

[[nodiscard]] DirectX::XMMATRIX createViewMatrix(const XrPosef& pose);

[[nodiscard]] bool isFiniteMatrix(DirectX::FXMMATRIX matrix) noexcept;

[[nodiscard]] bool runProjectionSelfTest(std::string& failureReason);

} // namespace uxrb

