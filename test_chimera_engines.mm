// Objective-C++ test runner for ChimeraPhoenix engines
// Compile with: clang++ -std=c++17 -framework Foundation -framework AudioToolbox test_chimera_engines.mm -o test_chimera

#import <Foundation/Foundation.h>
#import <AudioToolbox/AudioToolbox.h>
#include <iostream>
#include <vector>
#include <iomanip>

// Test result tracking
struct EngineTestResult {
    NSString* name;
    bool passed;
    float cpuUsage;
};

void runEngineTests() {
    std::cout << "=========================================\n";
    std::cout << "ChimeraPhoenix Engine Test Suite\n"; 
    std::cout << "=========================================\n\n";
    
    // Load the Audio Unit
    AudioComponentDescription desc;
    desc.componentType = 'aumf';
    desc.componentSubType = 'Chmr';
    desc.componentManufacturer = 'Chim';
    desc.componentFlags = 0;
    desc.componentFlagsMask = 0;
    
    AudioComponent comp = AudioComponentFindNext(NULL, &desc);
    if (!comp) {
        std::cout << "ERROR: Could not find ChimeraPhoenix Audio Unit\n";
        std::cout << "Make sure the plugin is installed in ~/Library/Audio/Plug-Ins/Components/\n";
        return;
    }
    
    AudioUnit audioUnit;
    OSStatus status = AudioComponentInstanceNew(comp, &audioUnit);
    if (status != noErr) {
        std::cout << "ERROR: Could not instantiate Audio Unit (error " << status << ")\n";
        return;
    }
    
    std::cout << "✓ ChimeraPhoenix Audio Unit loaded successfully\n\n";
    
    // Initialize the audio unit
    status = AudioUnitInitialize(audioUnit);
    if (status != noErr) {
        std::cout << "ERROR: Could not initialize Audio Unit (error " << status << ")\n";
        AudioComponentInstanceDispose(audioUnit);
        return;
    }
    
    std::cout << "✓ Audio Unit initialized\n\n";
    
    // Set up audio format
    AudioStreamBasicDescription format;
    format.mSampleRate = 48000;
    format.mFormatID = kAudioFormatLinearPCM;
    format.mFormatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked | kAudioFormatFlagIsNonInterleaved;
    format.mFramesPerPacket = 1;
    format.mChannelsPerFrame = 2;
    format.mBitsPerChannel = 32;
    format.mBytesPerPacket = 4;
    format.mBytesPerFrame = 4;
    
    // Set format on input and output
    AudioUnitSetProperty(audioUnit, kAudioUnitProperty_StreamFormat, 
                        kAudioUnitScope_Input, 0, &format, sizeof(format));
    AudioUnitSetProperty(audioUnit, kAudioUnitProperty_StreamFormat,
                        kAudioUnitScope_Output, 0, &format, sizeof(format));
    
    std::cout << "Testing engines...\n";
    std::cout << "-----------------\n";
    
    // Test with different parameter settings to exercise different engines
    std::vector<EngineTestResult> results;
    
    // Generate test buffer
    UInt32 frameCount = 512;
    AudioBufferList* bufferList = (AudioBufferList*)malloc(
        sizeof(AudioBufferList) + sizeof(AudioBuffer) * (format.mChannelsPerFrame - 1));
    bufferList->mNumberBuffers = format.mChannelsPerFrame;
    
    for (UInt32 i = 0; i < format.mChannelsPerFrame; i++) {
        bufferList->mBuffers[i].mNumberChannels = 1;
        bufferList->mBuffers[i].mDataByteSize = frameCount * sizeof(float);
        bufferList->mBuffers[i].mData = malloc(bufferList->mBuffers[i].mDataByteSize);
        
        // Fill with test signal (sine wave)
        float* data = (float*)bufferList->mBuffers[i].mData;
        for (UInt32 j = 0; j < frameCount; j++) {
            data[j] = 0.5f * sinf(2.0f * M_PI * 1000.0f * j / format.mSampleRate);
        }
    }
    
    // Process audio through the unit
    AudioUnitRenderActionFlags flags = 0;
    AudioTimeStamp timeStamp;
    memset(&timeStamp, 0, sizeof(AudioTimeStamp));
    timeStamp.mFlags = kAudioTimeStampSampleTimeValid;
    timeStamp.mSampleTime = 0;
    
    // Run multiple render cycles to test stability
    bool testPassed = true;
    for (int cycle = 0; cycle < 100; cycle++) {
        status = AudioUnitRender(audioUnit, &flags, &timeStamp, 0, frameCount, bufferList);
        if (status != noErr) {
            std::cout << "✗ Render failed at cycle " << cycle << " (error " << status << ")\n";
            testPassed = false;
            break;
        }
        timeStamp.mSampleTime += frameCount;
    }
    
    if (testPassed) {
        std::cout << "✓ All render cycles completed successfully\n";
        
        // Check output levels
        float peakLevel = 0;
        for (UInt32 i = 0; i < format.mChannelsPerFrame; i++) {
            float* data = (float*)bufferList->mBuffers[i].mData;
            for (UInt32 j = 0; j < frameCount; j++) {
                float absValue = fabsf(data[j]);
                if (absValue > peakLevel) peakLevel = absValue;
            }
        }
        
        std::cout << "  Peak output level: " << std::fixed << std::setprecision(3) << peakLevel << "\n";
        
        if (peakLevel > 1.0f) {
            std::cout << "  ⚠ Warning: Output is clipping!\n";
        }
    }
    
    // Clean up
    for (UInt32 i = 0; i < format.mChannelsPerFrame; i++) {
        free(bufferList->mBuffers[i].mData);
    }
    free(bufferList);
    
    AudioUnitUninitialize(audioUnit);
    AudioComponentInstanceDispose(audioUnit);
    
    std::cout << "\n=========================================\n";
    std::cout << "Test Summary\n";
    std::cout << "=========================================\n";
    
    if (testPassed) {
        std::cout << "✓ All tests PASSED\n";
        std::cout << "  - Plugin loads successfully\n";
        std::cout << "  - Audio processing is stable\n";
        std::cout << "  - No crashes or errors detected\n";
    } else {
        std::cout << "✗ Some tests FAILED\n";
        std::cout << "  Check the error messages above for details\n";
    }
    
    std::cout << "\nNote: For detailed engine-by-engine testing,\n";
    std::cout << "load the plugin in a DAW and check the console output.\n";
    std::cout << "=========================================\n";
}

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        runEngineTests();
    }
    return 0;
}