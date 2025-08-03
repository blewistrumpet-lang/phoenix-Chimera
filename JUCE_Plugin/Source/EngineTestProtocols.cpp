#include "EngineTestProtocols.h"
#include "EngineTypes.h"
#include <chrono>

EngineTestProtocols::EngineTestReport EngineTestProtocols::testDynamicsEngine(
    EngineBase* engine, int engineID) {
    
    EngineTestReport report;
    report.engineName = engine->getName();
    report.engineID = engineID;
    report.overallPass = true;
    
    // Test 1: Threshold behavior with varying input levels
    {
        auto testSignal = TestSignalGenerator::generateSineWave(1000.0f, TEST_DURATION, SAMPLE_RATE);
        
        for (float dB = -60.0f; dB <= 0.0f; dB += 10.0f) {
            float amplitude = TestSignalGenerator::dBToLinear(dB);
            auto scaled = testSignal;
            TestSignalGenerator::scaleSignal(scaled, amplitude);
            
            auto output = processEngine(engine, scaled);
            float gainReduction = AudioMeasurements::measureGainReduction(scaled, output);
            
            // Compressor should reduce gain above threshold
            bool expectedBehavior = (dB > -20.0f) ? (gainReduction < 0.0f) : (gainReduction >= -1.0f);
            
            report.addResult(
                juce::String::formatted("Gain at %.0fdB input", dB),
                expectedBehavior,
                gainReduction,
                -20.0f, 0.0f,
                juce::String::formatted("Gain reduction: %.2fdB", gainReduction)
            );
        }
    }
    
    // Test 2: Attack/Release timing
    {
        auto burstSignal = TestSignalGenerator::generateBurst(0.1f, 0.1f, 1.0f, SAMPLE_RATE);
        auto output = processEngine(engine, burstSignal);
        
        auto timing = AudioMeasurements::measureEnvelopeTiming(output, SAMPLE_RATE);
        
        report.addResult(
            "Attack Time",
            timing.first < 100.0f, // Should be less than 100ms
            timing.first,
            0.1f, 100.0f,
            juce::String::formatted("%.2fms", timing.first)
        );
        
        report.addResult(
            "Release Time",
            timing.second < 500.0f, // Should be less than 500ms
            timing.second,
            1.0f, 500.0f,
            juce::String::formatted("%.2fms", timing.second)
        );
    }
    
    // Test 3: Distortion at high levels
    {
        auto testSignal = TestSignalGenerator::generateSineWave(440.0f, TEST_DURATION, SAMPLE_RATE, 0.9f);
        auto output = processEngine(engine, testSignal);
        
        float thd = AudioMeasurements::measureTHD(output, 440.0f, SAMPLE_RATE);
        
        report.addResult(
            "THD at high input",
            thd < 5.0f, // Less than 5% THD
            thd,
            0.0f, 5.0f,
            juce::String::formatted("%.2f%%", thd)
        );
    }
    
    return report;
}

EngineTestProtocols::EngineTestReport EngineTestProtocols::testFilterEngine(
    EngineBase* engine, int engineID) {
    
    EngineTestReport report;
    report.engineName = engine->getName();
    report.engineID = engineID;
    report.overallPass = true;
    
    // Test 1: Frequency response
    {
        auto sweep = TestSignalGenerator::generateSweep(20.0f, 20000.0f, 2.0f, SAMPLE_RATE);
        auto output = processEngine(engine, sweep);
        
        auto response = AudioMeasurements::computeFrequencyResponse(output, SAMPLE_RATE);
        
        // Check for reasonable frequency response
        float maxMag = *std::max_element(response.magnitudes.begin(), response.magnitudes.end());
        float minMag = *std::min_element(response.magnitudes.begin(), response.magnitudes.end());
        float dynamicRange = 20.0f * std::log10(maxMag / (minMag + 0.0001f));
        
        report.addResult(
            "Frequency Response Range",
            dynamicRange > 6.0f, // At least 6dB of filtering
            dynamicRange,
            6.0f, 60.0f,
            juce::String::formatted("%.1fdB range", dynamicRange)
        );
    }
    
    // Test 2: Self-oscillation check
    {
        // Set high resonance if parameter exists
        std::map<int, float> params;
        for (int i = 0; i < engine->getNumParameters(); ++i) {
            juce::String paramName = engine->getParameterName(i).toLowerCase();
            if (paramName.contains("resonance") || paramName.contains("q")) {
                params[i] = 0.95f; // High resonance
            }
        }
        engine->updateParameters(params);
        
        auto impulse = TestSignalGenerator::generateImpulse(SAMPLE_RATE);
        auto output = processEngine(engine, impulse);
        
        bool selfOscillates = AudioMeasurements::detectSustainedOscillation(output, SAMPLE_RATE);
        
        report.addResult(
            "Self-oscillation Test",
            !selfOscillates, // Should NOT self-oscillate
            selfOscillates ? 1.0f : 0.0f,
            0.0f, 0.0f,
            selfOscillates ? "OSCILLATING!" : "Stable"
        );
    }
    
    // Test 3: Phase response
    {
        auto sine = TestSignalGenerator::generateSineWave(1000.0f, TEST_DURATION, SAMPLE_RATE);
        auto output = processEngine(engine, sine);
        
        float latency = AudioMeasurements::measureLatency(sine, output, SAMPLE_RATE);
        
        report.addResult(
            "Filter Latency",
            latency < 10.0f, // Less than 10ms
            latency,
            0.0f, 10.0f,
            juce::String::formatted("%.2fms", latency)
        );
    }
    
    return report;
}

