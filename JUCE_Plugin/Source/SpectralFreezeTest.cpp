#include "SpectralFreeze.h"
#include <iostream>
#include <cassert>

// Simple test to reproduce the window validation bug
int main() {
    std::cout << "Testing SpectralFreeze Window Validation Bug\n";
    std::cout << "============================================\n";
    
    // Create SpectralFreeze instance
    SpectralFreeze engine;
    
    try {
        // This should trigger the assertion failure at line 128
        std::cout << "Calling prepareToPlay (this should trigger assertion failure)...\n";
        engine.prepareToPlay(44100.0, 512);
        std::cout << "ERROR: No assertion failure occurred - bug may be fixed already\n";
    } catch (const std::exception& e) {
        std::cout << "Exception caught: " << e.what() << "\n";
    } catch (...) {
        std::cout << "Unknown exception caught\n";
    }
    
    std::cout << "Test completed.\n";
    return 0;
}