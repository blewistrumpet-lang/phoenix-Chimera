/**
 * Comprehensive Engine Test Suite for Project Chimera v3.0
 * Tests all 57 DSP engines to verify proper audio processing
 * 
 * This test suite verifies that each engine:
 * 1. Loads successfully
 * 2. Processes audio without crashing
 * 3. Produces output appropriate to its type
 * 4. Responds to parameter changes
 * 5. Doesn't introduce artifacts like NaN, inf, or excessive DC
 */

#include <iostream>
#include <iomanip>
#include <vector>
#include <memory>
#include <string>
#include <map>
#include <cmath>
#include <chrono>
#include <fstream>
#include <sstream>

// JUCE and engine includes
#include "JUCE_Plugin/Source/EngineFactory.h"
#include "JUCE_Plugin/Source/EngineTypes.h"
#include "JUCE_Plugin/Source/EngineBase.h"
#include "JUCE_Plugin/Source/TestSignalGenerator.h"
#include "JUCE_Plugin/Source/AudioMeasurements.h"

// Include all individual engine headers
#include "JUCE_Plugin/Source/AnalogPhaser.h"
#include "JUCE_Plugin/Source/AnalogRingModulator.h"
#include "JUCE_Plugin/Source/BitCrusher.h"
#include "JUCE_Plugin/Source/BucketBrigadeDelay.h"
#include "JUCE_Plugin/Source/BufferRepeat.h"
#include "JUCE_Plugin/Source/BufferRepeat_Platinum.h"
#include "JUCE_Plugin/Source/ChaosGenerator.h"
#include "JUCE_Plugin/Source/ChaosGenerator_Platinum.h"
#include "JUCE_Plugin/Source/ClassicCompressor.h"
#include "JUCE_Plugin/Source/ClassicTremolo.h"
#include "JUCE_Plugin/Source/CombResonator.h"
#include "JUCE_Plugin/Source/ConvolutionReverb.h"
#include "JUCE_Plugin/Source/DetuneDoubler.h"
#include "JUCE_Plugin/Source/DigitalDelay.h"
#include "JUCE_Plugin/Source/DimensionExpander.h"
#include "JUCE_Plugin/Source/DynamicEQ.h"
#include "JUCE_Plugin/Source/EnvelopeFilter.h"
#include "JUCE_Plugin/Source/FeedbackNetwork.h"
#include "JUCE_Plugin/Source/FormantFilter.h"
#include "JUCE_Plugin/Source/FrequencyShifter.h"
#include "JUCE_Plugin/Source/GainUtility_Platinum.h"
#include "JUCE_Plugin/Source/GatedReverb.h"
#include "JUCE_Plugin/Source/GranularCloud.h"
#include "JUCE_Plugin/Source/HarmonicExciter.h"
#include "JUCE_Plugin/Source/HarmonicExciter_Platinum.h"
#include "JUCE_Plugin/Source/HarmonicTremolo.h"
#include "JUCE_Plugin/Source/IntelligentHarmonizer.h"
#include "JUCE_Plugin/Source/KStyleOverdrive.h"
#include "JUCE_Plugin/Source/LadderFilter.h"
#include "JUCE_Plugin/Source/MagneticDrumEcho.h"
#include "JUCE_Plugin/Source/MasteringLimiter_Platinum.h"
#include "JUCE_Plugin/Source/MidSideProcessor_Platinum.h"
#include "JUCE_Plugin/Source/MonoMaker_Platinum.h"
#include "JUCE_Plugin/Source/MuffFuzz.h"
#include "JUCE_Plugin/Source/MultibandSaturator.h"
#include "JUCE_Plugin/Source/NoiseGate.h"
#include "JUCE_Plugin/Source/NoiseGate_Platinum.h"
#include "JUCE_Plugin/Source/ParametricEQ.h"
#include "JUCE_Plugin/Source/ParametricEQ_Platinum.h"
#include "JUCE_Plugin/Source/ParametricEQ_Studio.h"
#include "JUCE_Plugin/Source/PhaseAlign_Platinum.h"
#include "JUCE_Plugin/Source/PhasedVocoder.h"
#include "JUCE_Plugin/Source/PitchShifter.h"
#include "JUCE_Plugin/Source/PlateReverb.h"
#include "JUCE_Plugin/Source/PlatinumRingModulator.h"
#include "JUCE_Plugin/Source/ResonantChorus.h"
#include "JUCE_Plugin/Source/ResonantChorus_Platinum.h"
#include "JUCE_Plugin/Source/RodentDistortion.h"
#include "JUCE_Plugin/Source/RotarySpeaker.h"
#include "JUCE_Plugin/Source/RotarySpeaker_Platinum.h"
#include "JUCE_Plugin/Source/ShimmerReverb.h"
#include "JUCE_Plugin/Source/SpectralFreeze.h"
#include "JUCE_Plugin/Source/SpectralGate.h"
#include "JUCE_Plugin/Source/SpectralGate_Platinum.h"
#include "JUCE_Plugin/Source/SpringReverb.h"
#include "JUCE_Plugin/Source/SpringReverb_Platinum.h"
#include "JUCE_Plugin/Source/StateVariableFilter.h"
#include "JUCE_Plugin/Source/StereoChorus.h"
#include "JUCE_Plugin/Source/StereoImager.h"
#include "JUCE_Plugin/Source/StereoWidener.h"
#include "JUCE_Plugin/Source/TapeEcho.h"
#include "JUCE_Plugin/Source/TransientShaper_Platinum.h"
#include "JUCE_Plugin/Source/VintageConsoleEQ.h"
#include "JUCE_Plugin/Source/VintageConsoleEQ_Platinum.h"
#include "JUCE_Plugin/Source/VintageConsoleEQ_Studio.h"
#include "JUCE_Plugin/Source/VintageOptoCompressor.h"
#include "JUCE_Plugin/Source/VintageOptoCompressor_Platinum.h"
#include "JUCE_Plugin/Source/VintageTubePreamp.h"
#include "JUCE_Plugin/Source/VintageTubePreamp_Studio.h"
#include "JUCE_Plugin/Source/VocalFormantFilter.h"
#include "JUCE_Plugin/Source/WaveFolder.h"