EngineTestProtocols::EngineTestReport EngineTestProtocols::testTimeBasedEngine(
    EngineBase* engine, int engineID) {
    
    EngineTestReport report;
    report.engineName = engine->getName();
    report.engineID = engineID;
    report.overallPass = true;
    
    // Test 1: Impulse response
    {
        auto impulse = TestSignalGenerator::generateImpulse(SAMPLE_RATE);
        auto output = processEngine(engine, impulse);
        
        // For delays, measure delay time
        float delayTime = AudioMeasurements::measureDelayTime(impulse, output, SAMPLE_RATE);
        
        report.addResult(
            "Delay Time",
            delayTime > 0.0f && delayTime < 2000.0f, // Between 0 and 2 seconds
            delayTime,
            0.0f, 2000.0f,
            juce::String::formatted("%.1fms", delayTime)
        );
        
        // For reverbs, measure RT60
        float rt60 = AudioMeasurements::measureRT60(output, SAMPLE_RATE);
        
        if (rt60 > 0.0f) {
            report.addResult(
                "RT60",
                rt60 > 0.1f && rt60 < 10.0f, // Reasonable reverb time
                rt60,
                0.1f, 10.0f,
                juce::String::formatted("%.2fs", rt60)
            );
        }
    }
    
    // Test 2: Frequency response of wet signal
    {
        auto whiteNoise = TestSignalGenerator::generateWhiteNoise(TEST_DURATION, SAMPLE_RATE);
        auto output = processEngine(engine, whiteNoise);
        
        auto response = AudioMeasurements::computeFrequencyResponse(output, SAMPLE_RATE);
        
        // Check if high frequencies are attenuated (typical of reverbs/delays)
        float lowFreqAvg = 0.0f, highFreqAvg = 0.0f;
        int midPoint = response.frequencies.size() / 2;
        
        for (int i = 0; i < midPoint; ++i) {
            lowFreqAvg += response.magnitudes[i];
        }
        for (int i = midPoint; i < response.frequencies.size(); ++i) {
            highFreqAvg += response.magnitudes[i];
        }
        
        lowFreqAvg /= midPoint;
        highFreqAvg /= (response.frequencies.size() - midPoint);
        
        float hfDamping = 20.0f * std::log10(highFreqAvg / (lowFreqAvg + 0.0001f));
        
        report.addResult(
            "HF Damping",
            true, // Just reporting
            hfDamping,
            -20.0f, 0.0f,
            juce::String::formatted("%.1fdB", hfDamping)
        );
    }
    
    // Test 3: Feedback stability
    {
        auto sustained = TestSignalGenerator::generateSineWave(500.0f, 2.0f, SAMPLE_RATE);
        auto output = processEngine(engine, sustained);
        
        float outputPeak = AudioMeasurements::measurePeak(output);
        
        report.addResult(
            "Feedback Stability",
            outputPeak < 1.0f, // Should not clip
            outputPeak,
            0.0f, 1.0f,
            outputPeak > 0.95f ? "Near clipping!" : "Stable"
        );
    }
    
    return report;
}

