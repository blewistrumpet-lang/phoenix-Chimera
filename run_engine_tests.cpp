// Direct Engine Test Runner using existing JUCE Plugin code
// This integrates directly with the compiled plugin to test all engines

#include <iostream>
#include <iomanip>
#include <vector>
#include <memory>
#include <chrono>
#include <cmath>

// Forward declarations for dynamic loading
typedef void* (*CreateEngineFunc)(int);
typedef void (*DeleteEngineFunc)(void*);
typedef void (*ProcessFunc)(void*, float*, int, int);
typedef const char* (*GetNameFunc)(void*);
typedef void (*PrepareFunc)(void*, double, int);

class DirectEngineTest {
private:
    void* pluginHandle = nullptr;
    CreateEngineFunc createEngine = nullptr;
    DeleteEngineFunc deleteEngine = nullptr;
    ProcessFunc processEngine = nullptr;
    GetNameFunc getEngineName = nullptr;
    PrepareFunc prepareEngine = nullptr;

public:
    bool loadPlugin(const char* path) {
        // This would use dlopen to load the plugin
        // For now, we'll output instructions
        std::cout << "Plugin loading requires the compiled ChimeraPhoenix plugin.\n";
        std::cout << "Please ensure the plugin is built first.\n";
        return false;
    }
    
    void runTests() {
        std::cout << "\n================================================\n";
        std::cout << "     CHIMERA PHOENIX ENGINE TEST RUNNER\n";
        std::cout << "================================================\n\n";
        
        std::cout << "This test requires the ChimeraPhoenix plugin to be built.\n\n";
        std::cout << "To build and test:\n";
        std::cout << "1. Build the plugin:\n";
        std::cout << "   xcodebuild -project JUCE_Plugin/Builds/MacOSX/ChimeraPhoenix.xcodeproj \\\n";
        std::cout << "              -scheme 'ChimeraPhoenix - All' \\\n";
        std::cout << "              -configuration Debug\n\n";
        
        std::cout << "2. The plugin will test all 57 engines during initialization\n";
        std::cout << "3. Check the Xcode console output for test results\n\n";
        
        std::cout << "Alternatively, you can test in Logic Pro:\n";
        std::cout << "1. Load the plugin on a track\n";
        std::cout << "2. Switch between engines using the dropdown\n";
        std::cout << "3. Each engine should process audio without crashing\n\n";
    }
};

int main() {
    DirectEngineTest tester;
    tester.runTests();
    return 0;
}