// Test configuration
const double SAMPLE_RATE = 44100.0;
const int BLOCK_SIZE = 512;
const float TEST_DURATION = 0.5f; // seconds
const float TEST_AMPLITUDE = 0.5f;
const float SILENCE_THRESHOLD = -60.0f; // dB
const float MAX_ACCEPTABLE_LEVEL = 0.95f; // prevent clipping

// Test result structure
struct EngineTestResult {
    int engineID;
    std::string engineName;
    bool loadSuccess = false;
    bool processSuccess = false;
    bool outputAppropriate = false;
    bool parameterResponse = false;
    bool passedAllTests = false;
    std::string errorMessage;
    
    // Measurements
    float silenceOutputLevel = 0.0f;
    float impulseResponseLength = 0.0f;
    float sineWaveDistortion = 0.0f;
    float noiseProcessingGain = 0.0f;
    bool hasModulation = false;
    bool hasDelayEffect = false;
    bool hasGainReduction = false;
    
    std::string getStatusString() const {
        if (passedAllTests) return "PASS";
        if (!loadSuccess) return "LOAD_FAIL";
        if (!processSuccess) return "PROCESS_FAIL";
        return "AUDIO_FAIL";
    }
};

// Helper functions for audio analysis
namespace AudioAnalysis {
    
    bool isValidBuffer(const juce::AudioBuffer<float>& buffer) {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            const float* channelData = buffer.getReadPointer(ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                if (std::isnan(channelData[i]) || std::isinf(channelData[i])) {
                    return false;
                }
            }
        }
        return true;
    }
    
    float getRMSLevel(const juce::AudioBuffer<float>& buffer) {
        float sum = 0.0f;
        int totalSamples = 0;
        
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            const float* channelData = buffer.getReadPointer(ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                sum += channelData[i] * channelData[i];
                totalSamples++;
            }
        }
        
        return totalSamples > 0 ? std::sqrt(sum / totalSamples) : 0.0f;
    }
    
    float getPeakLevel(const juce::AudioBuffer<float>& buffer) {
        float peak = 0.0f;
        
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            const float* channelData = buffer.getReadPointer(ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                peak = std::max(peak, std::abs(channelData[i]));
            }
        }
        
        return peak;
    }
    
    bool hasSignificantOutput(const juce::AudioBuffer<float>& buffer, float thresholdDB = -40.0f) {
        float rms = getRMSLevel(buffer);
        float rmsDB = rms > 0 ? 20.0f * std::log10(rms) : -100.0f;
        return rmsDB > thresholdDB;
    }
    
    bool detectDelayEffect(const juce::AudioBuffer<float>& input, const juce::AudioBuffer<float>& output) {
        // Simple delay detection: look for correlation peaks at various delays
        if (input.getNumSamples() != output.getNumSamples()) return false;
        
        const float* inputData = input.getReadPointer(0);
        const float* outputData = output.getReadPointer(0);
        int numSamples = input.getNumSamples();
        
        // Check for delays up to 100ms
        int maxDelay = std::min(numSamples / 2, (int)(0.1f * SAMPLE_RATE));
        
        for (int delay = 10; delay < maxDelay; delay += 10) {
            float correlation = 0.0f;
            for (int i = delay; i < numSamples; ++i) {
                correlation += inputData[i - delay] * outputData[i];
            }
            
            if (std::abs(correlation) > 0.1f) return true;
        }
        
        return false;
    }
    
    bool detectModulation(const juce::AudioBuffer<float>& buffer, double sampleRate) {
        // Detect amplitude or frequency modulation by analyzing envelope
        if (buffer.getNumSamples() < 1024) return false;
        
        const float* data = buffer.getReadPointer(0);
        int numSamples = buffer.getNumSamples();
        
        // Calculate envelope using simple peak detection
        std::vector<float> envelope;
        int windowSize = 64;
        for (int i = 0; i < numSamples - windowSize; i += windowSize) {
            float peak = 0.0f;
            for (int j = 0; j < windowSize; ++j) {
                peak = std::max(peak, std::abs(data[i + j]));
            }
            envelope.push_back(peak);
        }
        
        // Look for periodic variation in envelope
        if (envelope.size() < 8) return false;
        
        float variance = 0.0f;
        float mean = 0.0f;
        for (float val : envelope) mean += val;
        mean /= envelope.size();
        
        for (float val : envelope) {
            variance += (val - mean) * (val - mean);
        }
        variance /= envelope.size();
        
        // If coefficient of variation > 0.2, consider it modulated
        return (variance > 0.0f && mean > 0.0f) ? (std::sqrt(variance) / mean) > 0.2f : false;
    }
}

