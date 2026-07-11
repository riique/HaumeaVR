#include "uxrb/depth_reconstruction.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

int main() {
    std::string failureReason;
    if (!uxrb::runDepthReconstructionSelfTest(failureReason)) {
        std::cerr << "Depth reconstruction self-test failed: " << failureReason << '\n';
        return EXIT_FAILURE;
    }
    std::cout << "Depth visualization and stereo compute reconstruction self-test passed.\n";
    return EXIT_SUCCESS;
}
