#include <iostream>
#include <AudioUnit/AudioUnit.h>
#include <CoreAudioKit/AUViewController.h>
#include <AudioToolbox/AudioToolbox.h>

int main() {
    std::cout << "Loading Chimera plugin for diagnostic..." << std::endl;
    
    // Create component description
    AudioComponentDescription desc = {0};
    desc.componentType = kAudioUnitType_Effect;
    desc.componentSubType = 'ChPx';  // ChimeraPhoenix
    desc.componentManufacturer = 'Manu';
    desc.componentFlags = 0;
    desc.componentFlagsMask = 0;
    
    // Find the component
    AudioComponent comp = AudioComponentFindNext(NULL, &desc);
    if (!comp) {
        std::cerr << "Could not find Chimera component!" << std::endl;
        return 1;
    }
    
    // Create instance
    AudioUnit unit;
    OSStatus err = AudioComponentInstanceNew(comp, &unit);
    if (err != noErr) {
        std::cerr << "Could not create instance: " << err << std::endl;
        return 1;
    }
    
    // Initialize the audio unit
    err = AudioUnitInitialize(unit);
    if (err != noErr) {
        std::cerr << "Could not initialize: " << err << std::endl;
        return 1;
    }
    
    std::cout << "Plugin loaded successfully!" << std::endl;
    std::cout << "Check Console.app for diagnostic output..." << std::endl;
    
    // Cleanup
    AudioUnitUninitialize(unit);
    AudioComponentInstanceDispose(unit);
    
    return 0;
}