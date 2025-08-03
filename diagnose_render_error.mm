// Diagnostic test to understand why AudioUnitRender is failing
#import <Foundation/Foundation.h>
#import <AudioToolbox/AudioToolbox.h>
#include <iostream>

const char* GetMacOSStatusErrorString(OSStatus status) {
    switch(status) {
        case kAudioUnitErr_InvalidProperty: return "Invalid Property";
        case kAudioUnitErr_InvalidParameter: return "Invalid Parameter";
        case kAudioUnitErr_InvalidElement: return "Invalid Element";
        case kAudioUnitErr_NoConnection: return "No Connection";
        case kAudioUnitErr_FailedInitialization: return "Failed Initialization";
        case kAudioUnitErr_TooManyFramesToProcess: return "Too Many Frames";
        case kAudioUnitErr_InvalidFile: return "Invalid File";
        case kAudioUnitErr_UnknownFileType: return "Unknown File Type";
        case kAudioUnitErr_FileNotSpecified: return "File Not Specified";
        case kAudioUnitErr_FormatNotSupported: return "Format Not Supported";
        case kAudioUnitErr_Uninitialized: return "Uninitialized";
        case kAudioUnitErr_InvalidScope: return "Invalid Scope";
        case kAudioUnitErr_PropertyNotWritable: return "Property Not Writable";
        case kAudioUnitErr_CannotDoInCurrentContext: return "Cannot Do In Current Context";
        case kAudioUnitErr_InvalidPropertyValue: return "Invalid Property Value";
        case kAudioUnitErr_PropertyNotInUse: return "Property Not In Use";
        case kAudioUnitErr_Initialized: return "Already Initialized";
        case kAudioUnitErr_InvalidOfflineRender: return "Invalid Offline Render";
        case kAudioUnitErr_Unauthorized: return "Unauthorized";
        case kAudioUnitErr_RenderTimeout: return "Render Timeout";
        default: return "Unknown Error";
    }
}

