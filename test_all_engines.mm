// Test all 56 Chimera engines
#import <Foundation/Foundation.h>
#import <AudioToolbox/AudioToolbox.h>
#include <iostream>
#include <vector>
#include <iomanip>
#include <fstream>
#include <chrono>
#include <map>

struct EngineTest {
    std::string name;
    int choiceIndex;
    bool passed;
    float peakLevel;
    float processingTime;
    std::string category;
};

std::string categorize(const std::string& name) {
    if (name.find("Compressor") != std::string::npos || 
        name.find("Limiter") != std::string::npos ||
        name.find("Gate") != std::string::npos) return "Dynamics";
    if (name.find("EQ") != std::string::npos || 
        name.find("Filter") != std::string::npos) return "Filters & EQ";
    if (name.find("Reverb") != std::string::npos || 
        name.find("Delay") != std::string::npos ||
        name.find("Echo") != std::string::npos) return "Time-Based";
    if (name.find("Chorus") != std::string::npos || 
        name.find("Phaser") != std::string::npos ||
        name.find("Tremolo") != std::string::npos ||
        name.find("Rotary") != std::string::npos) return "Modulation";
    if (name.find("Distortion") != std::string::npos || 
        name.find("Fuzz") != std::string::npos ||
        name.find("Overdrive") != std::string::npos ||
        name.find("Saturator") != std::string::npos ||
        name.find("Tube") != std::string::npos ||
        name.find("Crusher") != std::string::npos ||
        name.find("Folder") != std::string::npos) return "Distortion";
    if (name.find("Stereo") != std::string::npos ||
        name.find("Mid/Side") != std::string::npos ||
        name.find("Widener") != std::string::npos ||
        name.find("Imager") != std::string::npos) return "Stereo Processing";
    return "Special";
}

void generateReport(const std::vector<EngineTest>& tests) {
    std::ofstream html("engine_test_report.html");
    
    html << "<!DOCTYPE html><html><head><title>Chimera Engine Test Report</title>";
    html << "<style>";
    html << "body { font-family: 'Segoe UI', Arial; margin: 20px; background: linear-gradient(135deg, #667eea, #764ba2); }";
    html << ".container { max-width: 1200px; margin: 0 auto; background: white; border-radius: 20px; padding: 30px; box-shadow: 0 20px 60px rgba(0,0,0,0.3); }";
    html << "h1 { color: #333; border-bottom: 3px solid #667eea; padding-bottom: 10px; }";
    html << ".stats { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 20px; margin: 30px 0; }";
    html << ".stat { background: linear-gradient(135deg, #667eea, #764ba2); color: white; padding: 20px; border-radius: 10px; text-align: center; }";
    html << ".stat-value { font-size: 2.5em; font-weight: bold; }";
    html << ".stat-label { font-size: 0.9em; text-transform: uppercase; opacity: 0.9; }";
    html << "table { width: 100%; border-collapse: collapse; margin: 20px 0; }";
    html << "th { background: #667eea; color: white; padding: 12px; text-align: left; }";
    html << "td { padding: 10px; border-bottom: 1px solid #e0e0e0; }";
    html << "tr:hover { background: #f8f9fa; }";
    html << ".pass { color: #22c55e; font-weight: bold; }";
    html << ".fail { color: #ef4444; font-weight: bold; }";
    html << ".category { background: #f8f9fa; font-weight: bold; padding: 15px; }";
    html << "</style></head><body>";
    
    html << "<div class='container'>";
    html << "<h1>🎵 Chimera Engine Test Report</h1>";
    html << "<p>Generated: " << __DATE__ << " " << __TIME__ << "</p>";
    
    // Calculate stats
    int passed = 0, failed = 0;
    float avgPeak = 0, avgTime = 0;
    std::map<std::string, std::vector<EngineTest>> byCategory;
    
    for (const auto& t : tests) {
        if (t.passed) passed++; else failed++;
        avgPeak += t.peakLevel;
        avgTime += t.processingTime;
        byCategory[t.category].push_back(t);
    }
    
    avgPeak /= tests.size();
    avgTime /= tests.size();
    
    // Summary
    html << "<div class='stats'>";
    html << "<div class='stat'><div class='stat-value'>" << tests.size() << "</div><div class='stat-label'>Total Engines</div></div>";
    html << "<div class='stat'><div class='stat-value'>" << passed << "</div><div class='stat-label'>Passed</div></div>";
    html << "<div class='stat'><div class='stat-value'>" << failed << "</div><div class='stat-label'>Failed</div></div>";
    html << "<div class='stat'><div class='stat-value'>" << std::fixed << std::setprecision(1) 
         << (passed * 100.0f / tests.size()) << "%</div><div class='stat-label'>Pass Rate</div></div>";
    html << "</div>";
    
    // Results by category
    html << "<h2>Test Results by Category</h2>";
    html << "<table>";
    html << "<tr><th>Engine</th><th>Status</th><th>Peak Level</th><th>Processing (ms)</th></tr>";
    
    for (const auto& [cat, engines] : byCategory) {
        html << "<tr><td colspan='4' class='category'>📁 " << cat << " (" << engines.size() << " engines)</td></tr>";
        for (const auto& e : engines) {
            html << "<tr>";
            html << "<td>" << e.name << "</td>";
            html << "<td class='" << (e.passed ? "pass'>PASS" : "fail'>FAIL") << "</td>";
            html << "<td>" << std::fixed << std::setprecision(3) << e.peakLevel << "</td>";
            html << "<td>" << std::fixed << std::setprecision(2) << e.processingTime << "</td>";
            html << "</tr>";
        }
    }
    
    html << "</table>";
    html << "</div></body></html>";
    html.close();
}

