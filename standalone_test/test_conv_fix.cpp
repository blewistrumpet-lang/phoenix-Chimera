#include <iostream>
#include <cmath>

// Quick test of the damping filter fix

int main() {
    double sampleRate = 48000.0;

    // Old broken formula
    float dampingParam = 1.0f;
    float oldDampFreq = 20000.0f * (1.0f - dampingParam);
    float oldDampCoeff = std::exp(-2.0f * M_PI * oldDampFreq / sampleRate);

    std::cout << "OLD FORMULA (BROKEN):" << std::endl;
    std::cout << "  dampingParam = " << dampingParam << std::endl;
    std::cout << "  dampFreq = " << oldDampFreq << " Hz" << std::endl;
    std::cout << "  dampCoeff = " << oldDampCoeff << std::endl;
    std::cout << "  Filter: state = data[i] * " << (1.0f - oldDampCoeff) << " + state * " << oldDampCoeff << std::endl;
    std::cout << "  Problem: When coeff=1.0, state = data[i] * 0.0 + state * 1.0 = state (zeros everything!)" << std::endl;
    std::cout << std::endl;

    // New fixed formula
    float newDampFreq = 20000.0f * std::pow(0.025f, dampingParam);
    newDampFreq = std::clamp(newDampFreq, 100.0f, 20000.0f);
    float newDampCoeff = 1.0f - std::exp(-2.0f * M_PI * newDampFreq / sampleRate);
    newDampCoeff = std::clamp(newDampCoeff, 0.0f, 1.0f);

    std::cout << "NEW FORMULA (FIXED):" << std::endl;
    std::cout << "  dampingParam = " << dampingParam << std::endl;
    std::cout << "  dampFreq = " << newDampFreq << " Hz" << std::endl;
    std::cout << "  dampCoeff = " << newDampCoeff << std::endl;
    std::cout << "  Filter: state = data[i] * " << newDampCoeff << " + state * " << (1.0f - newDampCoeff) << std::endl;
    std::cout << "  This is a proper lowpass filter that preserves signal!" << std::endl;
    std::cout << std::endl;

    // Test with dampingParam = 0.0 (no damping)
    dampingParam = 0.0f;
    newDampFreq = 20000.0f * std::pow(0.025f, dampingParam);
    newDampCoeff = 1.0f - std::exp(-2.0f * M_PI * newDampFreq / sampleRate);

    std::cout << "NEW FORMULA at dampingParam = 0.0:" << std::endl;
    std::cout << "  dampFreq = " << newDampFreq << " Hz (full bandwidth)" << std::endl;
    std::cout << "  dampCoeff = " << newDampCoeff << std::endl;
    std::cout << std::endl;

    return 0;
}
