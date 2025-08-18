/**
 * Comprehensive Parameter Mapping Test
 * Date: August 17, 2025
 * Purpose: Verify all parameter mappings are consistent across the codebase
 */

#include <iostream>
#include <iomanip>
#include <memory>
#include <vector>
#include <map>
#include <set>
#include <string>
#include <cmath>
#include <JuceHeader.h>

#include "JUCE_Plugin/Source/EngineFactory.h"
#include "JUCE_Plugin/Source/EngineBase.h"
#include "JUCE_Plugin/Source/PluginProcessor.h"

struct ParameterInfo {
    int index;
    std::string name;
    float defaultValue;
    float minValue;
    float maxValue;
    bool isValid;
};

struct EngineParameterReport {
    int engineID;
    std::string engineName;
    int numParameters;
    int mixParameterIndex;
    bool mixParameterValid;
    std::vector<ParameterInfo> parameters;
    std::vector<std::string> issues;
};

class ParameterMappingTest {
private:
    ChimeraAudioProcessor processor;
    std::vector<EngineParameterReport> reports;
    
    const std::vector<std::pair<int, std::string>> allEngines = {
        {0, "NoneEngine"},
        {1, "ClassicCompressor"},
        {2, "VintageOptoCompressor_Platinum"},
        {3, "VCA_Compressor"},
        {4, "NoiseGate_Platinum"},
        {5, "TransientShaper_Platinum"},
        {6, "MasteringLimiter_Platinum"},
        {7, "ParametricEQ"},
        {8, "VintageConsoleEQ"},
        {9, "DynamicEQ"},
        {10, "AnalogPhaser"},
        {11, "EnvelopeFilter"},
        {12, "StateVariableFilter"},
        {13, "FormantFilter"},
        {14, "LadderFilter"},
        {15, "VintageTubePreamp"},
        {16, "TapeDistortion"},
        {17, "KStyleOverdrive"},
        {18, "BitCrusher"},
        {19, "WaveFolder"},
        {20, "MuffFuzz"},
        {21, "RodentDistortion"},
        {22, "MultibandSaturator"},
        {23, "StereoChorus"},
        {24, "VintageFlanger"},
        {25, "ClassicTremolo"},
        {26, "HarmonicTremolo"},
        {27, "RotarySpeaker"},
        {28, "RingModulator"},
        {29, "FrequencyShifter"},
        {30, "PitchShifter"},
        {31, "HarmonicExciter"},
        {32, "VocalFormant"},
        {33, "ResonantChorus"},
        {34, "DigitalDelay"},
        {35, "TapeEcho"},
        {36, "BucketBrigadeDelay"},
        {37, "MagneticDrumEcho"},
        {38, "BufferRepeat"},
        {39, "PlateReverb"},
        {40, "SpringReverb_Platinum"},
        {41, "ConvolutionReverb"},
        {42, "ShimmerReverb"},
        {43, "GatedReverb"},
        {44, "StereoWidener"},
        {45, "StereoImager"},
        {46, "MidSideProcessor"},
        {47, "DimensionExpander"},
        {48, "CombResonator"},
        {49, "SpectralFreeze"},
        {50, "GranularCloud"},
        {51, "ChaosGenerator"},
        {52, "FeedbackNetwork"},
        {53, "PhaseAlign_Platinum"},
        {54, "GainUtility"},
        {55, "MonoMaker"},
        {56, "SpectralGate"}
    };
    
    bool isLikelyMixParameter(const std::string& name) {
        std::string lower = name;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        return (lower.find("mix") != std::string::npos ||
                lower.find("wet") != std::string::npos ||
                lower.find("dry") != std::string::npos ||
                lower.find("blend") != std::string::npos ||
                lower.find("amount") != std::string::npos);
    }
    
