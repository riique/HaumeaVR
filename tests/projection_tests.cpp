#include "uxrb/projection.hpp"

#include <DirectXMath.h>

#include <cmath>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void require(const bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

} // namespace

int main() {
    try {
        std::string failureReason;
        require(uxrb::runProjectionSelfTest(failureReason), failureReason.c_str());

        bool rejectedInvalidPlanes = false;
        try {
            const XrFovf fov{-0.5F, 0.5F, 0.5F, -0.5F};
            static_cast<void>(uxrb::createProjectionMatrix(fov, 1.0F, 0.5F));
        } catch (const std::invalid_argument&) {
            rejectedInvalidPlanes = true;
        }
        require(rejectedInvalidPlanes, "Projection accepted far <= near.");

        const XrFovf asymmetricFov{-0.9F, 0.6F, 0.7F, -0.8F};
        const auto projection = uxrb::createProjectionMatrix(asymmetricFov, 0.05F, 250.0F);
        require(uxrb::isFiniteMatrix(projection), "Asymmetric projection is not finite.");

        std::cout << "All projection and pose tests passed.\n";
        return 0;
    } catch (const std::exception& exception) {
        std::cerr << "Projection test failure: " << exception.what() << '\n';
        return 1;
    }
}

