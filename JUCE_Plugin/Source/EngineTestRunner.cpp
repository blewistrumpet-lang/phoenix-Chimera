#include "EngineTestRunner.h"
#include "EngineTypes.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>

EngineTestRunner::TestSummary EngineTestRunner::runAllTests() {
    TestSummary summary;
    
    // Test all known engine IDs
    std::vector<int> engineIDs = {
        ENGINE_K_STYLE, ENGINE_TAPE_ECHO, ENGINE_PLATE_REVERB, ENGINE_RODENT_DISTORTION,
        ENGINE_MUFF_FUZZ, ENGINE_CLASSIC_TREMOLO, ENGINE_MAGNETIC_DRUM_ECHO,
        ENGINE_BUCKET_BRIGADE_DELAY, ENGINE_DIGITAL_DELAY, ENGINE_HARMONIC_TREMOLO,
        ENGINE_ROTARY_SPEAKER, ENGINE_DETUNE_DOUBLER, ENGINE_LADDER_FILTER,
        ENGINE_FORMANT_FILTER, ENGINE_VCA_COMPRESSOR, ENGINE_STATE_VARIABLE_FILTER,
        ENGINE_DIGITAL_CHORUS, ENGINE_SPECTRAL_FREEZE, ENGINE_GRANULAR_CLOUD,
        ENGINE_RING_MODULATOR, ENGINE_MULTIBAND_SATURATOR, ENGINE_COMB_RESONATOR,
        ENGINE_PITCH_SHIFTER, ENGINE_PHASED_VOCODER, ENGINE_CONVOLUTION_REVERB,
        ENGINE_BIT_CRUSHER, ENGINE_FREQUENCY_SHIFTER, ENGINE_WAVE_FOLDER,
        ENGINE_SHIMMER_REVERB, ENGINE_VOCAL_FORMANT, ENGINE_TRANSIENT_SHAPER,
        ENGINE_DIMENSION_EXPANDER, ENGINE_ANALOG_PHASER, ENGINE_ENVELOPE_FILTER,
        ENGINE_GATED_REVERB, ENGINE_HARMONIC_EXCITER, ENGINE_FEEDBACK_NETWORK,
        ENGINE_INTELLIGENT_HARMONIZER, ENGINE_PARAMETRIC_EQ, ENGINE_MASTERING_LIMITER,
        ENGINE_NOISE_GATE, ENGINE_OPTO_COMPRESSOR, ENGINE_SPECTRAL_GATE,
        ENGINE_CHAOS_GENERATOR, ENGINE_BUFFER_REPEAT, ENGINE_VINTAGE_CONSOLE_EQ,
        ENGINE_MID_SIDE_PROCESSOR, ENGINE_VINTAGE_TUBE, ENGINE_SPRING_REVERB,
        ENGINE_RESONANT_CHORUS, ENGINE_STEREO_WIDENER, ENGINE_STEREO_IMAGER,
        ENGINE_DYNAMIC_EQ
    };
    
    float totalCPU = 0.0f;
    
    for (int engineID : engineIDs) {
        TestResult result = testEngine(engineID);
        summary.results.push_back(result);
        
        if (result.passed()) {
            summary.passedEngines++;
        } else {
            summary.failedEngines++;
        }
        
        totalCPU += result.cpuUsage;
        summary.totalEngines++;
    }
    
    summary.averageCPU = summary.totalEngines > 0 ? totalCPU / summary.totalEngines : 0.0f;
    
    return summary;
}

EngineTestRunner::TestResult EngineTestRunner::testEngine(int engineID) {
    TestResult result;
    result.engineID = engineID;
    
    // Create the engine
    auto engine = EngineFactory::createEngine(engineID);
    
    if (!engine) {
        result.engineName = "Unknown Engine " + juce::String(engineID);
        result.silenceTest = false;
        result.unityGainTest = false;
        result.stabilityTest = false;
        result.notes = "Failed to create engine";
        return result;
    }
    
    result.engineName = engine->getName();
    
    // Prepare the engine
    engine->prepareToPlay(SAMPLE_RATE, BLOCK_SIZE);
    
    // Run tests
    result.silenceTest = testSilence(engine.get());
    result.unityGainTest = testUnityGain(engine.get());
    result.stabilityTest = testStability(engine.get());
    result.cpuUsage = measureCPU(engine.get());
    
    // Generate notes
    if (!result.silenceTest) {
        result.notes += "Generates noise with silence input. ";
    }
    if (!result.unityGainTest) {
        result.notes += "Gain mismatch. ";
    }
    if (!result.stabilityTest) {
        result.notes += "Unstable output. ";
    }
    if (result.cpuUsage > 5.0f) {
        result.notes += "High CPU usage. ";
    }
    
    if (result.passed() && result.cpuUsage <= 5.0f) {
        result.notes = "All tests passed";
    }
    
    return result;
}