int main() {
    @autoreleasepool {
        std::cout << "Audio Unit Render Diagnostic Test\n";
        std::cout << "==================================\n\n";
        
        // Load Audio Unit
        AudioComponentDescription desc = {
            .componentType = 'aumf',
            .componentSubType = 'Chmr',
            .componentManufacturer = 'Chim',
            .componentFlags = 0,
            .componentFlagsMask = 0
        };
        
        AudioComponent comp = AudioComponentFindNext(NULL, &desc);
        if (!comp) {
            std::cout << "ERROR: ChimeraPhoenix not found\n";
            return 1;
        }
        std::cout << "✓ Found ChimeraPhoenix Audio Unit\n";
        
        // Create instance
        AudioUnit audioUnit;
        OSStatus status = AudioComponentInstanceNew(comp, &audioUnit);
        if (status != noErr) {
            std::cout << "✗ Could not create instance: " << status << " (" << GetMacOSStatusErrorString(status) << ")\n";
            return 1;
        }
        std::cout << "✓ Created Audio Unit instance\n";
        
        // Check current state
        UInt32 size = sizeof(UInt32);
        
        // Set format BEFORE initializing
        AudioStreamBasicDescription format = {0};
        format.mSampleRate = 44100;
        format.mFormatID = kAudioFormatLinearPCM;
        format.mFormatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked | kAudioFormatFlagIsNonInterleaved;
        format.mFramesPerPacket = 1;
        format.mChannelsPerFrame = 2;
        format.mBitsPerChannel = 32;
        format.mBytesPerPacket = 4;
        format.mBytesPerFrame = 4;
        
        // Set input format
        status = AudioUnitSetProperty(audioUnit, kAudioUnitProperty_StreamFormat,
                                     kAudioUnitScope_Input, 0, &format, sizeof(format));
        if (status != noErr) {
            std::cout << "✗ Could not set input format: " << status << " (" << GetMacOSStatusErrorString(status) << ")\n";
        } else {
            std::cout << "✓ Set input format\n";
        }
        
        // Set output format
        status = AudioUnitSetProperty(audioUnit, kAudioUnitProperty_StreamFormat,
                                     kAudioUnitScope_Output, 0, &format, sizeof(format));
        if (status != noErr) {
            std::cout << "✗ Could not set output format: " << status << " (" << GetMacOSStatusErrorString(status) << ")\n";
        } else {
            std::cout << "✓ Set output format\n";
        }
        
        // Check maximum frames per slice
        UInt32 maxFrames = 0;
        size = sizeof(maxFrames);
        status = AudioUnitGetProperty(audioUnit, kAudioUnitProperty_MaximumFramesPerSlice,
                                      kAudioUnitScope_Global, 0, &maxFrames, &size);
        std::cout << "  Max frames per slice: " << maxFrames << "\n";
        
        // Set maximum frames if needed
        if (maxFrames == 0 || maxFrames > 4096) {
            maxFrames = 4096;
            status = AudioUnitSetProperty(audioUnit, kAudioUnitProperty_MaximumFramesPerSlice,
                                         kAudioUnitScope_Global, 0, &maxFrames, sizeof(maxFrames));
            if (status == noErr) {
                std::cout << "  Set max frames to: " << maxFrames << "\n";
            }
        }
        
        // Initialize
        std::cout << "\nInitializing Audio Unit...\n";
        status = AudioUnitInitialize(audioUnit);
        if (status != noErr) {
            std::cout << "✗ Could not initialize: " << status << " (" << GetMacOSStatusErrorString(status) << ")\n";
            AudioComponentInstanceDispose(audioUnit);
            return 1;
        }
        std::cout << "✓ Audio Unit initialized\n";
        
        // Check render callback requirement
        AURenderCallbackStruct callback;
        size = sizeof(callback);
        status = AudioUnitGetProperty(audioUnit, kAudioUnitProperty_SetRenderCallback,
                                      kAudioUnitScope_Input, 0, &callback, &size);
        std::cout << "  Input callback status: " << status << "\n";
        
        // Set up input callback (required for effect units)
        std::cout << "\nSetting up input callback...\n";
        AURenderCallbackStruct inputCallback;
        inputCallback.inputProc = [](void* inRefCon,
                                    AudioUnitRenderActionFlags* ioActionFlags,
                                    const AudioTimeStamp* inTimeStamp,
                                    UInt32 inBusNumber,
                                    UInt32 inNumberFrames,
                                    AudioBufferList* ioData) -> OSStatus {
            // Provide input data - simple sine wave
            for (UInt32 buf = 0; buf < ioData->mNumberBuffers; buf++) {
                float* data = (float*)ioData->mBuffers[buf].mData;
                for (UInt32 frame = 0; frame < inNumberFrames; frame++) {
                    data[frame] = 0.5f * sinf(2.0f * M_PI * 440.0f * (frame + inTimeStamp->mSampleTime) / 44100.0f);
                }
            }
            return noErr;
        };
        inputCallback.inputProcRefCon = nullptr;
        
        status = AudioUnitSetProperty(audioUnit, kAudioUnitProperty_SetRenderCallback,
                                     kAudioUnitScope_Input, 0, &inputCallback, sizeof(inputCallback));
        if (status != noErr) {
            std::cout << "✗ Could not set input callback: " << status << " (" << GetMacOSStatusErrorString(status) << ")\n";
        } else {
            std::cout << "✓ Set input callback\n";
        }
        
        // Try to render
        std::cout << "\nAttempting render...\n";
        
        const UInt32 frameCount = 512;
        
        // Allocate buffer list
        size_t bufferListSize = sizeof(AudioBufferList) + sizeof(AudioBuffer) * (format.mChannelsPerFrame - 1);
        AudioBufferList* bufferList = (AudioBufferList*)malloc(bufferListSize);
        bufferList->mNumberBuffers = format.mChannelsPerFrame;
        
        for (UInt32 i = 0; i < format.mChannelsPerFrame; i++) {
            bufferList->mBuffers[i].mNumberChannels = 1;
            bufferList->mBuffers[i].mDataByteSize = frameCount * sizeof(float);
            bufferList->mBuffers[i].mData = calloc(frameCount, sizeof(float));
        }
        
        // Set up render parameters
        AudioUnitRenderActionFlags flags = 0;
        AudioTimeStamp timeStamp = {0};
        timeStamp.mSampleTime = 0;
        timeStamp.mFlags = kAudioTimeStampSampleTimeValid;
        
        // Attempt render
        status = AudioUnitRender(audioUnit, &flags, &timeStamp, 0, frameCount, bufferList);
        
        if (status != noErr) {
            std::cout << "✗ Render failed: " << status << " (" << GetMacOSStatusErrorString(status) << ")\n";
            std::cout << "  Error code: 0x" << std::hex << status << std::dec << "\n";
        } else {
            std::cout << "✓ Render succeeded!\n";
            
            // Check output
            float maxLevel = 0;
            for (UInt32 buf = 0; buf < bufferList->mNumberBuffers; buf++) {
                float* data = (float*)bufferList->mBuffers[buf].mData;
                for (UInt32 frame = 0; frame < frameCount; frame++) {
                    float level = fabsf(data[frame]);
                    if (level > maxLevel) maxLevel = level;
                }
            }
            std::cout << "  Output level: " << maxLevel << "\n";
        }
        
        // Try alternate render method (pull from input)
        std::cout << "\nTrying alternate render (AudioUnitRender on input)...\n";
        
        // Reset buffers
        for (UInt32 i = 0; i < format.mChannelsPerFrame; i++) {
            memset(bufferList->mBuffers[i].mData, 0, frameCount * sizeof(float));
        }
        
        // First, render input into our buffer
        flags = 0;
        status = AudioUnitRender(audioUnit, &flags, &timeStamp, 0, frameCount, bufferList);
        
        if (status != noErr) {
            std::cout << "✗ Input render failed: " << status << " (" << GetMacOSStatusErrorString(status) << ")\n";
        } else {
            std::cout << "✓ Input render succeeded\n";
            
            // Check if we got data
            float maxLevel = 0;
            for (UInt32 buf = 0; buf < bufferList->mNumberBuffers; buf++) {
                float* data = (float*)bufferList->mBuffers[buf].mData;
                for (UInt32 frame = 0; frame < frameCount; frame++) {
                    float level = fabsf(data[frame]);
                    if (level > maxLevel) maxLevel = level;
                }
            }
            std::cout << "  Input level: " << maxLevel << "\n";
        }
        
        // Cleanup
        for (UInt32 i = 0; i < format.mChannelsPerFrame; i++) {
            free(bufferList->mBuffers[i].mData);
        }
        free(bufferList);
        
        AudioUnitUninitialize(audioUnit);
        AudioComponentInstanceDispose(audioUnit);
        
        std::cout << "\n==================================\n";
        std::cout << "Diagnostic Complete\n";
        std::cout << "==================================\n";
    }
    
    return 0;
}