int main() {
    @autoreleasepool {
        std::cout << "=========================================\n";
        std::cout << "Chimera Phoenix All Engines Test\n";
        std::cout << "=========================================\n\n";
        
        // Engine list matching the plugin's dropdown
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
        
        // Find slot 1 engine parameter
        UInt32 paramListSize = 0;
        AudioUnitGetPropertyInfo(audioUnit, kAudioUnitProperty_ParameterList,
                                 kAudioUnitScope_Global, 0, &paramListSize, NULL);
        
        int numParams = paramListSize / sizeof(AudioUnitParameterID);
        AudioUnitParameterID* paramList = (AudioUnitParameterID*)malloc(paramListSize);
        AudioUnitGetProperty(audioUnit, kAudioUnitProperty_ParameterList,
                            kAudioUnitScope_Global, 0, paramList, &paramListSize);
        
        AudioUnitParameterID slot1EngineParam = -1;
        for (int i = 0; i < numParams; i++) {
            AudioUnitParameterInfo paramInfo;
            UInt32 size = sizeof(paramInfo);
            AudioUnitGetProperty(audioUnit, kAudioUnitProperty_ParameterInfo,
                                kAudioUnitScope_Global, paramList[i], &paramInfo, &size);
            
            if (strcmp(paramInfo.name, "Slot 1 Engine") == 0) {
                slot1EngineParam = paramList[i];
                break;
            }
        }
        
        if (slot1EngineParam == -1) {
            std::cout << "ERROR: Could not find Slot 1 Engine parameter\n";
            return 1;
        }
        
        // Test each engine
        std::vector<EngineTest> results;
        std::cout << "Testing " << engineNames.size() << " engines...\n";
        std::cout << "-----------------------------------------\n";
        
        for (int i = 0; i < engineNames.size(); i++) {
            if (i == 0) continue; // Skip Bypass
            
            EngineTest test;
            test.name = engineNames[i];
            test.choiceIndex = i;
            test.category = categorize(test.name);
            
            std::cout << std::setw(30) << std::left << test.name << ": ";
            
            // Set engine
            AudioUnitSetParameter(audioUnit, slot1EngineParam, kAudioUnitScope_Global, 0, i, 0);
            
            // Create test buffer with sine wave
            UInt32 frameCount = 512;
            AudioBufferList bufferList;
            bufferList.mNumberBuffers = 2;
            float leftData[512], rightData[512];
            
            for (int j = 0; j < frameCount; j++) {
                float t = j / format.mSampleRate;
                leftData[j] = 0.3f * sinf(2.0f * M_PI * 440.0f * t);
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
            
            auto start = std::chrono::high_resolution_clock::now();
            status = AudioUnitRender(audioUnit, &flags, &timeStamp, 0, frameCount, &bufferList);
            auto end = std::chrono::high_resolution_clock::now();
            
            test.processingTime = std::chrono::duration<float, std::milli>(end - start).count();
            test.passed = (status == noErr);
            
            // Check output
            test.peakLevel = 0;
            for (int j = 0; j < frameCount; j++) {
                float absL = fabsf(leftData[j]);
                float absR = fabsf(rightData[j]);
                if (absL > test.peakLevel) test.peakLevel = absL;
                if (absR > test.peakLevel) test.peakLevel = absR;
            }
            
            if (test.passed) {
                std::cout << "✓ PASS";
            } else {
                std::cout << "✗ FAIL";
            }
            std::cout << " (peak: " << std::fixed << std::setprecision(3) << test.peakLevel 
                     << ", time: " << std::setprecision(2) << test.processingTime << "ms)\n";
            
            results.push_back(test);
            
            // Reset to bypass
            AudioUnitSetParameter(audioUnit, slot1EngineParam, kAudioUnitScope_Global, 0, 0, 0);
        }
        
        // Cleanup
        free(paramList);
        AudioUnitUninitialize(audioUnit);
        AudioComponentInstanceDispose(audioUnit);
        
        // Summary
        std::cout << "\n=========================================\n";
        std::cout << "Test Complete\n";
        std::cout << "=========================================\n";
        
        int passed = 0;
        for (const auto& r : results) {
            if (r.passed) passed++;
        }
        
        std::cout << "Total: " << results.size() << " engines\n";
        std::cout << "Passed: " << passed << "\n";
        std::cout << "Failed: " << (results.size() - passed) << "\n";
        std::cout << "Pass Rate: " << std::fixed << std::setprecision(1) 
                 << (passed * 100.0f / results.size()) << "%\n";
        
        // Generate HTML report
        generateReport(results);
        std::cout << "\nHTML report saved to: engine_test_report.html\n";
        
        // Open report
        system("open engine_test_report.html");
    }
    
    return 0;
}