    bool isReasonableDefault(float value, const std::string& paramName) {
        // Mix parameters should typically default to 0.5-1.0 (50-100% wet)
        if (isLikelyMixParameter(paramName)) {
            return value >= 0.3f && value <= 1.0f;
        }
        // Other parameters typically start at 0 or 0.5
        return value >= 0.0f && value <= 1.0f;
    }
    
public:
    void testEngine(int engineID, const std::string& engineName) {
        EngineParameterReport report;
        report.engineID = engineID;
        report.engineName = engineName;
        report.mixParameterValid = false;
        
        std::cout << "\n[" << std::setw(2) << engineID << "] " 
                  << std::setw(30) << std::left << engineName << "\n";
        std::cout << std::string(50, '-') << "\n";
        
        try {
            // Create engine
            auto engine = EngineFactory::createEngine(engineID);
            if (!engine) {
                report.issues.push_back("Failed to create engine");
                reports.push_back(report);
                std::cout << "  ❌ Failed to create engine\n";
                return;
            }
            
            // Get parameter count
            report.numParameters = engine->getNumParameters();
            std::cout << "  Parameters: " << report.numParameters << "\n";
            
            // Get mix parameter index from processor
            report.mixParameterIndex = processor.getMixParameterIndex(engineID);
            std::cout << "  Mix Parameter Index: ";
            
            if (report.mixParameterIndex < 0) {
                std::cout << "NONE (bypass/utility engine)\n";
                if (engineID > 0) { // NoneEngine is expected to have no mix
                    // Check if this is a utility that shouldn't have mix
                    bool isUtility = (engineID == 28 || // RingModulator
                                     engineID == 46 || // MidSideProcessor
                                     engineID == 50 || // GranularCloud
                                     engineID == 53 || // PhaseAlign
                                     engineID == 54 || // GainUtility
                                     engineID == 55 || // MonoMaker
                                     engineID == 49); // SpectralFreeze
                    
                    if (!isUtility) {
                        report.issues.push_back("Missing mix parameter mapping");
                    }
                }
            } else {
                std::cout << report.mixParameterIndex;
                
                // Validate mix parameter index
                if (report.mixParameterIndex >= report.numParameters) {
                    std::cout << " ❌ OUT OF RANGE!";
                    report.issues.push_back("Mix parameter index out of range");
                } else {
                    std::cout << " ✅";
                    report.mixParameterValid = true;
                    
                    // Check if the parameter name makes sense
                    juce::String mixParamName = engine->getParameterName(report.mixParameterIndex);
                    std::cout << " (\"" << mixParamName << "\")";
                    
                    if (!isLikelyMixParameter(mixParamName.toStdString())) {
                        std::cout << " ⚠️ Unexpected name";
                        report.issues.push_back("Mix parameter has unexpected name: " + mixParamName.toStdString());
                    }
                }
                std::cout << "\n";
            }
            
            // Test all parameters
            std::cout << "\n  Parameter Details:\n";
            std::map<int, float> testParams;
            std::set<std::string> paramNames;
            
            for (int i = 0; i < report.numParameters; ++i) {
                ParameterInfo param;
                param.index = i;
                param.name = engine->getParameterName(i).toStdString();
                param.isValid = true;
                
                // Check for duplicate parameter names
                if (paramNames.count(param.name) > 0) {
                    param.isValid = false;
                    report.issues.push_back("Duplicate parameter name: " + param.name);
                    std::cout << "    [" << std::setw(2) << i << "] " 
                             << std::setw(20) << std::left << param.name 
                             << " ❌ DUPLICATE NAME\n";
                } else {
                    paramNames.insert(param.name);
                }
                
                // Test parameter update
                testParams.clear();
                testParams[i] = 0.0f;
                engine->updateParameters(testParams);
                
                testParams[i] = 1.0f;
                engine->updateParameters(testParams);
                
                testParams[i] = 0.5f;
                engine->updateParameters(testParams);
                
                // Default value (assuming 0.5 for most)
                param.defaultValue = 0.5f;
                param.minValue = 0.0f;
                param.maxValue = 1.0f;
                
                if (param.isValid) {
                    std::cout << "    [" << std::setw(2) << i << "] " 
                             << std::setw(20) << std::left << param.name;
                    
                    if (i == report.mixParameterIndex) {
                        std::cout << " [MIX]";
                    }
                    
                    std::cout << " ✅\n";
                }
                
                report.parameters.push_back(param);
            }
            
            // Test parameter interaction
            std::cout << "\n  Testing parameter interactions...\n";
            
            // Set all parameters to various values
            testParams.clear();
            for (int i = 0; i < report.numParameters; ++i) {
                testParams[i] = 0.7f;
            }
            engine->updateParameters(testParams);
            
            // Test with audio processing
            engine->prepareToPlay(48000, 512);
            engine->reset();
            
            juce::AudioBuffer<float> buffer(2, 512);
            buffer.clear();
            
            // Process with all parameters at 0
            testParams.clear();
            for (int i = 0; i < report.numParameters; ++i) {
                testParams[i] = 0.0f;
            }
            engine->updateParameters(testParams);
            engine->process(buffer);
            
            // Process with all parameters at 1
            for (int i = 0; i < report.numParameters; ++i) {
                testParams[i] = 1.0f;
            }
            engine->updateParameters(testParams);
            engine->process(buffer);
            
            // Process with random values
            for (int i = 0; i < report.numParameters; ++i) {
                testParams[i] = static_cast<float>(rand()) / RAND_MAX;
            }
            engine->updateParameters(testParams);
            engine->process(buffer);
            
            std::cout << "  ✅ Parameter interaction test passed\n";
            
        } catch (const std::exception& e) {
            report.issues.push_back(std::string("Exception: ") + e.what());
            std::cout << "  ❌ Exception: " << e.what() << "\n";
        }
        
        // Summary for this engine
        if (report.issues.empty()) {
            std::cout << "\n  ✅ All parameter mappings valid\n";
        } else {
            std::cout << "\n  ⚠️ Issues found:\n";
            for (const auto& issue : report.issues) {
                std::cout << "    - " << issue << "\n";
            }
        }
        
        reports.push_back(report);
    }
    