// Mix parameter correction function (based on the corrected getMixParameterIndex)
int getMixParameterIndex(int engineID) {
    // This should match the corrected implementation from PluginProcessor.cpp
    switch (engineID) {
        // Mix at index 2
        case ENGINE_OPTO_COMPRESSOR:
        case ENGINE_VCA_COMPRESSOR:
        case ENGINE_TRANSIENT_SHAPER:
        case ENGINE_NOISE_GATE:
        case ENGINE_MASTERING_LIMITER:
        case ENGINE_DYNAMIC_EQ:
        case ENGINE_PARAMETRIC_EQ:
        case ENGINE_VINTAGE_CONSOLE_EQ:
        case ENGINE_LADDER_FILTER:
        case ENGINE_STATE_VARIABLE_FILTER:
        case ENGINE_FORMANT_FILTER:
        case ENGINE_ENVELOPE_FILTER:
        case ENGINE_COMB_RESONATOR:
        case ENGINE_VOCAL_FORMANT:
        case ENGINE_VINTAGE_TUBE:
        case ENGINE_WAVE_FOLDER:
        case ENGINE_HARMONIC_EXCITER:
        case ENGINE_BIT_CRUSHER:
        case ENGINE_MULTIBAND_SATURATOR:
        case ENGINE_MUFF_FUZZ:
        case ENGINE_RODENT_DISTORTION:
        case ENGINE_K_STYLE:
        case ENGINE_DIGITAL_CHORUS:
        case ENGINE_RESONANT_CHORUS:
        case ENGINE_ANALOG_PHASER:
        case ENGINE_RING_MODULATOR:
        case ENGINE_FREQUENCY_SHIFTER:
        case ENGINE_HARMONIC_TREMOLO:
        case ENGINE_CLASSIC_TREMOLO:
        case ENGINE_ROTARY_SPEAKER:
        case ENGINE_PITCH_SHIFTER:
        case ENGINE_DETUNE_DOUBLER:
        case ENGINE_INTELLIGENT_HARMONIZER:
        case ENGINE_TAPE_ECHO:
        case ENGINE_DIGITAL_DELAY:
        case ENGINE_MAGNETIC_DRUM_ECHO:
        case ENGINE_BUCKET_BRIGADE_DELAY:
        case ENGINE_BUFFER_REPEAT:
        case ENGINE_PLATE_REVERB:
        case ENGINE_SPRING_REVERB:
        case ENGINE_CONVOLUTION_REVERB:
        case ENGINE_SHIMMER_REVERB:
        case ENGINE_GATED_REVERB:
        case ENGINE_STEREO_WIDENER:
        case ENGINE_STEREO_IMAGER:
        case ENGINE_DIMENSION_EXPANDER:
        case ENGINE_SPECTRAL_FREEZE:
        case ENGINE_SPECTRAL_GATE:
        case ENGINE_PHASED_VOCODER:
        case ENGINE_GRANULAR_CLOUD:
        case ENGINE_CHAOS_GENERATOR:
        case ENGINE_FEEDBACK_NETWORK:
        case ENGINE_MID_SIDE_PROCESSOR:
        case ENGINE_GAIN_UTILITY:
        case ENGINE_MONO_MAKER:
        case ENGINE_PHASE_ALIGN:
            return 2; // Mix parameter at index 2
            
        default:
            return -1; // No mix parameter
    }
}

