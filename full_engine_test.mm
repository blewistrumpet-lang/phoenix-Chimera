// Full engine test for ChimeraPhoenix
#import <Foundation/Foundation.h>
#import <AudioToolbox/AudioToolbox.h>
#include <iostream>
#include <vector>
#include <iomanip>
#include <fstream>
#include <chrono>

struct TestResult {
    std::string engineName;
    int parameterID;
    bool passed;
    float peakLevel;
    float processingTimeMs;
    std::string notes;
};

void generateHTMLReport(const std::vector<TestResult>& results) {
    std::ofstream html("chimera_test_report.html");
    
    html << "<!DOCTYPE html><html><head><title>ChimeraPhoenix Test Report</title>";
    html << "<style>";
    html << "body { font-family: Arial; margin: 20px; background: #f5f5f5; }";
    html << "h1 { color: #333; }";
    html << "table { width: 100%; border-collapse: collapse; background: white; }";
    html << "th { background: #4CAF50; color: white; padding: 10px; }";
    html << "td { padding: 8px; border: 1px solid #ddd; }";
    html << ".pass { color: green; font-weight: bold; }";
    html << ".fail { color: red; font-weight: bold; }";
    html << "</style></head><body>";
    
    html << "<h1>ChimeraPhoenix Engine Test Report</h1>";
    html << "<p>Generated: " << __DATE__ << " " << __TIME__ << "</p>";
    
    // Summary
    int passed = 0, failed = 0;
    for (const auto& r : results) {
        if (r.passed) passed++; else failed++;
    }
    
    html << "<h2>Summary</h2>";
    html << "<p>Total Tests: " << results.size() << "<br>";
    html << "Passed: <span class='pass'>" << passed << "</span><br>";
    html << "Failed: <span class='fail'>" << failed << "</span><br>";
    html << "Pass Rate: " << std::fixed << std::setprecision(1) 
         << (passed * 100.0 / results.size()) << "%</p>";
    
    // Results table
    html << "<h2>Test Results</h2>";
    html << "<table>";
    html << "<tr><th>Engine</th><th>Status</th><th>Peak Level</th><th>Processing Time</th><th>Notes</th></tr>";
    
    for (const auto& r : results) {
        html << "<tr>";
        html << "<td>" << r.engineName << "</td>";
        html << "<td class='" << (r.passed ? "pass'>PASS" : "fail'>FAIL") << "</td>";
        html << "<td>" << std::fixed << std::setprecision(3) << r.peakLevel << "</td>";
        html << "<td>" << std::fixed << std::setprecision(2) << r.processingTimeMs << " ms</td>";
        html << "<td>" << r.notes << "</td>";
        html << "</tr>";
    }
    
    html << "</table></body></html>";
    html.close();
    
    std::cout << "\nHTML report saved to: chimera_test_report.html\n";
}

void runFullEngineTests() {
    std::cout << "=========================================\n";
    std::cout << "ChimeraPhoenix Full Engine Test\n";
    std::cout << "=========================================\n\n";
    
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
        return;
    }
    
    AudioUnit audioUnit;
    OSStatus status = AudioComponentInstanceNew(comp, &audioUnit);
    if (status != noErr) {
        std::cout << "ERROR: Could not create instance\n";
        return;
    }
    
    // Initialize
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
    
    // Get parameter list
    UInt32 paramListSize = 0;
    AudioUnitGetPropertyInfo(audioUnit, kAudioUnitProperty_ParameterList,
                             kAudioUnitScope_Global, 0, &paramListSize, NULL);
    
    int numParams = paramListSize / sizeof(AudioUnitParameterID);
    AudioUnitParameterID* paramList = (AudioUnitParameterID*)malloc(paramListSize);
    AudioUnitGetProperty(audioUnit, kAudioUnitProperty_ParameterList,
                        kAudioUnitScope_Global, 0, paramList, &paramListSize);
    
    std::cout << "Found " << numParams << " parameters\n\n";
    
    // Test results
    std::vector<TestResult> results;
    
    // Test each slot's engine parameter
    std::cout << "Testing engine slots...\n";
    std::cout << "-----------------------\n";
    
    for (int slot = 1; slot <= 6; slot++) {
        // Find the engine selection parameter for this slot
        for (int i = 0; i < numParams; i++) {
            AudioUnitParameterInfo paramInfo;
            UInt32 size = sizeof(paramInfo);
            AudioUnitGetProperty(audioUnit, kAudioUnitProperty_ParameterInfo,
                                kAudioUnitScope_Global, paramList[i], &paramInfo, &size);
            
            std::string paramName(paramInfo.name);
            
            // Look for slot engine parameters
            if (paramName.find("Slot " + std::to_string(slot) + " Engine") != std::string::npos) {
                std::cout << "Testing " << paramName << "... ";
                
                TestResult result;
                result.engineName = paramName;
                result.parameterID = paramList[i];
                
                // Set to a non-bypass value (1 = first real engine)
                AudioUnitSetParameter(audioUnit, paramList[i], kAudioUnitScope_Global, 0, 1, 0);
                
                // Create test buffer
                UInt32 frameCount = 512;
                AudioBufferList bufferList;
                bufferList.mNumberBuffers = 2;
                float leftData[512], rightData[512];
                
                // Fill with test signal
                for (int j = 0; j < frameCount; j++) {
                    leftData[j] = 0.3f * sinf(2.0f * M_PI * 440.0f * j / format.mSampleRate);
                    rightData[j] = leftData[j];
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
                
                auto startTime = std::chrono::high_resolution_clock::now();
                
                status = AudioUnitRender(audioUnit, &flags, &timeStamp, 0, frameCount, &bufferList);
                
                auto endTime = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
                
                result.processingTimeMs = duration.count() / 1000.0f;
                result.passed = (status == noErr);
                
                // Check peak level
                result.peakLevel = 0;
                for (int j = 0; j < frameCount; j++) {
                    float absL = fabsf(leftData[j]);
                    float absR = fabsf(rightData[j]);
                    if (absL > result.peakLevel) result.peakLevel = absL;
                    if (absR > result.peakLevel) result.peakLevel = absR;
                }
                
                if (result.passed) {
                    if (result.peakLevel > 1.0f) {
                        result.notes = "Output clipping";
                        result.passed = false;
                    } else if (result.peakLevel < 0.001f) {
                        result.notes = "No output detected";
                    } else {
                        result.notes = "OK";
                    }
                    std::cout << "✓ PASS";
                } else {
                    result.notes = "Render failed";
                    std::cout << "✗ FAIL";
                }
                
                std::cout << " (peak: " << result.peakLevel << ")\n";
                
                results.push_back(result);
                
                // Reset to bypass
                AudioUnitSetParameter(audioUnit, paramList[i], kAudioUnitScope_Global, 0, 0, 0);
                
                break; // Found the slot parameter
            }
        }
    }
    
    // Cleanup
    free(paramList);
    AudioUnitUninitialize(audioUnit);
    AudioComponentInstanceDispose(audioUnit);
    
    // Generate report
    std::cout << "\n=========================================\n";
    std::cout << "Test Complete\n";
    std::cout << "=========================================\n";
    
    int passed = 0, failed = 0;
    for (const auto& r : results) {
        if (r.passed) passed++; else failed++;
    }
    
    std::cout << "Passed: " << passed << "/" << results.size() << "\n";
    std::cout << "Failed: " << failed << "/" << results.size() << "\n";
    
    generateHTMLReport(results);
}

int main() {
    @autoreleasepool {
        runFullEngineTests();
        
        // Open the report
        system("open chimera_test_report.html");
    }
    return 0;
}