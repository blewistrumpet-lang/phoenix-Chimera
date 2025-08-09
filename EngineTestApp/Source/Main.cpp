/*
  ==============================================================================

    Chimera Phoenix Engine Test Application
    Tests all 57 DSP engines for functionality

  ==============================================================================
*/

#include <JuceHeader.h>
#include "../../JUCE_Plugin/Source/EngineFactory.h"
#include "../../JUCE_Plugin/Source/EngineBase.h"
#include <iomanip>
#include <chrono>

//==============================================================================
class EngineTestApplication : public juce::JUCEApplication
{
public:
    //==============================================================================
    EngineTestApplication() {}

    const juce::String getApplicationName() override       { return "Chimera Engine Test"; }
    const juce::String getApplicationVersion() override    { return "1.0.0"; }
    bool moreThanOneInstanceAllowed() override             { return true; }

    //==============================================================================
    void initialise (const juce::String& commandLine) override
    {
        // Run tests immediately
        runEngineTests();
        
        // Quit after tests complete
        quit();
    }

    void shutdown() override
    {
        // Cleanup if needed
    }

    //==============================================================================
    void systemRequestedQuit() override
    {
        quit();
    }

    void anotherInstanceStarted (const juce::String& commandLine) override
    {
        // Not used
    }

    //==============================================================================
    struct TestResult 
    {
        int engineId;
        juce::String engineName;
        bool created = false;
        bool initialized = false;
        bool processed = false;
        bool modifiesAudio = false;
        float processingTimeMs = 0.0f;
        float rmsChange = 0.0f;
        juce::String error;
    };

    void runEngineTests()
    {
        std::cout << "\n";
        std::cout << "╔═══════════════════════════════════════════════════════════╗\n";
        std::cout << "║         CHIMERA PHOENIX ENGINE TEST SUITE v1.0           ║\n";
        std::cout << "╚═══════════════════════════════════════════════════════════╝\n\n";

        const double sampleRate = 48000.0;
        const int blockSize = 512;
        const int testDurationSamples = 48000; // 1 second
        
        std::vector<TestResult> results;
        
        // Test each engine
        for (int engineId = 0; engineId <= 56; ++engineId)
        {
            TestResult result = testEngine(engineId, sampleRate, blockSize, testDurationSamples);
            results.push_back(result);
            printResult(result);
        }
        
        // Print summary
        printSummary(results);
        
        // Save detailed report
        saveReport(results);
    }

    TestResult testEngine(int engineId, double sampleRate, int blockSize, int testDurationSamples)
    {
        TestResult result;
        result.engineId = engineId;
        
        std::cout << "[" << std::setw(2) << std::setfill('0') << engineId << "] ";
        std::cout.flush();
        
        // Create engine
        try 
        {
            auto engine = EngineFactory::createEngine(engineId);
            if (!engine)
            {
                result.error = "Failed to create engine";
                return result;
            }
            result.created = true;
            result.engineName = engine->getName();
            
            std::cout << std::setw(30) << std::setfill(' ') << std::left 
                     << result.engineName.toStdString() << " ";
            std::cout.flush();
            
            // Initialize
            engine->prepareToPlay(sampleRate, blockSize);
            result.initialized = true;
            
            // Set parameters for maximum effect
            std::map<int, float> params;
            for (int i = 0; i < engine->getNumParameters(); ++i)
            {
                auto paramName = engine->getParameterName(i).toLowerCase();
                
                if (paramName.contains("mix") || paramName.contains("wet"))
                    params[i] = 1.0f;  // 100% wet
                else if (paramName.contains("drive") || paramName.contains("gain"))
                    params[i] = 0.75f; // High drive
                else if (paramName.contains("depth") || paramName.contains("amount"))
                    params[i] = 0.8f;  // High depth
                else if (paramName.contains("feedback") || paramName.contains("resonance"))
                    params[i] = 0.6f;  // Moderate feedback
                else if (paramName.contains("time") || paramName.contains("delay"))
                    params[i] = 0.3f;  // Short delays for testing
                else
                    params[i] = 0.5f;  // Default middle
            }
            engine->updateParameters(params);
            
            // Create test signal
            juce::AudioBuffer<float> testBuffer(2, testDurationSamples);
            juce::AudioBuffer<float> originalBuffer(2, testDurationSamples);
            
            // Generate appropriate test signal based on engine ID
            generateTestSignal(testBuffer, engineId, sampleRate);
            originalBuffer.makeCopyOf(testBuffer);
            
            // Process in blocks and measure time
            auto startTime = std::chrono::high_resolution_clock::now();
            
            for (int offset = 0; offset < testDurationSamples; offset += blockSize)
            {
                int samplesThisBlock = juce::jmin(blockSize, testDurationSamples - offset);
                
                // Create sub-buffer for this block
                juce::AudioBuffer<float> blockBuffer(
                    testBuffer.getArrayOfWritePointers(),
                    testBuffer.getNumChannels(),
                    offset,
                    samplesThisBlock
                );
                
                engine->process(blockBuffer);
            }
            
            auto endTime = std::chrono::high_resolution_clock::now();
            result.processingTimeMs = std::chrono::duration<float, std::milli>(endTime - startTime).count();
            result.processed = true;
            
            // Analyze results
            result.modifiesAudio = detectModification(originalBuffer, testBuffer);
            result.rmsChange = calculateRMSChange(originalBuffer, testBuffer);
            
            // Reset engine for cleanup
            engine->reset();
        }
        catch (const std::exception& e)
        {
            result.error = juce::String(e.what());
        }
        
        return result;
    }