EngineTestProtocols::EngineTestReport EngineTestProtocols::testModulationEngine(
    EngineBase* engine, int engineID) {
    
    EngineTestReport report;
    report.engineName = engine->getName();
    report.engineID = engineID;
    report.overallPass = true;
    
    // Test 1: Modulation detection
    {
        auto sine = TestSignalGenerator::generateSineWave(1000.0f, 2.0f, SAMPLE_RATE);
        auto output = processEngine(engine, sine);
        
        auto modProfile = AudioMeasurements::extractModulationProfile(output, SAMPLE_RATE);
        
        report.addResult(
            "Modulation Rate",
            modProfile.rate > 0.1f && modProfile.rate < 20.0f, // Typical LFO range
            modProfile.rate,
            0.1f, 20.0f,
            juce::String::formatted("%.2fHz", modProfile.rate)
        );
        
        report.addResult(
            "Modulation Depth",
            modProfile.depth > 0.0f && modProfile.depth <= 1.0f,
            modProfile.depth * 100.0f,
            0.0f, 100.0f,
            juce::String::formatted("%.1f%%", modProfile.depth * 100.0f)
        );
    }
    
    // Test 2: Stereo width (for chorus/ensemble effects)
    {
        auto mono = TestSignalGenerator::generateSineWave(440.0f, TEST_DURATION, SAMPLE_RATE);
        auto output = processEngine(engine, mono);
        
        // Check stereo correlation
        const float* left = output.getReadPointer(0);
        const float* right = output.getNumChannels() > 1 ? output.getReadPointer(1) : left;
        
        float correlation = 0.0f;
        for (int i = 0; i < output.getNumSamples(); ++i) {
            correlation += left[i] * right[i];
        }
        correlation /= output.getNumSamples();
        
        report.addResult(
            "Stereo Width",
            true, // Just reporting
            1.0f - std::abs(correlation),
            0.0f, 1.0f,
            juce::String::formatted("%.2f", 1.0f - std::abs(correlation))
        );
    }
    
    // Test 3: Harmonic content preservation
    {
        auto chord = TestSignalGenerator::generateChord(220.0f, TEST_DURATION, SAMPLE_RATE);
        auto output = processEngine(engine, chord);
        
        float thd = AudioMeasurements::measureTHD(output, 220.0f, SAMPLE_RATE);
        
        report.addResult(
            "Harmonic Preservation",
            thd < 10.0f, // Should not add too much distortion
            thd,
            0.0f, 10.0f,
            juce::String::formatted("%.2f%% THD", thd)
        );
    }
    
    return report;
}

EngineTestProtocols::EngineTestReport EngineTestProtocols::testDistortionEngine(
    EngineBase* engine, int engineID) {
    
    EngineTestReport report;
    report.engineName = engine->getName();
    report.engineID = engineID;
    report.overallPass = true;
    
    // Test 1: Harmonic generation at different drive levels
    {
        auto sine = TestSignalGenerator::generateSineWave(440.0f, TEST_DURATION, SAMPLE_RATE);
        
        std::vector<float> driveSettings = {0.0f, 0.25f, 0.5f, 0.75f, 1.0f};
        
        for (float drive : driveSettings) {
            // Set drive parameter if it exists
            std::map<int, float> params;
            for (int i = 0; i < engine->getNumParameters(); ++i) {
                juce::String paramName = engine->getParameterName(i).toLowerCase();
                if (paramName.contains("drive") || paramName.contains("gain") || 
                    paramName.contains("distortion")) {
                    params[i] = drive;
                }
            }
            engine->updateParameters(params);
            
            auto output = processEngine(engine, sine);
            auto harmonics = AudioMeasurements::measureHarmonicContent(output, 440.0f, SAMPLE_RATE);
            
            report.addResult(
                juce::String::formatted("THD at %.0f%% drive", drive * 100),
                harmonics.thd >= drive * 5.0f, // THD should increase with drive
                harmonics.thd,
                0.0f, 100.0f,
                juce::String::formatted("%.2f%%", harmonics.thd)
            );
        }
    }
    
    // Test 2: Intermodulation distortion
    {
        auto twoTone = TestSignalGenerator::generateTwoTone(440.0f, 550.0f, TEST_DURATION, SAMPLE_RATE);
        auto output = processEngine(engine, twoTone);
        
        float imd = AudioMeasurements::measureIMD(output, 440.0f, 550.0f, SAMPLE_RATE);
        
        report.addResult(
            "IMD",
            imd < 50.0f, // Less than 50% IMD
            imd,
            0.0f, 50.0f,
            juce::String::formatted("%.2f%%", imd)
        );
    }
    
    // Test 3: Dynamic response
    {
        auto drum = TestSignalGenerator::generateDrumHit(SAMPLE_RATE);
        auto output = processEngine(engine, drum);
        
        float inputPeak = AudioMeasurements::measurePeak(drum);
        float outputPeak = AudioMeasurements::measurePeak(output);
        float compression = outputPeak / inputPeak;
        
        report.addResult(
            "Dynamic Compression",
            compression > 0.5f && compression <= 1.2f, // Some compression but not excessive
            compression,
            0.5f, 1.2f,
            juce::String::formatted("%.2fx", compression)
        );
    }
    
    return report;
}

