#include "uxrb/projection.hpp"

#include <cmath>
#include <cstddef>
#include <stdexcept>

namespace uxrb {

DirectX::XMMATRIX createProjectionMatrix(
    const XrFovf& fov,
    const float nearPlane,
    const float farPlane) {
    if (!(nearPlane > 0.0F) || !(farPlane > nearPlane)) {
        throw std::invalid_argument("Projection planes must satisfy 0 < near < far.");
    }

    const float left = nearPlane * std::tan(fov.angleLeft);
    const float right = nearPlane * std::tan(fov.angleRight);
    const float bottom = nearPlane * std::tan(fov.angleDown);
    const float top = nearPlane * std::tan(fov.angleUp);

    if (!(left < right) || !(bottom < top)) {
        throw std::invalid_argument("OpenXR field-of-view angles are not ordered correctly.");
    }

    return DirectX::XMMatrixPerspectiveOffCenterRH(
        left,
        right,
        bottom,
        top,
        nearPlane,
        farPlane);
}

DirectX::XMMATRIX createViewMatrix(const XrPosef& pose) {
    using namespace DirectX;

    XMVECTOR orientation = XMVectorSet(
        pose.orientation.x,
        pose.orientation.y,
        pose.orientation.z,
        pose.orientation.w);
    orientation = XMQuaternionNormalize(orientation);

    const XMMATRIX worldFromView = XMMatrixRotationQuaternion(orientation)
        * XMMatrixTranslation(pose.position.x, pose.position.y, pose.position.z);
    return XMMatrixInverse(nullptr, worldFromView);
}

bool isFiniteMatrix(const DirectX::FXMMATRIX matrix) noexcept {
    DirectX::XMFLOAT4X4 stored{};
    DirectX::XMStoreFloat4x4(&stored, matrix);
    const auto* values = &stored.m[0][0];
    for (std::size_t index = 0; index < 16; ++index) {
        if (!std::isfinite(values[index])) {
            return false;
        }
    }
    return true;
}

bool runProjectionSelfTest(std::string& failureReason) {
    using namespace DirectX;

    const XrPosef identityPose{
        XrQuaternionf{0.0F, 0.0F, 0.0F, 1.0F},
        XrVector3f{0.0F, 0.0F, 0.0F},
    };
    XMFLOAT4X4 identityView{};
    XMStoreFloat4x4(&identityView, createViewMatrix(identityPose));

    constexpr float tolerance = 0.0001F;
    const XMFLOAT4X4 expectedIdentity{
        1.0F, 0.0F, 0.0F, 0.0F,
        0.0F, 1.0F, 0.0F, 0.0F,
        0.0F, 0.0F, 1.0F, 0.0F,
        0.0F, 0.0F, 0.0F, 1.0F,
    };

    const auto* actualValues = &identityView.m[0][0];
    const auto* expectedValues = &expectedIdentity.m[0][0];
    for (std::size_t index = 0; index < 16; ++index) {
        if (std::abs(actualValues[index] - expectedValues[index]) > tolerance) {
            failureReason = "An identity OpenXR pose did not produce an identity view matrix.";
            return false;
        }
    }

    const XrPosef translatedPose{
        XrQuaternionf{0.0F, 0.0F, 0.0F, 1.0F},
        XrVector3f{1.0F, 2.0F, 3.0F},
    };
    XMFLOAT4X4 translatedView{};
    XMStoreFloat4x4(&translatedView, createViewMatrix(translatedPose));
    if (std::abs(translatedView._41 + 1.0F) > tolerance
        || std::abs(translatedView._42 + 2.0F) > tolerance
        || std::abs(translatedView._43 + 3.0F) > tolerance) {
        failureReason = "View translation is not the inverse of the OpenXR pose translation.";
        return false;
    }

    const XrFovf symmetricFov{-0.75F, 0.75F, 0.75F, -0.75F};
    const XMMATRIX projection = createProjectionMatrix(symmetricFov, 0.05F, 100.0F);
    if (!isFiniteMatrix(projection)) {
        failureReason = "A valid OpenXR field of view produced a non-finite projection matrix.";
        return false;
    }

    failureReason.clear();
    return true;
}

} // namespace uxrb