    void generateTestSignal(juce::AudioBuffer<float>& buffer, int engineId, double sampleRate)
    {
        juce::Random random;
        const int numSamples = buffer.getNumSamples();
        
        // Different test signals for different engine types
        if (engineId == 0) // Bypass
        {
            // Simple sine wave
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            {
                auto* data = buffer.getWritePointer(ch);
                for (int i = 0; i < numSamples; ++i)
                {
                    data[i] = 0.5f * std::sin(2.0 * juce::MathConstants<double>::pi * 440.0 * i / sampleRate);
                }
            }
        }
        else if (engineId >= 1 && engineId <= 6) // Dynamics
        {
            // Dynamic sine with envelope
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            {
                auto* data = buffer.getWritePointer(ch);
                for (int i = 0; i < numSamples; ++i)
                {
                    float envelope = (i < numSamples/2) ? 0.9f : 0.3f;
                    data[i] = envelope * std::sin(2.0 * juce::MathConstants<double>::pi * 1000.0 * i / sampleRate);
                }
            }
        }
        else if (engineId >= 7 && engineId <= 14) // Filters
        {
            // White noise for filter testing
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            {
                auto* data = buffer.getWritePointer(ch);
                for (int i = 0; i < numSamples; ++i)
                {
                    data[i] = random.nextFloat() * 0.6f - 0.3f;
                }
            }
        }
        else if (engineId >= 34 && engineId <= 43) // Reverb/Delay
        {
            // Impulse for reverb testing
            buffer.clear();
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            {
                buffer.setSample(ch, 0, 1.0f);
                buffer.setSample(ch, numSamples/4, 0.5f);
                buffer.setSample(ch, numSamples/2, 0.25f);
            }
        }
        else // Default complex signal
        {
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            {
                auto* data = buffer.getWritePointer(ch);
                for (int i = 0; i < numSamples; ++i)
                {
                    float t = i / sampleRate;
                    data[i] = 0.3f * std::sin(2.0 * juce::MathConstants<double>::pi * 100.0 * t) +
                             0.2f * std::sin(2.0 * juce::MathConstants<double>::pi * 440.0 * t) +
                             0.1f * std::sin(2.0 * juce::MathConstants<double>::pi * 2000.0 * t);
                }
            }
        }
    }

    bool detectModification(const juce::AudioBuffer<float>& original, const juce::AudioBuffer<float>& processed)
    {
        const int numSamples = original.getNumSamples();
        const float threshold = 0.0001f;
        
        for (int ch = 0; ch < original.getNumChannels(); ++ch)
        {
            const float* origData = original.getReadPointer(ch);
            const float* procData = processed.getReadPointer(ch);
            
            for (int i = 0; i < numSamples; ++i)
            {
                if (std::abs(origData[i] - procData[i]) > threshold)
                    return true;
            }
        }
        return false;
    }

