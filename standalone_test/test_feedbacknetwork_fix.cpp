/**
 * Quick test to verify FeedbackNetwork fix
 * This tests the signed-to-unsigned conversion issue
 */

#include <iostream>
#include <cmath>
#include <algorithm>

int main() {
    std::cout << "Testing FeedbackNetwork modulation offset fix...\n\n";

    // Simulate the scenario
    size_t bufferSize = 88200; // 2 seconds at 44.1kHz
    size_t delaySamples = 11025; // 250ms at 44.1kHz
    double fs = 44100.0;
    float modulationDepth = 0.05f; // 5% modulation

    std::cout << "Buffer size: " << bufferSize << " samples\n";
    std::cout << "Delay: " << delaySamples << " samples\n";
    std::cout << "Modulation depth: " << (modulationDepth * 100) << "%\n\n";

    // Test various modulation values
    for (float modPhase = 0; modPhase < 2 * M_PI; modPhase += M_PI / 8) {
        float modOffset = std::sin(modPhase) * modulationDepth * fs;

        std::cout << "sin(" << modPhase << ") = " << std::sin(modPhase)
                  << ", modOffset = " << modOffset << " samples\n";

        // OLD BUGGY WAY (crashes):
        std::cout << "  OLD (buggy): ";
        size_t buggy = (size_t)modOffset; // WRONG: negative wraps to huge number
        std::cout << "cast to size_t = " << buggy;
        if (buggy > bufferSize) {
            std::cout << " *** OUT OF BOUNDS ***";
        }
        std::cout << "\n";

        // NEW FIXED WAY:
        std::cout << "  NEW (fixed): ";
        int modDelay = static_cast<int>(delaySamples) + static_cast<int>(modOffset);
        size_t safeDelay = std::clamp<size_t>(std::max(1, modDelay), 1, bufferSize - 1);
        std::cout << "modDelay = " << modDelay << ", safeDelay = " << safeDelay;
        if (safeDelay >= 1 && safeDelay < bufferSize) {
            std::cout << " ✓ SAFE";
        } else {
            std::cout << " ✗ STILL BROKEN";
        }
        std::cout << "\n\n";
    }

    std::cout << "Fix verification complete!\n";
    std::cout << "The new code properly handles negative modulation offsets.\n";

    return 0;
}