bool EngineTestRunner::testSilence(EngineBase* engine) {
    // Generate silence
    juce::AudioBuffer<float> buffer(2, BLOCK_SIZE * 10);
    buffer.clear();
    
    // Process
    for (int i = 0; i < 10; ++i) {
        juce::AudioBuffer<float> block(buffer.getArrayOfWritePointers(), 2, i * BLOCK_SIZE, BLOCK_SIZE);
        engine->process(block);
    }
    
    // Check if output is silence (or very quiet)
    float peak = buffer.getMagnitude(0, buffer.getNumSamples());
    return peak < 0.001f; // -60dB threshold
}

bool EngineTestRunner::testUnityGain(EngineBase* engine) {
    // Reset engine to default
    engine->reset();
    std::map<int, float> defaultParams;
    engine->updateParameters(defaultParams);
    
    // Generate test signal
    juce::AudioBuffer<float> input = generateTestSignal(0, 0.5f); // 0.5 second sine
    juce::AudioBuffer<float> output(input);
    
    // Process
    int numBlocks = output.getNumSamples() / BLOCK_SIZE;
    for (int i = 0; i < numBlocks; ++i) {
        juce::AudioBuffer<float> block(output.getArrayOfWritePointers(), 2, i * BLOCK_SIZE, BLOCK_SIZE);
        engine->process(block);
    }
    
    // Compare RMS
    float inputRMS = input.getRMSLevel(0, 0, input.getNumSamples());
    float outputRMS = output.getRMSLevel(0, 0, output.getNumSamples());
    
    float gainDiff = std::abs(20.0f * std::log10(outputRMS / (inputRMS + 0.00001f)));
    
    return gainDiff < 6.0f; // Within 6dB
}

bool EngineTestRunner::testStability(EngineBase* engine) {
    // Process loud signal
    juce::AudioBuffer<float> buffer = generateTestSignal(0, 1.0f);
    
    // Scale to 0.9 amplitude
    buffer.applyGain(0.9f);
    
    // Process
    int numBlocks = buffer.getNumSamples() / BLOCK_SIZE;
    for (int i = 0; i < numBlocks; ++i) {
        juce::AudioBuffer<float> block(buffer.getArrayOfWritePointers(), 2, i * BLOCK_SIZE, BLOCK_SIZE);
        engine->process(block);
    }
    
    // Check for clipping
    float peak = buffer.getMagnitude(0, buffer.getNumSamples());
    return peak <= 1.0f;
}