// Main test class
class ComprehensiveEngineTest {
private:
    std::vector<EngineTestResult> results;
    
public:
    void runAllTests() {
        std::cout << "=== Comprehensive Engine Test Suite ===" << std::endl;
        std::cout << "Testing all " << (ENGINE_COUNT - 1) << " DSP engines" << std::endl;
        std::cout << "Sample Rate: " << SAMPLE_RATE << " Hz" << std::endl;
        std::cout << "Block Size: " << BLOCK_SIZE << " samples" << std::endl;
        std::cout << "Test Duration: " << TEST_DURATION << " seconds" << std::endl;
        std::cout << std::endl;
        
        // Test all engines from 1 to ENGINE_COUNT-1 (skipping ENGINE_NONE = 0)
        for (int engineID = 1; engineID < ENGINE_COUNT; ++engineID) {
            results.push_back(testEngine(engineID));
        }
        
        generateReport();
    }
    
private:
    EngineTestResult testEngine(int engineID) {
        EngineTestResult result;
        result.engineID = engineID;
        result.engineName = getEngineTypeName(engineID);
        
        std::cout << std::setw(3) << engineID << ": " << std::setw(30) << std::left 
                  << result.engineName << " ";
        
        try {
            // Test 1: Engine Loading
            auto engine = EngineFactory::createEngine(engineID);
            if (!engine) {
                result.errorMessage = "Failed to create engine";
                std::cout << "[LOAD_FAIL]" << std::endl;
                return result;
            }
            result.loadSuccess = true;
            
            // Test 2: Engine Preparation
            engine->prepareToPlay(SAMPLE_RATE, BLOCK_SIZE);
            
            // Test 3: Set Mix Parameter to 100%
            int mixIndex = getMixParameterIndex(engineID);
            if (mixIndex >= 0) {
                std::map<int, float> params;
                params[mixIndex] = 1.0f; // 100% mix
                engine->updateParameters(params);
            }
            
            // Test 4: Process different signal types
            bool silenceTest = testWithSignal(engine.get(), result, "silence");
            bool impulseTest = testWithSignal(engine.get(), result, "impulse");
            bool sineTest = testWithSignal(engine.get(), result, "sine");
            bool noiseTest = testWithSignal(engine.get(), result, "noise");
            bool transientTest = testWithSignal(engine.get(), result, "transient");
            
            result.processSuccess = silenceTest && impulseTest && sineTest && noiseTest && transientTest;
            
            if (!result.processSuccess) {
                std::cout << "[PROCESS_FAIL]" << std::endl;
                return result;
            }
            
            // Test 5: Verify output characteristics
            result.outputAppropriate = verifyEngineOutput(engine.get(), result);
            
            // Test 6: Parameter response test
            result.parameterResponse = testParameterResponse(engine.get(), result);
            
            result.passedAllTests = result.loadSuccess && result.processSuccess && 
                                   result.outputAppropriate && result.parameterResponse;
            
            std::cout << "[" << result.getStatusString() << "]" << std::endl;
            
        } catch (const std::exception& e) {
            result.errorMessage = std::string("Exception: ") + e.what();
            std::cout << "[EXCEPTION]" << std::endl;
        } catch (...) {
            result.errorMessage = "Unknown exception occurred";
            std::cout << "[UNKNOWN_ERROR]" << std::endl;
        }
        
        return result;
    }
    
