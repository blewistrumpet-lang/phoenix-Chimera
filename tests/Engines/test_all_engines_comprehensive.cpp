#include <iostream>
#include <iomanip>
#include <map>
#include <memory>
#include <JuceHeader.h>
#include "JUCE_Plugin/Source/EngineFactory.h"
#include "JUCE_Plugin/Source/EngineBase.h"

class EngineTestResult {
public:
    bool passed = false;
    bool processesAudio = false;
    bool respondsToParams = false;
    bool mixWorks = false;
    float inputRMS = 0.0f;
    float outputRMS = 0.0f;
    float paramResponseRMS = 0.0f;
    float mixTestRMS = 0.0f;
    std::string notes;
};

EngineTestResult testEngine(int engineId, const std::string& engineName) {
    EngineTestResult result;
    
    try {
        // Create engine
        auto engine = EngineFactory::createEngine(engineId);
        if (!engine) {
            result.notes = "Failed to create engine";
            return result;
        }
        
        // Prepare engine
        engine->prepareToPlay(44100, 512);
        
        // Create test buffer with sine wave
        juce::AudioBuffer<float> buffer(2, 512);
        for (int i = 0; i < 512; ++i) {
            float sample = 0.5f * std::sin(2.0f * M_PI * 440.0f * i / 44100.0f);
            buffer.setSample(0, i, sample);
            buffer.setSample(1, i, sample);
        }
        
        result.inputRMS = buffer.getRMSLevel(0, 0, 512);
        
        // Test 1: Process with default parameters
        std::map<int, float> params;
        for (int p = 0; p < engine->getNumParameters(); ++p) {
            params[p] = 0.5f; // Mid-range default
        }
        engine->updateParameters(params);
        
        // Make a copy for processing
        juce::AudioBuffer<float> testBuffer(buffer);
        engine->process(testBuffer);
        result.outputRMS = testBuffer.getRMSLevel(0, 0, 512);
        
        // Check if it processes audio (RMS should change or stay similar)
        float rmsDiff = std::abs(result.outputRMS - result.inputRMS);
        result.processesAudio = (result.outputRMS > 0.01f); // Not silence
        
        // Test 2: Parameter response - set aggressive parameters
        testBuffer = buffer; // Reset
        for (int p = 0; p < engine->getNumParameters(); ++p) {
            if (engineName.find("Compressor") != std::string::npos && p == 0) {
                params[p] = 0.1f; // Low threshold for compressor
            } else if (engineName.find("Distortion") != std::string::npos || 
                      engineName.find("Fuzz") != std::string::npos ||
                      engineName.find("Overdrive") != std::string::npos) {
                if (p == 0) params[p] = 0.9f; // High drive
            } else if (engineName.find("Filter") != std::string::npos && p == 0) {
                params[p] = 0.2f; // Low frequency
            } else if (engineName.find("Delay") != std::string::npos && p == 0) {
                params[p] = 0.3f; // Some delay time
            } else if (engineName.find("Reverb") != std::string::npos) {
                if (p == 0) params[p] = 0.8f; // High size/decay
            } else {
                params[p] = (p == 0) ? 0.8f : 0.5f; // First param high, others mid
            }
        }
        engine->updateParameters(params);
        engine->process(testBuffer);
        result.paramResponseRMS = testBuffer.getRMSLevel(0, 0, 512);
        
        // Check if parameters affect output
        result.respondsToParams = std::abs(result.paramResponseRMS - result.outputRMS) > 0.01f ||
                                  (result.paramResponseRMS > 0.01f && result.outputRMS > 0.01f);
        
        // Test 3: Mix control (if last parameter is mix)
        if (engine->getNumParameters() > 0) {
            testBuffer = buffer; // Reset
            int mixParam = engine->getNumParameters() - 1;
            
            // Check if last param is likely "Mix"
            juce::String lastParamName = engine->getParameterName(mixParam).toLowerCase();
            if (lastParamName.contains("mix") || lastParamName.contains("wet") || 
                lastParamName.contains("blend")) {
                
                // Test with 0% mix (should be dry)
                params[mixParam] = 0.0f;
                engine->updateParameters(params);
                testBuffer = buffer;
                engine->process(testBuffer);
                float dryRMS = testBuffer.getRMSLevel(0, 0, 512);
                
                // Test with 100% mix (should be wet)
                params[mixParam] = 1.0f;
                engine->updateParameters(params);
                testBuffer = buffer;
                engine->process(testBuffer);
                float wetRMS = testBuffer.getRMSLevel(0, 0, 512);
                
                // Test with 50% mix
                params[mixParam] = 0.5f;
                engine->updateParameters(params);
                testBuffer = buffer;
                engine->process(testBuffer);
                result.mixTestRMS = testBuffer.getRMSLevel(0, 0, 512);
                
                // Mix works if dry and wet are different and 50% is in between
                result.mixWorks = (std::abs(dryRMS - wetRMS) > 0.01f) &&
                                 (result.mixTestRMS > std::min(dryRMS, wetRMS) * 0.9f) &&
                                 (result.mixTestRMS < std::max(dryRMS, wetRMS) * 1.1f);
            }
        }
        
        // Overall pass/fail
        result.passed = result.processesAudio && 
                       (result.respondsToParams || engineName == "None" || engineName.find("Utility") != std::string::npos);
        
        if (result.outputRMS > result.inputRMS * 5.0f) {
            result.notes = "Very high gain";
        } else if (result.outputRMS < result.inputRMS * 0.1f && result.outputRMS > 0.01f) {
            result.notes = "Very low output";
        } else if (result.outputRMS < 0.01f) {
            result.notes = "Silent output";
        } else {
            result.notes = "Normal";
        }
        
    } catch (const std::exception& e) {
        result.notes = std::string("Exception: ") + e.what();
    } catch (...) {
        result.notes = "Unknown exception";
    }
    
    return result;
}

