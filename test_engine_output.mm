// Test to verify engines are actually outputting audio
#import <Foundation/Foundation.h>
#import <AudioToolbox/AudioToolbox.h>
#include <iostream>
#include <iomanip>
#include <cmath>

int main() {
    @autoreleasepool {
        std::cout << "=========================================\n";
        std::cout << "Engine Output Level Test\n";
        std::cout << "=========================================\n\n";
        
        // Engine names matching the dropdown
        std::vector<std::string> engineNames = {
            "Bypass", "K-Style Overdrive", "Tape Echo", "Plate Reverb",
            "Rodent Distortion", "Muff Fuzz", "Classic Tremolo",
            "Magnetic Drum Echo", "Bucket Brigade Delay", "Digital Delay",
            "Harmonic Tremolo", "Rotary Speaker", "Detune Doubler",
            "Ladder Filter", "Formant Filter", "Classic Compressor",
            "State Variable Filter", "Stereo Chorus", "Spectral Freeze",
            "Granular Cloud", "Analog Ring Modulator", "Multiband Saturator",
            "Comb Resonator", "Pitch Shifter", "Phased Vocoder",
            "Convolution Reverb", "Bit Crusher", "Frequency Shifter",
            "Wave Folder", "Shimmer Reverb", "Vocal Formant Filter",
            "Transient Shaper", "Dimension Expander", "Analog Phaser",
            "Envelope Filter", "Gated Reverb", "Harmonic Exciter",
            "Feedback Network", "Intelligent Harmonizer", "Parametric EQ",
            "Mastering Limiter", "Noise Gate", "Vintage Opto",
            "Spectral Gate", "Chaos Generator", "Buffer Repeat",
            "Vintage Console EQ", "Mid/Side Processor", "Vintage Tube Preamp",
            "Spring Reverb", "Resonant Chorus", "Stereo Widener",
            "Dynamic EQ", "Stereo Imager"
        };
        
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
            std::cout << "ERROR: Could not create instance\n";
            return 1;
        }
        
        AudioUnitInitialize(audioUnit);
        
        // Set format
        AudioStreamBasicDescription format = {
            .mSampleRate = 48000,
            .mFormatID = kAudioFormatLinearPCM,
            .mFormatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked | kAudioFormatFlagIsNonInterleaved,
            .mFramesPerPacket = 1,
            .mChannelsPerFrame = 2,
            .mBitsPerChannel = 32,
            .mBytesPerPacket = 4,
            .mBytesPerFrame = 4
        };
        
        AudioUnitSetProperty(audioUnit, kAudioUnitProperty_StreamFormat,
                            kAudioUnitScope_Input, 0, &format, sizeof(format));
        AudioUnitSetProperty(audioUnit, kAudioUnitProperty_StreamFormat,
                            kAudioUnitScope_Output, 0, &format, sizeof(format));
        
        // Find parameters
        UInt32 paramListSize = 0;
        AudioUnitGetPropertyInfo(audioUnit, kAudioUnitProperty_ParameterList,
                                 kAudioUnitScope_Global, 0, &paramListSize, NULL);
        
        int numParams = paramListSize / sizeof(AudioUnitParameterID);
        AudioUnitParameterID* paramList = (AudioUnitParameterID*)malloc(paramListSize);
        AudioUnitGetProperty(audioUnit, kAudioUnitProperty_ParameterList,
                            kAudioUnitScope_Global, 0, paramList, &paramListSize);
        
        AudioUnitParameterID slot1EngineParam = -1;
        AudioUnitParameterID slot1Params[10];
        int paramCount = 0;
        
        for (int i = 0; i < numParams; i++) {
            AudioUnitParameterInfo paramInfo;
            UInt32 size = sizeof(paramInfo);
            AudioUnitGetProperty(audioUnit, kAudioUnitProperty_ParameterInfo,
                                kAudioUnitScope_Global, paramList[i], &paramInfo, &size);
            
            std::string name(paramInfo.name);
            if (name == "Slot 1 Engine") {
                slot1EngineParam = paramList[i];
            } else if (name.find("Slot 1 Param") != std::string::npos && paramCount < 10) {
                slot1Params[paramCount++] = paramList[i];
            }
        }
        
        if (slot1EngineParam == -1) {
            std::cout << "ERROR: Could not find Slot 1 Engine parameter\n";
            return 1;
        }
        
        std::cout << "Testing " << engineNames.size() << " engines...\n";
        std::cout << "Format: Engine Name -> Input Level -> Output Level (Status)\n";
        std::cout << "-------------------------------------------------------------\n";
        
        // Test a selection of engines
        int testEngines[] = {1, 3, 5, 13, 15, 17, 26, 29, 33, 39}; // Various engine types
        
        for (int idx : testEngines) {
            if (idx >= engineNames.size()) continue;
            
            std::cout << std::setw(25) << std::left << engineNames[idx] << ": ";
            
            // Set engine
            AudioUnitSetParameter(audioUnit, slot1EngineParam, kAudioUnitScope_Global, 0, idx, 0);
            
            // Set all parameters to 0.5 (middle position)
            for (int p = 0; p < paramCount; p++) {
                AudioUnitSetParameter(audioUnit, slot1Params[p], kAudioUnitScope_Global, 0, 0.5f, 0);
            }
            
            // Create test buffer with sine wave
            UInt32 frameCount = 512;
            AudioBufferList bufferList;
            bufferList.mNumberBuffers = 2;
            float leftData[512], rightData[512];
            
            // Generate test signal
            float inputLevel = 0;
            for (int j = 0; j < frameCount; j++) {
                float sample = 0.5f * sinf(2.0f * M_PI * 1000.0f * j / format.mSampleRate);
                leftData[j] = sample;
                rightData[j] = sample;
                if (fabsf(sample) > inputLevel) inputLevel = fabsf(sample);
            }
            
            bufferList.mBuffers[0].mNumberChannels = 1;
            bufferList.mBuffers[0].mDataByteSize = frameCount * sizeof(float);
            bufferList.mBuffers[0].mData = leftData;
            
            bufferList.mBuffers[1].mNumberChannels = 1;
            bufferList.mBuffers[1].mDataByteSize = frameCount * sizeof(float);
            bufferList.mBuffers[1].mData = rightData;
            
            // Process
            AudioUnitRenderActionFlags flags = 0;
            AudioTimeStamp timeStamp = {0};
            timeStamp.mFlags = kAudioTimeStampSampleTimeValid;
            
            status = AudioUnitRender(audioUnit, &flags, &timeStamp, 0, frameCount, &bufferList);
            
            // Check output
            float outputLevel = 0;
            bool hasNonZeroOutput = false;
            for (int j = 0; j < frameCount; j++) {
                if (leftData[j] != 0 || rightData[j] != 0) {
                    hasNonZeroOutput = true;
                }
                float absL = fabsf(leftData[j]);
                float absR = fabsf(rightData[j]);
                if (absL > outputLevel) outputLevel = absL;
                if (absR > outputLevel) outputLevel = absR;
            }
            
            std::cout << std::fixed << std::setprecision(3) 
                     << inputLevel << " -> " << outputLevel;
            
            if (status != noErr) {
                std::cout << " (ERROR)";
            } else if (!hasNonZeroOutput) {
                std::cout << " (SILENT!)";
            } else if (outputLevel < 0.001f) {
                std::cout << " (TOO QUIET)";
            } else if (outputLevel > inputLevel * 1.2f) {
                std::cout << " (BOOSTED)";
            } else if (outputLevel < inputLevel * 0.8f) {
                std::cout << " (ATTENUATED)";
            } else {
                std::cout << " (OK)";
            }
            
            std::cout << "\n";
            
            // Reset to bypass
            AudioUnitSetParameter(audioUnit, slot1EngineParam, kAudioUnitScope_Global, 0, 0, 0);
        }
        
        // Test with bypass for reference
        std::cout << std::setw(25) << std::left << "Bypass (Reference)" << ": ";
        AudioUnitSetParameter(audioUnit, slot1EngineParam, kAudioUnitScope_Global, 0, 0, 0);
        
        // Create test buffer
        UInt32 frameCount = 512;
        AudioBufferList bufferList;
        bufferList.mNumberBuffers = 2;
        float leftData[512], rightData[512];
        
        float inputLevel = 0;
        for (int j = 0; j < frameCount; j++) {
            float sample = 0.5f * sinf(2.0f * M_PI * 1000.0f * j / format.mSampleRate);
            leftData[j] = sample;
            rightData[j] = sample;
            if (fabsf(sample) > inputLevel) inputLevel = fabsf(sample);
        }
        
        bufferList.mBuffers[0].mNumberChannels = 1;
        bufferList.mBuffers[0].mDataByteSize = frameCount * sizeof(float);
        bufferList.mBuffers[0].mData = leftData;
        bufferList.mBuffers[1].mNumberChannels = 1;
        bufferList.mBuffers[1].mDataByteSize = frameCount * sizeof(float);
        bufferList.mBuffers[1].mData = rightData;
        
        AudioUnitRenderActionFlags flags = 0;
        AudioTimeStamp timeStamp = {0};
        timeStamp.mFlags = kAudioTimeStampSampleTimeValid;
        AudioUnitRender(audioUnit, &flags, &timeStamp, 0, frameCount, &bufferList);
        
        float outputLevel = 0;
        for (int j = 0; j < frameCount; j++) {
            float absL = fabsf(leftData[j]);
            float absR = fabsf(rightData[j]);
            if (absL > outputLevel) outputLevel = absL;
            if (absR > outputLevel) outputLevel = absR;
        }
        
        std::cout << std::fixed << std::setprecision(3) 
                 << inputLevel << " -> " << outputLevel << " (REFERENCE)\n";
        
        // Cleanup
        free(paramList);
        AudioUnitUninitialize(audioUnit);
        AudioComponentInstanceDispose(audioUnit);
        
        std::cout << "\n=========================================\n";
        std::cout << "Test Complete\n";
        std::cout << "=========================================\n";
        std::cout << "\nNote: SILENT means no output at all\n";
        std::cout << "      TOO QUIET means output < 0.001\n";
        std::cout << "      OK means output is within expected range\n";
    }
    
    return 0;
}