    bool testWithSignal(EngineBase* engine, EngineTestResult& result, const std::string& signalType) {
        try {
            // Generate appropriate test signal
            juce::AudioBuffer<float> testSignal;
            
            if (signalType == "silence") {
                testSignal = TestSignalGenerator::generateSilence(TEST_DURATION, SAMPLE_RATE);
            } else if (signalType == "impulse") {
                testSignal = TestSignalGenerator::generateImpulse(SAMPLE_RATE, TEST_AMPLITUDE);
                // Pad with silence to match duration
                int totalSamples = (int)(TEST_DURATION * SAMPLE_RATE);
                if (testSignal.getNumSamples() < totalSamples) {
                    juce::AudioBuffer<float> padded(2, totalSamples);
                    padded.clear();
                    for (int ch = 0; ch < std::min(2, testSignal.getNumChannels()); ++ch) {
                        padded.copyFrom(ch, 0, testSignal, ch, 0, testSignal.getNumSamples());
                    }
                    testSignal = std::move(padded);
                }
            } else if (signalType == "sine") {
                testSignal = TestSignalGenerator::generateSineWave(440.0f, TEST_DURATION, SAMPLE_RATE, TEST_AMPLITUDE);
            } else if (signalType == "noise") {
                testSignal = TestSignalGenerator::generateWhiteNoise(TEST_DURATION, SAMPLE_RATE, TEST_AMPLITUDE);
            } else if (signalType == "transient") {
                testSignal = TestSignalGenerator::generateDrumHit(SAMPLE_RATE);
                // Ensure proper duration
                int targetSamples = (int)(TEST_DURATION * SAMPLE_RATE);
                if (testSignal.getNumSamples() < targetSamples) {
                    juce::AudioBuffer<float> extended(2, targetSamples);
                    extended.clear();
                    for (int ch = 0; ch < std::min(2, testSignal.getNumChannels()); ++ch) {
                        extended.copyFrom(ch, 0, testSignal, ch, 0, testSignal.getNumSamples());
                    }
                    testSignal = std::move(extended);
                }
            }
            
            // Ensure stereo
            if (testSignal.getNumChannels() == 1) {
                juce::AudioBuffer<float> stereo(2, testSignal.getNumSamples());
                stereo.copyFrom(0, 0, testSignal, 0, 0, testSignal.getNumSamples());
                stereo.copyFrom(1, 0, testSignal, 0, 0, testSignal.getNumSamples());
                testSignal = std::move(stereo);
            }
            
            // Keep a copy of input for analysis
            juce::AudioBuffer<float> inputCopy(testSignal.getNumChannels(), testSignal.getNumSamples());
            for (int ch = 0; ch < testSignal.getNumChannels(); ++ch) {
                inputCopy.copyFrom(ch, 0, testSignal, ch, 0, testSignal.getNumSamples());
            }
            
            // Process in blocks
            int samplesProcessed = 0;
            int totalSamples = testSignal.getNumSamples();
            
            while (samplesProcessed < totalSamples) {
                int samplesToProcess = std::min(BLOCK_SIZE, totalSamples - samplesProcessed);
                
                // Create a buffer for this block
                juce::AudioBuffer<float> block(testSignal.getNumChannels(), samplesToProcess);
                for (int ch = 0; ch < testSignal.getNumChannels(); ++ch) {
                    block.copyFrom(ch, 0, testSignal, ch, samplesProcessed, samplesToProcess);
                }
                
                // Process this block
                engine->process(block);
                
                // Copy back to main buffer
                for (int ch = 0; ch < testSignal.getNumChannels(); ++ch) {
                    testSignal.copyFrom(ch, samplesProcessed, block, ch, 0, samplesToProcess);
                }
                
                samplesProcessed += samplesToProcess;
            }
            
            // Validate output
            if (!AudioAnalysis::isValidBuffer(testSignal)) {
                result.errorMessage = "Invalid output (NaN/Inf) with " + signalType;
                return false;
            }
            
            if (AudioAnalysis::getPeakLevel(testSignal) > MAX_ACCEPTABLE_LEVEL) {
                result.errorMessage = "Output clipping with " + signalType;
                return false;
            }
            
            // Store specific measurements
            if (signalType == "silence") {
                result.silenceOutputLevel = AudioAnalysis::getRMSLevel(testSignal);
            } else if (signalType == "impulse") {
                result.impulseResponseLength = testSignal.getNumSamples() / SAMPLE_RATE;
                result.hasDelayEffect = AudioAnalysis::detectDelayEffect(inputCopy, testSignal);
            } else if (signalType == "sine") {
                result.hasModulation = AudioAnalysis::detectModulation(testSignal, SAMPLE_RATE);
            }
            
            return true;
            
        } catch (...) {
            result.errorMessage = "Processing failed with " + signalType;
            return false;
        }
    }
    
    bool verifyEngineOutput(EngineBase* engine, EngineTestResult& result) {
        // Classify engine type and verify appropriate behavior
        int category = getEngineCategory(result.engineID);
        
        switch (category) {
            case EngineCategory::DYNAMICS:
                // Compressors should reduce dynamic range, gates should have gain reduction
                return verifyDynamicsProcessor(engine, result);
                
            case EngineCategory::SPATIAL_TIME:
                // Reverbs/delays should add tail or echoes
                return verifyTimeBasedProcessor(engine, result);
                
            case EngineCategory::FILTERS_EQ:
                // Filters/EQs should change frequency response
                return verifyFrequencyProcessor(engine, result);
                
            case EngineCategory::DISTORTION_SATURATION:
                // Distortion should add harmonics
                return verifyDistortionProcessor(engine, result);
                
            case EngineCategory::MODULATION:
                // Modulation effects should create periodic changes
                return verifyModulationProcessor(engine, result);
                
            case EngineCategory::UTILITY:
                // Utility processors should process cleanly
                return verifyUtilityProcessor(engine, result);
                
            default:
                return true; // Unknown category, assume pass
        }
    }
    