    void runAllTests() {
        std::cout << "\n==========================================\n";
        std::cout << "  PARAMETER MAPPING VERIFICATION TEST\n";
        std::cout << "  Date: " << __DATE__ << " " << __TIME__ << "\n";
        std::cout << "==========================================\n";
        
        // Test all engines
        for (const auto& [id, name] : allEngines) {
            testEngine(id, name);
        }
        
        // Generate comprehensive report
        generateReport();
    }
    
    void generateReport() {
        std::cout << "\n\n==========================================\n";
        std::cout << "         COMPREHENSIVE REPORT\n";
        std::cout << "==========================================\n\n";
        
        // Statistics
        int totalEngines = reports.size();
        int enginesWithIssues = 0;
        int totalParameters = 0;
        int enginesWithoutMix = 0;
        int enginesWithInvalidMix = 0;
        
        std::map<int, std::vector<std::string>> mixIndexGroups;
        
        for (const auto& report : reports) {
            if (!report.issues.empty()) {
                enginesWithIssues++;
            }
            totalParameters += report.numParameters;
            
            if (report.mixParameterIndex < 0) {
                enginesWithoutMix++;
            } else if (!report.mixParameterValid) {
                enginesWithInvalidMix++;
            } else {
                mixIndexGroups[report.mixParameterIndex].push_back(report.engineName);
            }
        }
        
        std::cout << "STATISTICS:\n";
        std::cout << "-----------\n";
        std::cout << "Total Engines: " << totalEngines << "\n";
        std::cout << "Engines with Issues: " << enginesWithIssues << "\n";
        std::cout << "Total Parameters: " << totalParameters << "\n";
        std::cout << "Average Parameters/Engine: " << (totalParameters / totalEngines) << "\n";
        std::cout << "Engines without Mix: " << enginesWithoutMix << "\n";
        std::cout << "Engines with Invalid Mix: " << enginesWithInvalidMix << "\n\n";
        
        // Mix parameter grouping
        std::cout << "MIX PARAMETER INDEX GROUPS:\n";
        std::cout << "---------------------------\n";
        for (const auto& [index, engines] : mixIndexGroups) {
            std::cout << "Index " << index << " (" << engines.size() << " engines):\n";
            for (const auto& engine : engines) {
                std::cout << "  - " << engine << "\n";
            }
            std::cout << "\n";
        }
        
        // Engines without mix (expected)
        std::cout << "ENGINES WITHOUT MIX PARAMETER:\n";
        std::cout << "------------------------------\n";
        for (const auto& report : reports) {
            if (report.mixParameterIndex < 0) {
                std::cout << "  - [" << report.engineID << "] " << report.engineName << "\n";
            }
        }
        std::cout << "\n";
        
        // Issues summary
        if (enginesWithIssues > 0) {
            std::cout << "ENGINES WITH ISSUES:\n";
            std::cout << "--------------------\n";
            for (const auto& report : reports) {
                if (!report.issues.empty()) {
                    std::cout << "[" << report.engineID << "] " << report.engineName << ":\n";
                    for (const auto& issue : report.issues) {
                        std::cout << "  - " << issue << "\n";
                    }
                    std::cout << "\n";
                }
            }
        }
        
        // Parameter count distribution
        std::cout << "PARAMETER COUNT DISTRIBUTION:\n";
        std::cout << "-----------------------------\n";
        std::map<int, int> paramCountDist;
        for (const auto& report : reports) {
            paramCountDist[report.numParameters]++;
        }
        for (const auto& [count, engines] : paramCountDist) {
            std::cout << count << " parameters: " << engines << " engines\n";
        }
        
        // Final verdict
        std::cout << "\n==========================================\n";
        std::cout << "              FINAL VERDICT\n";
        std::cout << "==========================================\n\n";
        
        if (enginesWithIssues == 0 && enginesWithInvalidMix == 0) {
            std::cout << "✅ ALL PARAMETER MAPPINGS ARE VALID!\n";
            std::cout << "All engines have consistent parameter mappings.\n";
        } else {
            std::cout << "⚠️ PARAMETER MAPPING ISSUES DETECTED\n";
            std::cout << enginesWithIssues << " engines have parameter issues.\n";
            std::cout << enginesWithInvalidMix << " engines have invalid mix mappings.\n";
            std::cout << "\nReview the detailed report above for specific issues.\n";
        }
        
        std::cout << "\n==========================================\n";
        std::cout << "         TEST COMPLETE\n";
        std::cout << "==========================================\n\n";
    }
};

int main() {
    ParameterMappingTest tester;
    tester.runAllTests();
    return 0;
}