EngineTestProtocols::EngineTestReport EngineTestProtocols::runBasicTests(
    EngineBase* engine, int engineID) {
    
    EngineTestReport report;
    report.engineName = engine->getName();
    report.engineID = engineID;
    report.overallPass = true;
    
    // Test 1: Silence in, silence out
    bool silenceTest = testSilenceInSilenceOut(engine);
    report.addResult(
        "Silence Test",
        silenceTest,
        silenceTest ? 0.0f : 1.0f,
        0.0f, 0.0f,
        silenceTest ? "Passed" : "Failed - generates noise"
    );
    
    // Test 2: Unity gain with default parameters
    bool unityTest = testUnityGain(engine);
    report.addResult(
        "Unity Gain Test",
        unityTest,
        unityTest ? 0.0f : 1.0f,
        0.0f, 0.0f,
        unityTest ? "Passed" : "Failed - gain mismatch"
    );
    
    // Test 3: Basic frequency response
    bool freqTest = testFrequencyResponse(engine);
    report.addResult(
        "Frequency Response",
        freqTest,
        freqTest ? 0.0f : 1.0f,
        0.0f, 0.0f,
        freqTest ? "Normal" : "Abnormal response"
    );
    
    // Test 4: Dynamic range
    bool dynamicTest = testDynamicRange(engine);
    report.addResult(
        "Dynamic Range",
        dynamicTest,
        dynamicTest ? 0.0f : 1.0f,
        0.0f, 0.0f,
        dynamicTest ? "Good" : "Clipping detected"
    );
    
    // Test 5: CPU usage
    float cpuUsage = measureCPUUsage(engine);
    report.cpuUsage = cpuUsage;
    report.addResult(
        "CPU Usage",
        cpuUsage < MAX_CPU_PERCENT,
        cpuUsage,
        0.0f, MAX_CPU_PERCENT,
        juce::String::formatted("%.2f%%", cpuUsage)
    );
    
    return report;
}

EngineTestProtocols::EngineTestReport EngineTestProtocols::runComprehensiveTest(
    EngineBase* engine, int engineID) {
    
    // First run basic tests
    auto report = runBasicTests(engine, engineID);
    
    // Then run category-specific tests
    EngineCategory category = detectEngineCategory(engineID);
    EngineTestReport categoryReport;
    
    switch (category) {
        case EngineCategory::Dynamics:
            categoryReport = testDynamicsEngine(engine, engineID);
            break;
        case EngineCategory::Filter:
            categoryReport = testFilterEngine(engine, engineID);
            break;
        case EngineCategory::TimeBased:
            categoryReport = testTimeBasedEngine(engine, engineID);
            break;
        case EngineCategory::Modulation:
            categoryReport = testModulationEngine(engine, engineID);
            break;
        case EngineCategory::Distortion:
            categoryReport = testDistortionEngine(engine, engineID);
            break;
        default:
            // No additional tests for "Other" category
            break;
    }
    
    // Merge category-specific results
    for (const auto& result : categoryReport.results) {
        report.results.push_back(result);
        if (!result.passed) {
            report.overallPass = false;
        }
    }
    
    return report;
}

// Helper function implementations

juce::AudioBuffer<float> EngineTestProtocols::processEngine(
    EngineBase* engine, const juce::AudioBuffer<float>& input) {
    
    // Make a copy of the input
    juce::AudioBuffer<float> output(input);
    
    // Process in blocks
    int totalSamples = output.getNumSamples();
    int position = 0;
    
    while (position < totalSamples) {
        int samplesThisBlock = std::min(BLOCK_SIZE, totalSamples - position);
        
        // Create a sub-buffer
        juce::AudioBuffer<float> block(output.getArrayOfWritePointers(), 
                                       output.getNumChannels(),
                                       position, samplesThisBlock);
        
        // Process the block
        engine->process(block);
        
        position += samplesThisBlock;
    }
    
    return output;
}

bool EngineTestProtocols::testSilenceInSilenceOut(EngineBase* engine) {
    auto silence = TestSignalGenerator::generateSilence(TEST_DURATION, SAMPLE_RATE);
    auto output = processEngine(engine, silence);
    
    float noiseFloor = AudioMeasurements::measureNoiseFloor(output);
    return noiseFloor < SILENCE_THRESHOLD;
}