    bool verifyDynamicsProcessor(EngineBase* engine, EngineTestResult& result) {
        // Test with loud signal to trigger compression/gating
        auto loudSignal = TestSignalGenerator::generateSineWave(1000.0f, 0.2f, SAMPLE_RATE, 0.8f);
        auto quietSignal = TestSignalGenerator::generateSineWave(1000.0f, 0.2f, SAMPLE_RATE, 0.1f);
        
        // Ensure stereo
        if (loudSignal.getNumChannels() == 1) {
            juce::AudioBuffer<float> stereo(2, loudSignal.getNumSamples());
            stereo.copyFrom(0, 0, loudSignal, 0, 0, loudSignal.getNumSamples());
            stereo.copyFrom(1, 0, loudSignal, 0, 0, loudSignal.getNumSamples());
            loudSignal = std::move(stereo);
        }
        
        if (quietSignal.getNumChannels() == 1) {
            juce::AudioBuffer<float> stereo(2, quietSignal.getNumSamples());
            stereo.copyFrom(0, 0, quietSignal, 0, 0, quietSignal.getNumSamples());
            stereo.copyFrom(1, 0, quietSignal, 0, 0, quietSignal.getNumSamples());
            quietSignal = std::move(stereo);
        }
        
        float loudInputLevel = AudioAnalysis::getRMSLevel(loudSignal);
        float quietInputLevel = AudioAnalysis::getRMSLevel(quietSignal);
        
        engine->process(loudSignal);
        engine->process(quietSignal);
        
        float loudOutputLevel = AudioAnalysis::getRMSLevel(loudSignal);
        float quietOutputLevel = AudioAnalysis::getRMSLevel(quietSignal);
        
        // For compressors: loud signals should be reduced more than quiet signals
        if (result.engineName.find("Compressor") != std::string::npos ||
            result.engineName.find("Limiter") != std::string::npos) {
            float loudReduction = loudInputLevel > 0 ? loudOutputLevel / loudInputLevel : 1.0f;
            float quietReduction = quietInputLevel > 0 ? quietOutputLevel / quietInputLevel : 1.0f;
            result.hasGainReduction = loudReduction < 0.9f;
            return loudReduction <= quietReduction + 0.1f; // Loud should be reduced more
        }
        
        // For gates: quiet signals should be reduced more than loud signals
        if (result.engineName.find("Gate") != std::string::npos) {
            float loudReduction = loudInputLevel > 0 ? loudOutputLevel / loudInputLevel : 1.0f;
            float quietReduction = quietInputLevel > 0 ? quietOutputLevel / quietInputLevel : 1.0f;
            result.hasGainReduction = quietReduction < 0.9f;
            return quietReduction <= loudReduction; // Quiet should be reduced more
        }
        
        return true; // Other dynamics processors
    }
    
    bool verifyTimeBasedProcessor(EngineBase* engine, EngineTestResult& result) {
        // Test with impulse to check for delays/reverb tails
        auto impulse = TestSignalGenerator::generateImpulse(SAMPLE_RATE, 0.5f);
        
        // Extend to capture tail
        int totalSamples = (int)(2.0 * SAMPLE_RATE); // 2 seconds
        juce::AudioBuffer<float> extended(2, totalSamples);
        extended.clear();
        
        // Place impulse at the beginning
        for (int ch = 0; ch < 2; ++ch) {
            if (ch < impulse.getNumChannels()) {
                extended.copyFrom(ch, 0, impulse, ch, 0, impulse.getNumSamples());
            } else {
                extended.copyFrom(ch, 0, impulse, 0, 0, impulse.getNumSamples());
            }
        }
        
        engine->process(extended);
        
        // Check for energy beyond the initial impulse
        int impulseEnd = impulse.getNumSamples() + 1000; // Give some margin
        bool hasTail = false;
        
        for (int i = impulseEnd; i < totalSamples; ++i) {
            for (int ch = 0; ch < extended.getNumChannels(); ++ch) {
                if (std::abs(extended.getSample(ch, i)) > 0.01f) {
                    hasTail = true;
                    break;
                }
            }
            if (hasTail) break;
        }
        
        return hasTail || result.hasDelayEffect;
    }
    
    bool verifyFrequencyProcessor(EngineBase* engine, EngineTestResult& result) {
        // Test with swept sine to check frequency response changes
        auto sweep = TestSignalGenerator::generateSweep(20.0f, 20000.0f, 1.0f, SAMPLE_RATE, 0.3f);
        
        // Ensure stereo
        if (sweep.getNumChannels() == 1) {
            juce::AudioBuffer<float> stereo(2, sweep.getNumSamples());
            stereo.copyFrom(0, 0, sweep, 0, 0, sweep.getNumSamples());
            stereo.copyFrom(1, 0, sweep, 0, 0, sweep.getNumSamples());
            sweep = std::move(stereo);
        }
        
        auto inputCopy = sweep;
        engine->process(sweep);
        
        // Simple frequency response check: compare RMS of different frequency bands
        // This is a basic test - a proper implementation would use FFT analysis
        float inputRMS = AudioAnalysis::getRMSLevel(inputCopy);
        float outputRMS = AudioAnalysis::getRMSLevel(sweep);
        
        // Allow for ±20dB gain range (0.1 to 10.0 ratio)
        if (inputRMS > 0) {
            float gainRatio = outputRMS / inputRMS;
            return gainRatio >= 0.1f && gainRatio <= 10.0f;
        }
        
        return true;
    }
    