float EngineTestRunner::measureCPU(EngineBase* engine) {
    juce::AudioBuffer<float> buffer = generateTestSignal(1, 1.0f); // 1 second white noise
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Process 1 second of audio
    int numBlocks = buffer.getNumSamples() / BLOCK_SIZE;
    for (int i = 0; i < numBlocks; ++i) {
        juce::AudioBuffer<float> block(buffer.getArrayOfWritePointers(), 2, i * BLOCK_SIZE, BLOCK_SIZE);
        engine->process(block);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    float processingTime = duration.count() / 1000000.0f; // Convert to seconds
    float realTime = 1.0f; // We processed 1 second of audio
    
    return (processingTime / realTime) * 100.0f; // Percentage
}

juce::AudioBuffer<float> EngineTestRunner::generateTestSignal(int type, float duration) {
    int numSamples = static_cast<int>(duration * SAMPLE_RATE);
    juce::AudioBuffer<float> buffer(2, numSamples);
    
    if (type == 0) { // Sine wave
        float freq = 1000.0f;
        float omega = 2.0f * M_PI * freq / SAMPLE_RATE;
        
        for (int ch = 0; ch < 2; ++ch) {
            float* data = buffer.getWritePointer(ch);
            for (int i = 0; i < numSamples; ++i) {
                data[i] = 0.5f * std::sin(omega * i);
            }
        }
    } else if (type == 1) { // White noise
        juce::Random random;
        for (int ch = 0; ch < 2; ++ch) {
            float* data = buffer.getWritePointer(ch);
            for (int i = 0; i < numSamples; ++i) {
                data[i] = random.nextFloat() * 0.5f - 0.25f;
            }
        }
    }
    
    return buffer;
}

void EngineTestRunner::generateHTMLReport(const TestSummary& summary, const juce::File& outputFile) {
    std::stringstream html;
    
    html << "<!DOCTYPE html>\n<html>\n<head>\n";
    html << "<title>Chimera Engine Test Report</title>\n";
    html << "<style>\n";
    html << "body { font-family: 'Segoe UI', Arial, sans-serif; margin: 20px; background: #f5f5f5; }\n";
    html << "h1 { color: #333; border-bottom: 3px solid #4CAF50; padding-bottom: 10px; }\n";
    html << ".summary { background: white; padding: 20px; border-radius: 8px; margin: 20px 0; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }\n";
    html << "table { width: 100%; border-collapse: collapse; background: white; margin: 20px 0; border-radius: 8px; overflow: hidden; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }\n";
    html << "th { background: #4CAF50; color: white; padding: 12px; text-align: left; }\n";
    html << "td { padding: 10px; border-bottom: 1px solid #eee; }\n";
    html << "tr:hover { background: #f9f9f9; }\n";
    html << ".pass { color: #4CAF50; font-weight: bold; }\n";
    html << ".fail { color: #f44336; font-weight: bold; }\n";
    html << ".stat { display: inline-block; margin: 10px 20px; }\n";
    html << ".stat-value { font-size: 2em; font-weight: bold; color: #333; }\n";
    html << ".stat-label { color: #666; margin-top: 5px; }\n";
    html << "</style>\n</head>\n<body>\n";
    
    html << "<h1>🎵 Chimera Engine Test Report</h1>\n";
    html << "<p>Generated: " << juce::Time::getCurrentTime().toString(true, true).toStdString() << "</p>\n";
    
    // Summary section
    html << "<div class='summary'>\n";
    html << "<h2>Summary</h2>\n";
    html << "<div class='stat'><div class='stat-value'>" << summary.totalEngines << "</div><div class='stat-label'>Total Engines</div></div>\n";
    html << "<div class='stat'><div class='stat-value pass'>" << summary.passedEngines << "</div><div class='stat-label'>Passed</div></div>\n";
    html << "<div class='stat'><div class='stat-value fail'>" << summary.failedEngines << "</div><div class='stat-label'>Failed</div></div>\n";
    html << "<div class='stat'><div class='stat-value'>" << std::fixed << std::setprecision(1) << summary.getPassRate() << "%</div><div class='stat-label'>Pass Rate</div></div>\n";
    html << "<div class='stat'><div class='stat-value'>" << std::fixed << std::setprecision(2) << summary.averageCPU << "%</div><div class='stat-label'>Avg CPU</div></div>\n";
    html << "</div>\n";
    
    // Detailed results table
    html << "<h2>Detailed Results</h2>\n";
    html << "<table>\n";
    html << "<tr><th>Engine</th><th>ID</th><th>Silence</th><th>Unity Gain</th><th>Stability</th><th>CPU Usage</th><th>Status</th><th>Notes</th></tr>\n";
    
    for (const auto& result : summary.results) {
        html << "<tr>\n";
        html << "<td><strong>" << result.engineName.toStdString() << "</strong></td>\n";
        html << "<td>" << result.engineID << "</td>\n";
        html << "<td>" << (result.silenceTest ? "✓" : "✗") << "</td>\n";
        html << "<td>" << (result.unityGainTest ? "✓" : "✗") << "</td>\n";
        html << "<td>" << (result.stabilityTest ? "✓" : "✗") << "</td>\n";
        html << "<td>" << std::fixed << std::setprecision(2) << result.cpuUsage << "%</td>\n";
        html << "<td class='" << (result.passed() ? "pass'>PASS" : "fail'>FAIL") << "</td>\n";
        html << "<td>" << result.notes.toStdString() << "</td>\n";
        html << "</tr>\n";
    }
    
    html << "</table>\n";
    html << "</body>\n</html>\n";
    
    outputFile.replaceWithText(html.str());
}

void EngineTestRunner::printConsoleReport(const TestSummary& summary) {
    std::cout << "\n=========================================\n";
    std::cout << "Chimera Engine Test Results\n";
    std::cout << "=========================================\n";
    std::cout << "Total Engines: " << summary.totalEngines << "\n";
    std::cout << "Passed: " << summary.passedEngines << "\n";
    std::cout << "Failed: " << summary.failedEngines << "\n";
    std::cout << "Pass Rate: " << std::fixed << std::setprecision(1) << summary.getPassRate() << "%\n";
    std::cout << "Average CPU: " << std::fixed << std::setprecision(2) << summary.averageCPU << "%\n";
    std::cout << "-----------------------------------------\n";
    
    for (const auto& result : summary.results) {
        std::cout << std::setw(25) << std::left << result.engineName.toStdString() << ": ";
        if (result.passed()) {
            std::cout << "✓ PASS";
        } else {
            std::cout << "✗ FAIL";
        }
        std::cout << " (CPU: " << std::fixed << std::setprecision(2) << result.cpuUsage << "%)\n";
    }
    
    std::cout << "=========================================\n";
}