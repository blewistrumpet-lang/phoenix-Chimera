#include <iostream>
#include <JuceHeader.h>
#include "JUCE_Plugin/Source/EngineFactory.h"
#include "JUCE_Plugin/Source/ParameterControlMap.h"
#include "JUCE_Plugin/Source/ParameterFormatter.h"

void testEngine(int engineId, const juce::String& engineName) {
    std::cout << "\n=== Testing " << engineName.toStdString() << " (ID: " << engineId << ") ===" << std::endl;
    
    // Create engine
    auto engine = EngineFactory::createEngine(engineId);
    if (!engine) {
        std::cout << "  ERROR: Failed to create engine!" << std::endl;
        return;
    }
    
    // Get parameter count
    int numParams = engine->getNumParameters();
    std::cout << "  Parameter count: " << numParams << std::endl;
    
    // Test each parameter
    for (int i = 0; i < numParams; ++i) {
        juce::String paramName = engine->getParameterName(i);
        
        // Get control type
        auto controlType = ParameterControlMap::getControlType(engineId, i);
        juce::String controlTypeStr;
        switch (controlType) {
            case ParameterControlMap::CONTROL_ROTARY: controlTypeStr = "ROTARY"; break;
            case ParameterControlMap::CONTROL_SLIDER: controlTypeStr = "SLIDER"; break;
            case ParameterControlMap::CONTROL_TOGGLE: controlTypeStr = "TOGGLE"; break;
            case ParameterControlMap::CONTROL_STEPPED: controlTypeStr = "STEPPED"; break;
            default: controlTypeStr = "UNKNOWN"; break;
        }
        
        // Test formatting at different values
        juce::String formatted0 = ParameterFormatter::formatValue(engineName, paramName, 0.0f);
        juce::String formatted50 = ParameterFormatter::formatValue(engineName, paramName, 0.5f);
        juce::String formatted100 = ParameterFormatter::formatValue(engineName, paramName, 1.0f);
        
        std::cout << "    Param " << i << ": " << paramName.toStdString() 
                  << " [" << controlTypeStr.toStdString() << "]" << std::endl;
        std::cout << "      Values: 0=" << formatted0.toStdString() 
                  << ", 0.5=" << formatted50.toStdString()
                  << ", 1=" << formatted100.toStdString() << std::endl;
    }
}

int main() {
    juce::ScopedJuceInitialiser_GUI juceInit;
    
    std::cout << "Testing All Engines UI Parameter Mappings" << std::endl;
    std::cout << "==========================================" << std::endl;
    
    // Test each engine category
    std::cout << "\n--- DYNAMICS ENGINES ---" << std::endl;
    testEngine(1, "ClassicCompressor");
    testEngine(2, "VintageOptoCompressor");
    testEngine(3, "MasteringLimiter");
    testEngine(4, "TransientShaper");
    testEngine(5, "NoiseGate");
    
    std::cout << "\n--- DISTORTION ENGINES ---" << std::endl;
    testEngine(6, "WarmTubeDistortion");
    testEngine(7, "VintageTapeSaturation");
    testEngine(8, "AnalogOverdrive");
    testEngine(9, "BitCrusher");
    testEngine(10, "KStyleOverdrive");
    
    std::cout << "\n--- MODULATION ENGINES ---" << std::endl;
    testEngine(11, "ClassicChorus");
    testEngine(12, "VintagePhaser");
    testEngine(13, "ClassicFlanger");
    testEngine(14, "ClassicTremolo");
    testEngine(15, "RotarySpeaker");
    
    std::cout << "\n--- TIME-BASED ENGINES ---" << std::endl;
    testEngine(16, "AnalogDelay");
    testEngine(17, "TapeEcho");
    testEngine(18, "PingPongDelay");
    testEngine(19, "DubDelay");
    testEngine(20, "BucketBrigadeDelay");
    
    std::cout << "\n--- REVERB ENGINES ---" << std::endl;
    testEngine(21, "PlateReverb");
    testEngine(22, "SpringReverb");
    testEngine(23, "ShimmerReverb");
    testEngine(24, "GatedReverb");
    testEngine(25, "ConvolutionReverb");
    
    std::cout << "\n--- FILTER ENGINES ---" << std::endl;
    testEngine(26, "StateVariableFilter");
    testEngine(27, "MoogStyleFilter");
    testEngine(28, "VocalFormantFilter");
    testEngine(29, "AutoWah");
    testEngine(30, "EnvelopeFilter");
    
    std::cout << "\n--- EQ ENGINES ---" << std::endl;
    testEngine(31, "VintageEQ");
    testEngine(32, "GraphicEQ");
    testEngine(33, "ParametricEQ");
    testEngine(34, "TiltShelfEQ");
    testEngine(35, "DynamicEQ");
    
    std::cout << "\n--- UTILITY ENGINES ---" << std::endl;
    testEngine(36, "StereoImager");
    testEngine(37, "MidSideProcessor");
    testEngine(38, "AutoPanner");
    testEngine(39, "Gain");
    testEngine(40, "PhaseAlign");
    
    std::cout << "\n--- SPECIAL ENGINES ---" << std::endl;
    testEngine(41, "Vocoder");
    testEngine(42, "RingModulator");
    testEngine(43, "FrequencyShifter");
    testEngine(44, "PitchShifter");
    testEngine(45, "IntelligentHarmonizer");
    
    // Additional engines
    testEngine(46, "TalkBox");
    testEngine(47, "Exciter");
    testEngine(48, "SubBassEnhancer");
    testEngine(49, "VintageWarmer");
    testEngine(50, "TransientDesigner");
    testEngine(51, "SpectralFilter");
    testEngine(52, "GranularDelay");
    testEngine(53, "CombFilter");
    testEngine(54, "ChaosGenerator");
    testEngine(55, "WaveFolder");
    testEngine(56, "HarmonicExciter");
    
    std::cout << "\n\nAll engines tested!" << std::endl;
    
    return 0;
}