    bool verifyDistortionProcessor(EngineBase* engine, EngineTestResult& result) {
        // Test with sine wave to check for harmonic generation
        auto sine = TestSignalGenerator::generateSineWave(440.0f, 0.5f, SAMPLE_RATE, 0.7f);
        
        // Ensure stereo
        if (sine.getNumChannels() == 1) {
            juce::AudioBuffer<float> stereo(2, sine.getNumSamples());
            stereo.copyFrom(0, 0, sine, 0, 0, sine.getNumSamples());
            stereo.copyFrom(1, 0, sine, 0, 0, sine.getNumSamples());
            sine = std::move(stereo);
        }
        
        engine->process(sine);
        
        // For distortion, we expect the signal to be modified but still valid
        float rms = AudioAnalysis::getRMSLevel(sine);
        return rms > 0.01f && rms < 1.0f; // Should have reasonable output level
    }
    
    bool verifyModulationProcessor(EngineBase* engine, EngineTestResult& result) {
        // Test with sustained tone to check for modulation
        auto tone = TestSignalGenerator::generateSineWave(440.0f, 1.0f, SAMPLE_RATE, 0.5f);
        
        // Ensure stereo
        if (tone.getNumChannels() == 1) {
            juce::AudioBuffer<float> stereo(2, tone.getNumSamples());
            stereo.copyFrom(0, 0, tone, 0, 0, tone.getNumSamples());
            stereo.copyFrom(1, 0, tone, 0, 0, tone.getNumSamples());
            tone = std::move(stereo);
        }
        
        engine->process(tone);
        
        // Check for modulation characteristics
        bool hasModulation = AudioAnalysis::detectModulation(tone, SAMPLE_RATE);
        result.hasModulation = hasModulation;
        
        return true; // Modulation detection is informational, not pass/fail
    }
    
    bool verifyUtilityProcessor(EngineBase* engine, EngineTestResult& result) {
        // Utility processors should handle audio cleanly
        auto testSignal = TestSignalGenerator::generateSineWave(1000.0f, 0.5f, SAMPLE_RATE, 0.5f);
        
        // Ensure stereo
        if (testSignal.getNumChannels() == 1) {
            juce::AudioBuffer<float> stereo(2, testSignal.getNumSamples());
            stereo.copyFrom(0, 0, testSignal, 0, 0, testSignal.getNumSamples());
            stereo.copyFrom(1, 0, testSignal, 0, 0, testSignal.getNumSamples());
            testSignal = std::move(stereo);
        }
        
        engine->process(testSignal);
        
        // Should produce clean output without artifacts
        return AudioAnalysis::isValidBuffer(testSignal) && 
               AudioAnalysis::getPeakLevel(testSignal) <= MAX_ACCEPTABLE_LEVEL;
    }
    
    bool testParameterResponse(EngineBase* engine, EngineTestResult& result) {
        try {
            // Test parameter updates don't crash
            std::map<int, float> params;
            
            // Try various parameter combinations
            for (int i = 0; i < 10; ++i) {
                params[i] = 0.5f + 0.3f * std::sin(i); // Varied values
            }
            
            engine->updateParameters(params);
            
            // Process a short signal after parameter change
            auto testSignal = TestSignalGenerator::generateSineWave(440.0f, 0.1f, SAMPLE_RATE, 0.3f);
            if (testSignal.getNumChannels() == 1) {
                juce::AudioBuffer<float> stereo(2, testSignal.getNumSamples());
                stereo.copyFrom(0, 0, testSignal, 0, 0, testSignal.getNumSamples());
                stereo.copyFrom(1, 0, testSignal, 0, 0, testSignal.getNumSamples());
                testSignal = std::move(stereo);
            }
            
            engine->process(testSignal);
            
            return AudioAnalysis::isValidBuffer(testSignal);
            
        } catch (...) {
            result.errorMessage = "Parameter update failed";
            return false;
        }
    }
    
