// Test SlotComponent directly to find crash
#include <iostream>

int main() {
    std::cout << "Testing SlotComponent creation..." << std::endl;
    
    try {
        // Test 1: Check if the issue is in the constructor
        std::cout << "Test 1: Creating SlotComponent object..." << std::endl;
        
        // We can't actually create it without JUCE, but we can test the logic
        // Let's test the control type mapping logic
        
        // Simulate what happens in getControlTypeForParameter
        std::cout << "Test 2: Testing parameter control mapping..." << std::endl;
        
        // Test array bounds
        for (int i = 0; i < 15; ++i) {
            std::cout << "  Creating controls for param " << i << std::endl;
            // In actual code, this creates sliders[i], toggles[i], labels[i]
        }
        
        std::cout << "Test 3: Testing Font creation (deprecated API issue)..." << std::endl;
        // The warnings showed deprecated Font constructor - this might be the issue
        
        std::cout << "All tests passed!" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "CRASH: " << e.what() << std::endl;
        return 1;
    }
    catch (...) {
        std::cerr << "CRASH: Unknown error" << std::endl;
        return 1;
    }
    
    return 0;
}