// Basic audio passthrough test
#import <Foundation/Foundation.h>
#import <AudioToolbox/AudioToolbox.h>
#include <iostream>
#include <cmath>

int main() {
    @autoreleasepool {
        std::cout << "Basic Audio Passthrough Test\n";
        std::cout << "============================\n\n";
        
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
        
        AudioUnit audioUnit;
        OSStatus status = AudioComponentInstanceNew(comp, &audioUnit);
        if (status != noErr) {
            std::cout << "ERROR: Could not create instance (status: " << status << ")\n";
            return 1;
        }
        
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
            std::cout << "ERROR: Could not set input format (status: " << status << ")\n";
            return 1;
        }
        
        // Set output format
        status = AudioUnitSetProperty(audioUnit, kAudioUnitProperty_StreamFormat,
                                     kAudioUnitScope_Output, 0, &format, sizeof(format));
        if (status != noErr) {
            std::cout << "ERROR: Could not set output format (status: " << status << ")\n";
            return 1;
        }
        
        // NOW initialize
        status = AudioUnitInitialize(audioUnit);
        if (status != noErr) {
            std::cout << "ERROR: Could not initialize (status: " << status << ")\n";
            AudioComponentInstanceDispose(audioUnit);
            return 1;
        }
        
        std::cout << "Audio Unit loaded and initialized\n\n";
        
        // Create simple test: just pass audio through bypass
        std::cout << "Test 1: Simple sine wave through bypass\n";
        std::cout << "----------------------------------------\n";
        
        const UInt32 frameCount = 512;
        
        // Allocate buffers properly
        float* leftBuffer = (float*)calloc(frameCount, sizeof(float));
        float* rightBuffer = (float*)calloc(frameCount, sizeof(float));
        
        // Generate test signal
        float inputPeak = 0;
        for (UInt32 i = 0; i < frameCount; i++) {
            float sample = 0.5f * sinf(2.0f * M_PI * 440.0f * i / format.mSampleRate);
            leftBuffer[i] = sample;
            rightBuffer[i] = sample;
            if (fabsf(sample) > inputPeak) inputPeak = fabsf(sample);
        }
        
        std::cout << "Input peak level: " << inputPeak << "\n";
        
        // Create AudioBufferList
        AudioBufferList bufferList;
        bufferList.mNumberBuffers = 2;
        
        bufferList.mBuffers[0].mNumberChannels = 1;
        bufferList.mBuffers[0].mDataByteSize = frameCount * sizeof(float);
        bufferList.mBuffers[0].mData = leftBuffer;
        
        bufferList.mBuffers[1].mNumberChannels = 1;
        bufferList.mBuffers[1].mDataByteSize = frameCount * sizeof(float);
        bufferList.mBuffers[1].mData = rightBuffer;
        
        // Set up render parameters
        AudioUnitRenderActionFlags flags = 0;
        AudioTimeStamp timeStamp = {0};
        timeStamp.mSampleTime = 0;
        timeStamp.mFlags = kAudioTimeStampSampleTimeValid;
        
        // Process audio
        status = AudioUnitRender(audioUnit, &flags, &timeStamp, 0, frameCount, &bufferList);
        if (status != noErr) {
            std::cout << "ERROR: Render failed (status: " << status << ")\n";
        } else {
            std::cout << "Render succeeded\n";
        }
        
        // Check output
        float outputPeak = 0;
        int nonZeroSamples = 0;
        for (UInt32 i = 0; i < frameCount; i++) {
            if (leftBuffer[i] != 0 || rightBuffer[i] != 0) {
                nonZeroSamples++;
            }
            float absL = fabsf(leftBuffer[i]);
            float absR = fabsf(rightBuffer[i]);
            if (absL > outputPeak) outputPeak = absL;
            if (absR > outputPeak) outputPeak = absR;
        }
        
        std::cout << "Output peak level: " << outputPeak << "\n";
        std::cout << "Non-zero samples: " << nonZeroSamples << "/" << frameCount << "\n";
        
        // Show first few samples
        std::cout << "\nFirst 10 output samples (left channel):\n";
        for (int i = 0; i < 10; i++) {
            std::cout << "  [" << i << "]: " << leftBuffer[i] << "\n";
        }
        
        // Test 2: Try with allocation using AudioUnitRender's pull model
        std::cout << "\nTest 2: Using pull model\n";
        std::cout << "------------------------\n";
        
        // Reset buffers
        for (UInt32 i = 0; i < frameCount; i++) {
            leftBuffer[i] = 0;
            rightBuffer[i] = 0;
        }
        
        // Set up input callback
        AURenderCallbackStruct inputCallback;
        inputCallback.inputProc = [](void* inRefCon,
                                    AudioUnitRenderActionFlags* ioActionFlags,
                                    const AudioTimeStamp* inTimeStamp,
                                    UInt32 inBusNumber,
                                    UInt32 inNumberFrames,
                                    AudioBufferList* ioData) -> OSStatus {
            // Provide input data
            for (UInt32 i = 0; i < ioData->mNumberBuffers; i++) {
                float* data = (float*)ioData->mBuffers[i].mData;
                for (UInt32 j = 0; j < inNumberFrames; j++) {
                    data[j] = 0.3f * sinf(2.0f * M_PI * 440.0f * j / 44100.0f);
                }
            }
            return noErr;
        };
        inputCallback.inputProcRefCon = nullptr;
        
        status = AudioUnitSetProperty(audioUnit, kAudioUnitProperty_SetRenderCallback,
                                     kAudioUnitScope_Input, 0, &inputCallback, sizeof(inputCallback));
        if (status == noErr) {
            // Now render with the callback providing input
            status = AudioUnitRender(audioUnit, &flags, &timeStamp, 0, frameCount, &bufferList);
            
            outputPeak = 0;
            for (UInt32 i = 0; i < frameCount; i++) {
                float absL = fabsf(leftBuffer[i]);
                float absR = fabsf(rightBuffer[i]);
                if (absL > outputPeak) outputPeak = absL;
                if (absR > outputPeak) outputPeak = absR;
            }
            
            std::cout << "Output peak with callback: " << outputPeak << "\n";
        } else {
            std::cout << "Could not set render callback (status: " << status << ")\n";
        }
        
        // Cleanup
        free(leftBuffer);
        free(rightBuffer);
        AudioUnitUninitialize(audioUnit);
        AudioComponentInstanceDispose(audioUnit);
        
        std::cout << "\n============================\n";
        std::cout << "Test Complete\n";
        std::cout << "============================\n";
    }
    
    return 0;
}