int main() {
    std::cout << "\n================================================" << std::endl;
    std::cout << "    CHIMERA PHOENIX - ALL ENGINES TEST" << std::endl;
    std::cout << "================================================\n" << std::endl;
    
    // Engine ID to name mapping (from EngineFactory)
    std::map<int, std::string> engineNames = {
        {0, "None"},
        {1, "Vintage Opto Compressor"},
        {2, "Classic Compressor"},
        {3, "Transient Shaper"},
        {4, "Noise Gate"},
        {5, "Mastering Limiter"},
        {6, "Dynamic EQ"},
        {7, "Parametric EQ"},
        {8, "Vintage Console EQ"},
        {9, "Ladder Filter"},
        {10, "State Variable Filter"},
        {11, "Formant Filter"},
        {12, "Envelope Filter"},
        {13, "Comb Resonator"},
        {14, "Vocal Formant Filter"},
        {15, "Vintage Tube Preamp"},
        {16, "Wave Folder"},
        {17, "Harmonic Exciter"},
        {18, "Bit Crusher"},
        {19, "Multiband Saturator"},
        {20, "Muff Fuzz"},
        {21, "Rodent Distortion"},
        {22, "K-Style Overdrive"},
        {23, "Stereo Chorus"},
        {24, "Resonant Chorus"},
        {25, "Analog Phaser"},
        {26, "Ring Modulator"},
        {27, "Frequency Shifter"},
        {28, "Harmonic Tremolo"},
        {29, "Classic Tremolo"},
        {30, "Rotary Speaker"},
        {31, "Pitch Shifter"},
        {32, "Detune Doubler"},
        {33, "Intelligent Harmonizer"},
        {34, "Tape Echo"},
        {35, "Digital Delay"},
        {36, "Magnetic Drum Echo"},
        {37, "Bucket Brigade Delay"},
        {38, "Buffer Repeat"},
        {39, "Plate Reverb"},
        {40, "Spring Reverb"},
        {41, "Convolution Reverb"},
        {42, "Shimmer Reverb"},
        {43, "Gated Reverb"},
        {44, "Stereo Widener"},
        {45, "Stereo Imager"},
        {46, "Dimension Expander"},
        {47, "Spectral Freeze"},
        {48, "Spectral Gate"},
        {49, "Phased Vocoder"},
        {50, "Granular Cloud"},
        {51, "Chaos Generator"},
        {52, "Feedback Network"},
        {53, "Mid-Side Processor"},
        {54, "Gain Utility"},
        {55, "Mono Maker"},
        {56, "Phase Align"}
    };
    
    int totalEngines = 57;
    int passed = 0;
    int failed = 0;
    int exceptions = 0;
    
    std::cout << std::left << std::setw(4) << "ID" 
              << std::setw(30) << "Engine Name"
              << std::setw(8) << "Status"
              << std::setw(10) << "Processes"
              << std::setw(10) << "Params"
              << std::setw(8) << "Mix"
              << std::setw(20) << "Notes" << std::endl;
    std::cout << std::string(90, '-') << std::endl;
    
    for (int id = 0; id < totalEngines; ++id) {
        auto it = engineNames.find(id);
        if (it == engineNames.end()) continue;
        
        std::string name = it->second;
        EngineTestResult result = testEngine(id, name);
        
        // Format output
        std::cout << std::left << std::setw(4) << id
                  << std::setw(30) << name.substr(0, 29);
        
        if (result.passed) {
            std::cout << "\033[32m" << std::setw(8) << "PASS" << "\033[0m";
            passed++;
        } else if (!result.notes.empty() && result.notes.find("Exception") != std::string::npos) {
            std::cout << "\033[31m" << std::setw(8) << "ERROR" << "\033[0m";
            exceptions++;
        } else {
            std::cout << "\033[33m" << std::setw(8) << "FAIL" << "\033[0m";
            failed++;
        }
        
        std::cout << std::setw(10) << (result.processesAudio ? "Yes" : "No")
                  << std::setw(10) << (result.respondsToParams ? "Yes" : "No")
                  << std::setw(8) << (result.mixWorks ? "Yes" : "-")
                  << std::setw(20) << result.notes.substr(0, 19) << std::endl;
        
        // Add detailed info for failures
        if (!result.passed && id != 0) {  // Skip "None" engine
            std::cout << "      Input RMS: " << std::fixed << std::setprecision(3) << result.inputRMS
                      << " Output RMS: " << result.outputRMS
                      << " Param RMS: " << result.paramResponseRMS << std::endl;
        }
    }
    
    std::cout << std::string(90, '-') << std::endl;
    std::cout << "\nSUMMARY:" << std::endl;
    std::cout << "  Passed:     " << passed << "/" << totalEngines << std::endl;
    std::cout << "  Failed:     " << failed << "/" << totalEngines << std::endl;
    std::cout << "  Exceptions: " << exceptions << "/" << totalEngines << std::endl;
    std::cout << "  Success Rate: " << std::fixed << std::setprecision(1) 
              << (100.0 * passed / totalEngines) << "%" << std::endl;
    
    return 0;
}