bool EngineTestProtocols::testUnityGain(EngineBase* engine) {
    // Reset to default parameters
    std::map<int, float> defaultParams;
    engine->updateParameters(defaultParams);
    
    auto testSignal = TestSignalGenerator::generateSineWave(1000.0f, TEST_DURATION, SAMPLE_RATE);
    auto output = processEngine(engine, testSignal);
    
    float inputRMS = AudioMeasurements::measureRMS(testSignal);
    float outputRMS = AudioMeasurements::measureRMS(output);
    
    float gainDiff = std::abs(20.0f * std::log10(outputRMS / inputRMS));
    return gainDiff < 3.0f; // Within 3dB
}

bool EngineTestProtocols::testFrequencyResponse(EngineBase* engine) {
    auto sweep = TestSignalGenerator::generateSweep(20.0f, 20000.0f, 2.0f, SAMPLE_RATE);
    auto output = processEngine(engine, sweep);
    
    auto response = AudioMeasurements::computeFrequencyResponse(output, SAMPLE_RATE);
    
    // Check for reasonable response (not completely silent or extremely loud)
    float avgMag = 0.0f;
    for (float mag : response.magnitudes) {
        avgMag += mag;
    }
    avgMag /= response.magnitudes.size();
    
    return avgMag > 0.001f && avgMag < 100.0f;
}

bool EngineTestProtocols::testDynamicRange(EngineBase* engine) {
    auto loud = TestSignalGenerator::generateSineWave(1000.0f, TEST_DURATION, SAMPLE_RATE, 0.9f);
    auto output = processEngine(engine, loud);
    
    float peak = AudioMeasurements::measurePeak(output);
    return peak <= 1.0f; // No clipping
}

float EngineTestProtocols::measureCPUUsage(EngineBase* engine) {
    auto testSignal = TestSignalGenerator::generateWhiteNoise(1.0f, SAMPLE_RATE);
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Process 1 second of audio
    for (int i = 0; i < 100; ++i) {
        auto block = testSignal;
        engine->process(block);
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    
    // Calculate CPU usage percentage
    float processingTime = duration.count() / 1000000.0f; // Convert to seconds
    float realTime = 1.0f; // We processed 1 second of audio
    
    return (processingTime / realTime) * 100.0f;
}

EngineTestProtocols::EngineCategory EngineTestProtocols::detectEngineCategory(int engineID) {
    // Based on engine ID ranges or specific IDs
    switch (engineID) {
        case ENGINE_VCA_COMPRESSOR:
        case ENGINE_OPTO_COMPRESSOR:
        case ENGINE_MASTERING_LIMITER:
        case ENGINE_NOISE_GATE:
            return EngineCategory::Dynamics;
            
        case ENGINE_LADDER_FILTER:
        case ENGINE_STATE_VARIABLE_FILTER:
        case ENGINE_FORMANT_FILTER:
        case ENGINE_ENVELOPE_FILTER:
        case ENGINE_PARAMETRIC_EQ:
        case ENGINE_VINTAGE_CONSOLE_EQ:
        case ENGINE_DYNAMIC_EQ:
            return EngineCategory::Filter;
            
        case ENGINE_TAPE_ECHO:
        case ENGINE_DIGITAL_DELAY:
        case ENGINE_BUCKET_BRIGADE_DELAY:
        case ENGINE_MAGNETIC_DRUM_ECHO:
        case ENGINE_PLATE_REVERB:
        case ENGINE_CONVOLUTION_REVERB:
        case ENGINE_SHIMMER_REVERB:
        case ENGINE_GATED_REVERB:
        case ENGINE_SPRING_REVERB:
        case ENGINE_FEEDBACK_NETWORK:
            return EngineCategory::TimeBased;
            
        case ENGINE_DIGITAL_CHORUS:
        case ENGINE_ANALOG_PHASER:
        case ENGINE_CLASSIC_TREMOLO:
        case ENGINE_HARMONIC_TREMOLO:
        case ENGINE_ROTARY_SPEAKER:
        case ENGINE_RESONANT_CHORUS:
        case ENGINE_DETUNE_DOUBLER:
            return EngineCategory::Modulation;
            
        case ENGINE_K_STYLE:
        case ENGINE_RODENT_DISTORTION:
        case ENGINE_MUFF_FUZZ:
        case ENGINE_VINTAGE_TUBE:
        case ENGINE_MULTIBAND_SATURATOR:
        case ENGINE_WAVE_FOLDER:
        case ENGINE_BIT_CRUSHER:
            return EngineCategory::Distortion;
            
        default:
            return EngineCategory::Other;
    }
}