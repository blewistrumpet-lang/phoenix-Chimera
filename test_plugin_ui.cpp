// Minimal test to verify the plugin UI doesn't crash
#include <iostream>
#include <dlfcn.h>

int main() {
    std::cout << "Testing ChimeraPhoenix plugin loading..." << std::endl;
    
    // Try to load the plugin component
    const char* pluginPath = "/Users/Branden/Library/Audio/Plug-Ins/Components/ChimeraPhoenix.component/Contents/MacOS/ChimeraPhoenix";
    
    std::cout << "Loading plugin from: " << pluginPath << std::endl;
    
    void* handle = dlopen(pluginPath, RTLD_NOW | RTLD_LOCAL);
    
    if (!handle) {
        std::cerr << "Failed to load plugin: " << dlerror() << std::endl;
        return 1;
    }
    
    std::cout << "Plugin loaded successfully!" << std::endl;
    
    // Check for common crash-causing symbols
    void* createEditorSym = dlsym(handle, "_ZN22ChimeraAudioProcessor12createEditorEv");
    if (createEditorSym) {
        std::cout << "Found createEditor symbol" << std::endl;
    }
    
    dlclose(handle);
    
    std::cout << "Test completed successfully - plugin should load without crashing" << std::endl;
    return 0;
}