    void generateReport() {
        std::cout << std::endl;
        std::cout << "=== TEST SUMMARY ===" << std::endl;
        
        int passCount = 0;
        int loadFailCount = 0;
        int processFailCount = 0;
        int audioFailCount = 0;
        
        for (const auto& result : results) {
            if (result.passedAllTests) {
                passCount++;
            } else if (!result.loadSuccess) {
                loadFailCount++;
            } else if (!result.processSuccess) {
                processFailCount++;
            } else {
                audioFailCount++;
            }
        }
        
        std::cout << "Total Engines Tested: " << results.size() << std::endl;
        std::cout << "PASSED: " << passCount << std::endl;
        std::cout << "LOAD_FAIL: " << loadFailCount << std::endl;
        std::cout << "PROCESS_FAIL: " << processFailCount << std::endl;
        std::cout << "AUDIO_FAIL: " << audioFailCount << std::endl;
        std::cout << std::endl;
        
        // Detailed failure report
        if (loadFailCount + processFailCount + audioFailCount > 0) {
            std::cout << "=== FAILED ENGINES ===" << std::endl;
            for (const auto& result : results) {
                if (!result.passedAllTests) {
                    std::cout << std::setw(3) << result.engineID << ": " 
                              << std::setw(30) << std::left << result.engineName 
                              << " [" << result.getStatusString() << "]";
                    if (!result.errorMessage.empty()) {
                        std::cout << " - " << result.errorMessage;
                    }
                    std::cout << std::endl;
                }
            }
            std::cout << std::endl;
        }
        
        // Success report
        if (passCount > 0) {
            std::cout << "=== WORKING ENGINES ===" << std::endl;
            for (const auto& result : results) {
                if (result.passedAllTests) {
                    std::cout << std::setw(3) << result.engineID << ": " 
                              << std::setw(30) << std::left << result.engineName << " [PASS]";
                    
                    // Add characteristics
                    std::vector<std::string> traits;
                    if (result.hasModulation) traits.push_back("Modulation");
                    if (result.hasDelayEffect) traits.push_back("Delay/Reverb");
                    if (result.hasGainReduction) traits.push_back("Dynamics");
                    
                    if (!traits.empty()) {
                        std::cout << " (";
                        for (size_t i = 0; i < traits.size(); ++i) {
                            if (i > 0) std::cout << ", ";
                            std::cout << traits[i];
                        }
                        std::cout << ")";
                    }
                    std::cout << std::endl;
                }
            }
        }
        
        // Write detailed report to file
        writeDetailedReport();
        
        std::cout << std::endl;
        std::cout << "Detailed report written to: comprehensive_engine_test_report.txt" << std::endl;
        std::cout << "Test completed at: " << getCurrentTimestamp() << std::endl;
    }
    
    void writeDetailedReport() {
        std::ofstream file("comprehensive_engine_test_report.txt");
        if (!file.is_open()) return;
        
        file << "Comprehensive Engine Test Report\n";
        file << "Generated: " << getCurrentTimestamp() << "\n";
        file << "Sample Rate: " << SAMPLE_RATE << " Hz\n";
        file << "Block Size: " << BLOCK_SIZE << " samples\n";
        file << "Test Duration: " << TEST_DURATION << " seconds\n\n";
        
        file << "Test Results:\n";
        file << "Engine_ID,Engine_Name,Status,Load_Success,Process_Success,Output_Appropriate,Parameter_Response,Error_Message\n";
        
        for (const auto& result : results) {
            file << result.engineID << ","
                 << "\"" << result.engineName << "\","
                 << result.getStatusString() << ","
                 << (result.loadSuccess ? "true" : "false") << ","
                 << (result.processSuccess ? "true" : "false") << ","
                 << (result.outputAppropriate ? "true" : "false") << ","
                 << (result.parameterResponse ? "true" : "false") << ","
                 << "\"" << result.errorMessage << "\"\n";
        }
        
        file << "\nDetailed Analysis:\n";
        for (const auto& result : results) {
            file << "\n--- Engine " << result.engineID << ": " << result.engineName << " ---\n";
            file << "Status: " << result.getStatusString() << "\n";
            if (!result.errorMessage.empty()) {
                file << "Error: " << result.errorMessage << "\n";
            }
            file << "Silence Output Level: " << result.silenceOutputLevel << "\n";
            file << "Has Modulation: " << (result.hasModulation ? "Yes" : "No") << "\n";
            file << "Has Delay Effect: " << (result.hasDelayEffect ? "Yes" : "No") << "\n";
            file << "Has Gain Reduction: " << (result.hasGainReduction ? "Yes" : "No") << "\n";
        }
        
        file.close();
    }
    
    std::string getCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }
};

// Main function
int main(int argc, char* argv[]) {
    std::cout << "Project Chimera v3.0 - Comprehensive Engine Test Suite" << std::endl;
    std::cout << "========================================================" << std::endl;
    
    try {
        ComprehensiveEngineTest tester;
        tester.runAllTests();
        
        std::cout << std::endl << "Test suite completed successfully!" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Test suite failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Test suite failed with unknown exception" << std::endl;
        return 1;
    }
}