    float calculateRMSChange(const juce::AudioBuffer<float>& original, const juce::AudioBuffer<float>& processed)
    {
        float origRMS = original.getRMSLevel(0, 0, original.getNumSamples());
        float procRMS = processed.getRMSLevel(0, 0, processed.getNumSamples());
        
        if (origRMS < 0.0001f) return 0.0f;
        return (procRMS - origRMS) / origRMS;
    }

    void printResult(const TestResult& result)
    {
        if (!result.created)
        {
            std::cout << "❌ FAILED: " << result.error.toStdString() << "\n";
        }
        else if (!result.initialized)
        {
            std::cout << "❌ INIT FAILED: " << result.error.toStdString() << "\n";
        }
        else if (!result.processed)
        {
            std::cout << "❌ PROCESS FAILED: " << result.error.toStdString() << "\n";
        }
        else if (result.engineId == 0) // Bypass
        {
            if (!result.modifiesAudio)
                std::cout << "✅ PASS (bypass)\n";
            else
                std::cout << "❌ FAIL (bypass modified signal!)\n";
        }
        else
        {
            if (result.modifiesAudio)
            {
                std::cout << "✅ PASS ";
                std::cout << "[" << std::fixed << std::setprecision(1) 
                         << result.processingTimeMs << "ms, "
                         << "RMS:" << std::showpos << std::setprecision(1) 
                         << (result.rmsChange * 100) << "%]\n";
            }
            else
            {
                std::cout << "⚠️  WARNING: No modification detected\n";
            }
        }
    }

    void printSummary(const std::vector<TestResult>& results)
    {
        int passed = 0, failed = 0, warnings = 0;
        
        for (const auto& r : results)
        {
            if (!r.created || !r.initialized || !r.processed)
                failed++;
            else if (r.engineId == 0)
                (!r.modifiesAudio) ? passed++ : failed++;
            else
                r.modifiesAudio ? passed++ : warnings++;
        }
        
        std::cout << "\n";
        std::cout << "╔═══════════════════════════════════════════════════════════╗\n";
        std::cout << "║                        SUMMARY                           ║\n";
        std::cout << "╚═══════════════════════════════════════════════════════════╝\n";
        std::cout << "  Total Engines: " << results.size() << "\n";
        std::cout << "  ✅ Passed: " << passed << " (" 
                  << std::fixed << std::setprecision(1) 
                  << (passed * 100.0 / results.size()) << "%)\n";
        std::cout << "  ❌ Failed: " << failed << "\n";
        std::cout << "  ⚠️  Warnings: " << warnings << "\n\n";
        
        if (failed > 0)
        {
            std::cout << "Failed Engines:\n";
            for (const auto& r : results)
            {
                if (!r.created || !r.initialized || !r.processed || 
                    (r.engineId == 0 && r.modifiesAudio))
                {
                    std::cout << "  - #" << r.engineId << " " << r.engineName 
                             << ": " << r.error << "\n";
                }
            }
        }
    }

    void saveReport(const std::vector<TestResult>& results)
    {
        juce::File reportFile = juce::File::getCurrentWorkingDirectory()
                                    .getChildFile("engine_test_report.txt");
        
        juce::String report;
        report << "Chimera Phoenix Engine Test Report\n";
        report << "Generated: " << juce::Time::getCurrentTime().toString(true, true) << "\n\n";
        
        report << "ID | Name                          | Status     | Modifies | Time (ms) | RMS Change\n";
        report << "---|-------------------------------|------------|----------|-----------|------------\n";
        
        for (const auto& r : results)
        {
            report << juce::String(r.engineId).paddedRight(' ', 2) << " | ";
            report << r.engineName.paddedRight(' ', 29) << " | ";
            
            if (!r.created) report << "CREATE_FAIL";
            else if (!r.initialized) report << "INIT_FAIL  ";
            else if (!r.processed) report << "PROC_FAIL  ";
            else report << "OK         ";
            
            report << " | ";
            report << (r.modifiesAudio ? "Yes     " : "No      ") << " | ";
            report << juce::String(r.processingTimeMs, 2).paddedLeft(' ', 9) << " | ";
            report << juce::String(r.rmsChange * 100, 1).paddedLeft(' ', 10) << "%\n";
        }
        
        reportFile.replaceWithText(report);
        std::cout << "Report saved to: " << reportFile.getFullPathName() << "\n";
    }
};

//==============================================================================
START_JUCE_APPLICATION (EngineTestApplication)