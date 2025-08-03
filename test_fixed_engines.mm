#import <Foundation/Foundation.h>
#import <AudioToolbox/AudioToolbox.h>
#include <iostream>
#include <vector>
#include <cmath>

int main() {
    @autoreleasepool {
        // Load ChimeraPhoenix
        AudioComponentDescription desc = {
            .componentType = 'aumf',
            .componentSubType = 'Chmr',
            .componentManufacturer = 'Chim',
            .componentFlags = 0,
            .componentFlagsMask = 0
        };
        
        AudioComponent comp = AudioComponentFindNext(NULL, &desc);
        if (\!comp) {
            std::cout << "ChimeraPhoenix not found\n";
            return 1;
        }
        
        AudioUnit audioUnit;
        OSStatus status = AudioComponentInstanceNew(comp, &audioUnit);
        if (status \!= noErr) {
            std::cout << "Could not create instance: " << status << "\n";
            return 1;
        }
        
        // Set format
        AudioStreamBasicDescription format = {0};
        format.mSampleRate = 44100;
        format.mFormatID = kAudioFormatLinearPCM;
        format.mFormatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked | kAudioFormatFlagIsNonInterleaved;
        format.mFramesPerPacket = 1;
        format.mChannelsPerFrame = 2;
        format.mBitsPerChannel = 32;
        format.mBytesPerPacket = 4;
        format.mBytesPerFrame = 4;
        
        AudioUnitSetProperty(audioUnit, kAudioUnitProperty_StreamFormat,
                           kAudioUnitScope_Input, 0, &format, sizeof(format));
        AudioUnitSetProperty(audioUnit, kAudioUnitProperty_StreamFormat,
                           kAudioUnitScope_Output, 0, &format, sizeof(format));
        
        // Set up input callback
        struct CallbackData {
            float phase = 0.0f;
        };
        
        CallbackData callbackData;
        
        AURenderCallbackStruct inputCallback;
        inputCallback.inputProc = [](void* inRefCon,
                                    AudioUnitRenderActionFlags* ioActionFlags,
                                    const AudioTimeStamp* inTimeStamp,
                                    UInt32 inBusNumber,
                                    UInt32 inNumberFrames,
                                    AudioBufferList* ioData) -> OSStatus {
            CallbackData* data = (CallbackData*)inRefCon;
            
            // Generate test signal
            for (UInt32 buf = 0; buf < ioData->mNumberBuffers; buf++) {
                float* output = (float*)ioData->mBuffers[buf].mData;
                for (UInt32 frame = 0; frame < inNumberFrames; frame++) {
                    output[frame] = 0.3f * sinf(data->phase);
                    data->phase += 2.0f * M_PI * 440.0f / 44100.0f;
                    if (data->phase > 2.0f * M_PI) data->phase -= 2.0f * M_PI;
                }
            }
            return noErr;
        };
        inputCallback.inputProcRefCon = &callbackData;
        
        AudioUnitSetProperty(audioUnit, kAudioUnitProperty_SetRenderCallback,
                           kAudioUnitScope_Input, 0, &inputCallback, sizeof(inputCallback));
        
        // Initialize
        status = AudioUnitInitialize(audioUnit);
        if (status \!= noErr) {
            std::cout << "Could not initialize: " << status << "\n";
            return 1;
        }
        
        // Test fixed engines
        std::vector<std::pair<int, std::string>> testEngines = {
            {1, "Bypass"},
            {8, "K-Style Overdrive"},
            {2, "Classic Compressor"},
            {19, "Parametric EQ"},
            {21, "Plate Reverb"},
            {38, "Tape Echo"},
            {36, "Stereo Chorus"},
            {17, "Ladder Filter"}
        };
        
        std::cout << "\n=== Testing Fixed Engines ===\n\n";
        
        for (auto& engine : testEngines) {
            // Set slot 1 to test engine using property
            Float32 engineType = engine.first;
            AudioUnitSetProperty(audioUnit, 1000, kAudioUnitScope_Global, 0, &engineType, sizeof(engineType));
            
            // Render some audio
            AudioBufferList* bufferList = (AudioBufferList*)malloc(sizeof(AudioBufferList) + sizeof(AudioBuffer));
            bufferList->mNumberBuffers = 2;
            
            for (UInt32 i = 0; i < 2; i++) {
                bufferList->mBuffers[i].mNumberChannels = 1;
                bufferList->mBuffers[i].mDataByteSize = 512 * sizeof(float);
                bufferList->mBuffers[i].mData = calloc(512, sizeof(float));
            }
            
            AudioUnitRenderActionFlags flags = 0;
            AudioTimeStamp timeStamp = {0};
            timeStamp.mSampleTime = 0;
            timeStamp.mFlags = kAudioTimeStampSampleTimeValid;
            
            status = AudioUnitRender(audioUnit, &flags, &timeStamp, 0, 512, bufferList);
            
            // Check output level
            float maxLevel = 0.0f;
            for (UInt32 buf = 0; buf < bufferList->mNumberBuffers; buf++) {
                float* data = (float*)bufferList->mBuffers[buf].mData;
                for (UInt32 frame = 0; frame < 512; frame++) {
                    float level = fabsf(data[frame]);
                    if (level > maxLevel) maxLevel = level;
                }
            }
            
            std::cout << engine.second << ": " << (maxLevel > 0.001f ? "✓ WORKING" : "✗ SILENT") 
                      << " (level: " << maxLevel << ")\n";
            
            for (UInt32 i = 0; i < 2; i++) {
                free(bufferList->mBuffers[i].mData);
            }
            free(bufferList);
        }
        
        std::cout << "\n=== Test Complete ===\n";
        
        AudioUnitUninitialize(audioUnit);
        AudioComponentInstanceDispose(audioUnit);
    }
    
    return 0;
}
