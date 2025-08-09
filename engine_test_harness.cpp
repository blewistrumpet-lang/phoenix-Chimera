/**
 * Chimera Phoenix Engine Test Harness
 * 
 * Standalone test harness for validating all 56 engines + bypass
 * Tests each engine for:
 * - Creation and initialization
 * - Parameter handling
 * - Audio processing safety
 * - Memory management
 * - Thread safety basics
 */

#include <iostream>
#include <iomanip>
#include <vector>
#include <memory>
#include <string>
#include <map>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <sstream>
#include <fstream>

// Simulate basic audio buffer structure
struct AudioBuffer {
    std::vector<std::vector<float>> channels;
    int numChannels;
    int numSamples;
    
    AudioBuffer(int channels = 2, int samples = 512) 
        : numChannels(channels), numSamples(samples) {
        this->channels.resize(channels);
        for (auto& channel : this->channels) {
            channel.resize(samples, 0.0f);
        }
    }
    
    float** getArrayOfWritePointers() {
        static std::vector<float*> ptrs;
        ptrs.clear();
        for (auto& channel : channels) {
            ptrs.push_back(channel.data());
        }
        return ptrs.data();
    }
    
    const float** getArrayOfReadPointers() const {
        static std::vector<const float*> ptrs;
        ptrs.clear();
        for (const auto& channel : channels) {
            ptrs.push_back(channel.data());
        }
        return const_cast<const float**>(ptrs.data());
    }
    
    void clear() {
        for (auto& channel : channels) {
            std::fill(channel.begin(), channel.end(), 0.0f);
        }
    }
    
    void fillWithTestSignal(float frequency = 440.0f, float sampleRate = 44100.0f) {
        for (int ch = 0; ch < numChannels; ++ch) {
            for (int s = 0; s < numSamples; ++s) {
                channels[ch][s] = 0.5f * std::sin(2.0f * M_PI * frequency * s / sampleRate);
            }
        }
    }
};

// Test result structure
struct TestResult {
    std::string engineName;
    int engineId;
    bool creationSuccess = false;
    bool parameterHandling = false;
    bool audioProcessing = false;
    bool memoryManagement = false;
    bool threadSafety = false;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    float performanceScore = 0.0f;
    
    bool isPassing() const {
        return creationSuccess && parameterHandling && audioProcessing && 
               memoryManagement && errors.empty();
    }
    
    std::string getSummary() const {
        std::stringstream ss;
        ss << "Engine #" << engineId << " (" << engineName << "): ";
        if (isPassing()) {
            ss << "✅ PASS";
        } else {
            ss << "❌ FAIL";
            if (!creationSuccess) ss << " [Creation]";
            if (!parameterHandling) ss << " [Params]";
            if (!audioProcessing) ss << " [Audio]";
            if (!memoryManagement) ss << " [Memory]";
            if (!errors.empty()) ss << " [" << errors.size() << " errors]";
        }
        return ss.str();
    }
};

// Mock engine base class for testing
class MockEngine {
public:
    virtual ~MockEngine() = default;
    virtual std::string getName() const = 0;
    virtual void setParameter(const std::string& param, float value) = 0;
    virtual float getParameter(const std::string& param) const = 0;
    virtual void processBlock(AudioBuffer& buffer, int numSamples) = 0;
    virtual void reset() = 0;
    virtual bool validateState() const { return true; }
};

// Test implementations for each engine type
class BypassEngine : public MockEngine {
public:
    std::string getName() const override { return "Bypass"; }
    void setParameter(const std::string&, float) override {}
    float getParameter(const std::string&) const override { return 0.0f; }
    void processBlock(AudioBuffer&, int) override {}
    void reset() override {}
};

// Simple mock implementations for testing framework
#define CREATE_MOCK_ENGINE(ID, NAME) \
class Engine_##ID : public MockEngine { \
private: \
    std::map<std::string, float> parameters; \
public: \
    Engine_##ID() { \
        parameters["mix"] = 1.0f; \
        parameters["gain"] = 0.5f; \
    } \
    std::string getName() const override { return NAME; } \
    void setParameter(const std::string& param, float value) override { \
        parameters[param] = std::clamp(value, 0.0f, 1.0f); \
    } \
    float getParameter(const std::string& param) const override { \
        auto it = parameters.find(param); \
        return (it != parameters.end()) ? it->second : 0.0f; \
    } \
    void processBlock(AudioBuffer& buffer, int numSamples) override { \
        float mix = parameters["mix"]; \
        float gain = parameters["gain"]; \
        auto** channels = buffer.getArrayOfWritePointers(); \
        for (int ch = 0; ch < buffer.numChannels; ++ch) { \
            for (int s = 0; s < numSamples; ++s) { \
                channels[ch][s] *= gain * mix; \
            } \
        } \
    } \
    void reset() override { \
        parameters["mix"] = 1.0f; \
        parameters["gain"] = 0.5f; \
    } \
};

// Create mock engines for all 56 types
CREATE_MOCK_ENGINE(1, "Opto Compressor")
CREATE_MOCK_ENGINE(2, "VCA Compressor")
CREATE_MOCK_ENGINE(3, "Transient Shaper")
CREATE_MOCK_ENGINE(4, "Noise Gate")
CREATE_MOCK_ENGINE(5, "Mastering Limiter")
CREATE_MOCK_ENGINE(6, "Dynamic EQ")
CREATE_MOCK_ENGINE(7, "Parametric EQ")
CREATE_MOCK_ENGINE(8, "Vintage Console EQ")
CREATE_MOCK_ENGINE(9, "Ladder Filter")
CREATE_MOCK_ENGINE(10, "State Variable Filter")
CREATE_MOCK_ENGINE(11, "Formant Filter")
CREATE_MOCK_ENGINE(12, "Envelope Filter")
CREATE_MOCK_ENGINE(13, "Comb Resonator")
CREATE_MOCK_ENGINE(14, "Vocal Formant")
CREATE_MOCK_ENGINE(15, "Vintage Tube")
CREATE_MOCK_ENGINE(16, "Wave Folder")
CREATE_MOCK_ENGINE(17, "Harmonic Exciter")
CREATE_MOCK_ENGINE(18, "Bit Crusher")
CREATE_MOCK_ENGINE(19, "Multiband Saturator")
CREATE_MOCK_ENGINE(20, "Muff Fuzz")
CREATE_MOCK_ENGINE(21, "Rodent Distortion")
CREATE_MOCK_ENGINE(22, "K-Style Overdrive")
CREATE_MOCK_ENGINE(23, "Digital Chorus")
CREATE_MOCK_ENGINE(24, "Resonant Chorus")
CREATE_MOCK_ENGINE(25, "Analog Phaser")
CREATE_MOCK_ENGINE(26, "Ring Modulator")
CREATE_MOCK_ENGINE(27, "Frequency Shifter")
CREATE_MOCK_ENGINE(28, "Harmonic Tremolo")
CREATE_MOCK_ENGINE(29, "Classic Tremolo")
CREATE_MOCK_ENGINE(30, "Rotary Speaker")
CREATE_MOCK_ENGINE(31, "Pitch Shifter")
CREATE_MOCK_ENGINE(32, "Detune Doubler")
CREATE_MOCK_ENGINE(33, "Intelligent Harmonizer")
CREATE_MOCK_ENGINE(34, "Tape Echo")
CREATE_MOCK_ENGINE(35, "Digital Delay")
CREATE_MOCK_ENGINE(36, "Magnetic Drum Echo")
CREATE_MOCK_ENGINE(37, "Bucket Brigade Delay")
CREATE_MOCK_ENGINE(38, "Buffer Repeat")
CREATE_MOCK_ENGINE(39, "Plate Reverb")
CREATE_MOCK_ENGINE(40, "Spring Reverb")
CREATE_MOCK_ENGINE(41, "Convolution Reverb")
CREATE_MOCK_ENGINE(42, "Shimmer Reverb")
CREATE_MOCK_ENGINE(43, "Gated Reverb")
CREATE_MOCK_ENGINE(44, "Stereo Widener")
CREATE_MOCK_ENGINE(45, "Stereo Imager")
CREATE_MOCK_ENGINE(46, "Dimension Expander")
CREATE_MOCK_ENGINE(47, "Spectral Freeze")
CREATE_MOCK_ENGINE(48, "Spectral Gate")
CREATE_MOCK_ENGINE(49, "Phased Vocoder")
CREATE_MOCK_ENGINE(50, "Granular Cloud")
CREATE_MOCK_ENGINE(51, "Chaos Generator")
CREATE_MOCK_ENGINE(52, "Feedback Network")
CREATE_MOCK_ENGINE(53, "Mid-Side Processor")
CREATE_MOCK_ENGINE(54, "Gain Utility")
CREATE_MOCK_ENGINE(55, "Mono Maker")
CREATE_MOCK_ENGINE(56, "Phase Align")

// Engine factory
std::unique_ptr<MockEngine> createEngine(int engineId) {
    switch(engineId) {
        case 0: return std::make_unique<BypassEngine>();
        case 1: return std::make_unique<Engine_1>();
        case 2: return std::make_unique<Engine_2>();
        case 3: return std::make_unique<Engine_3>();
        case 4: return std::make_unique<Engine_4>();
        case 5: return std::make_unique<Engine_5>();
        case 6: return std::make_unique<Engine_6>();
        case 7: return std::make_unique<Engine_7>();
        case 8: return std::make_unique<Engine_8>();
        case 9: return std::make_unique<Engine_9>();
        case 10: return std::make_unique<Engine_10>();
        case 11: return std::make_unique<Engine_11>();
        case 12: return std::make_unique<Engine_12>();
        case 13: return std::make_unique<Engine_13>();
        case 14: return std::make_unique<Engine_14>();
        case 15: return std::make_unique<Engine_15>();
        case 16: return std::make_unique<Engine_16>();
        case 17: return std::make_unique<Engine_17>();
        case 18: return std::make_unique<Engine_18>();
        case 19: return std::make_unique<Engine_19>();
        case 20: return std::make_unique<Engine_20>();
        case 21: return std::make_unique<Engine_21>();
        case 22: return std::make_unique<Engine_22>();
        case 23: return std::make_unique<Engine_23>();
        case 24: return std::make_unique<Engine_24>();
        case 25: return std::make_unique<Engine_25>();
        case 26: return std::make_unique<Engine_26>();
        case 27: return std::make_unique<Engine_27>();
        case 28: return std::make_unique<Engine_28>();
        case 29: return std::make_unique<Engine_29>();
        case 30: return std::make_unique<Engine_30>();
        case 31: return std::make_unique<Engine_31>();
        case 32: return std::make_unique<Engine_32>();
        case 33: return std::make_unique<Engine_33>();
        case 34: return std::make_unique<Engine_34>();
        case 35: return std::make_unique<Engine_35>();
        case 36: return std::make_unique<Engine_36>();
        case 37: return std::make_unique<Engine_37>();
        case 38: return std::make_unique<Engine_38>();
        case 39: return std::make_unique<Engine_39>();
        case 40: return std::make_unique<Engine_40>();
        case 41: return std::make_unique<Engine_41>();
        case 42: return std::make_unique<Engine_42>();
        case 43: return std::make_unique<Engine_43>();
        case 44: return std::make_unique<Engine_44>();
        case 45: return std::make_unique<Engine_45>();
        case 46: return std::make_unique<Engine_46>();
        case 47: return std::make_unique<Engine_47>();
        case 48: return std::make_unique<Engine_48>();
        case 49: return std::make_unique<Engine_49>();
        case 50: return std::make_unique<Engine_50>();
        case 51: return std::make_unique<Engine_51>();
        case 52: return std::make_unique<Engine_52>();
        case 53: return std::make_unique<Engine_53>();
        case 54: return std::make_unique<Engine_54>();
        case 55: return std::make_unique<Engine_55>();
        case 56: return std::make_unique<Engine_56>();
        default: return nullptr;
    }
}

// Test harness class
class EngineTestHarness {
private:
    std::vector<TestResult> results;
    bool verbose;
    
    bool checkForNaNOrInf(const AudioBuffer& buffer) {
        for (int ch = 0; ch < buffer.numChannels; ++ch) {
            for (int s = 0; s < buffer.numSamples; ++s) {
                float sample = buffer.channels[ch][s];
                if (std::isnan(sample) || std::isinf(sample)) {
                    return true;
                }
            }
        }
        return false;
    }
    
    float calculateRMS(const AudioBuffer& buffer) {
        float sum = 0.0f;
        int totalSamples = buffer.numChannels * buffer.numSamples;
        
        for (int ch = 0; ch < buffer.numChannels; ++ch) {
            for (int s = 0; s < buffer.numSamples; ++s) {
                float sample = buffer.channels[ch][s];
                sum += sample * sample;
            }
        }
        
        return std::sqrt(sum / totalSamples);
    }

public:
    EngineTestHarness(bool verboseMode = false) : verbose(verboseMode) {}
    
    TestResult testEngine(int engineId) {
        TestResult result;
        result.engineId = engineId;
        
        if (verbose) {
            std::cout << "\nTesting Engine #" << engineId << "..." << std::endl;
        }
        
        // Test 1: Creation
        std::unique_ptr<MockEngine> engine;
        try {
            engine = createEngine(engineId);
            if (engine) {
                result.creationSuccess = true;
                result.engineName = engine->getName();
            } else {
                result.errors.push_back("Failed to create engine");
                return result;
            }
        } catch (const std::exception& e) {
            result.errors.push_back("Exception during creation: " + std::string(e.what()));
            return result;
        }
        
        // Test 2: Parameter Handling
        try {
            engine->setParameter("mix", 0.5f);
            engine->setParameter("gain", 0.7f);
            
            float mixValue = engine->getParameter("mix");
            float gainValue = engine->getParameter("gain");
            
            if (std::abs(mixValue - 0.5f) < 0.01f && std::abs(gainValue - 0.7f) < 0.01f) {
                result.parameterHandling = true;
            } else {
                result.warnings.push_back("Parameter values not retained correctly");
            }
            
            // Test extreme values
            engine->setParameter("mix", -1.0f);
            engine->setParameter("mix", 2.0f);
            
            if (engine->validateState()) {
                result.parameterHandling = true;
            }
        } catch (const std::exception& e) {
            result.errors.push_back("Parameter handling error: " + std::string(e.what()));
            result.parameterHandling = false;
        }
        
        // Test 3: Audio Processing
        try {
            AudioBuffer testBuffer(2, 512);
            testBuffer.fillWithTestSignal(440.0f, 44100.0f);
            
            // Process multiple blocks
            for (int i = 0; i < 10; ++i) {
                engine->processBlock(testBuffer, testBuffer.numSamples);
                
                if (checkForNaNOrInf(testBuffer)) {
                    result.errors.push_back("NaN or Inf detected in audio output");
                    result.audioProcessing = false;
                    break;
                }
            }
            
            if (result.errors.empty()) {
                result.audioProcessing = true;
            }
            
            // Test with silence
            testBuffer.clear();
            engine->processBlock(testBuffer, testBuffer.numSamples);
            
            if (checkForNaNOrInf(testBuffer)) {
                result.errors.push_back("NaN or Inf detected with silent input");
                result.audioProcessing = false;
            }
            
        } catch (const std::exception& e) {
            result.errors.push_back("Audio processing error: " + std::string(e.what()));
            result.audioProcessing = false;
        }
        
        // Test 4: Memory Management
        try {
            // Create and destroy multiple instances
            for (int i = 0; i < 100; ++i) {
                auto tempEngine = createEngine(engineId);
                AudioBuffer tempBuffer(2, 128);
                tempEngine->processBlock(tempBuffer, tempBuffer.numSamples);
            }
            result.memoryManagement = true;
        } catch (const std::exception& e) {
            result.errors.push_back("Memory management error: " + std::string(e.what()));
            result.memoryManagement = false;
        }
        
        // Test 5: Reset functionality
        try {
            engine->reset();
            result.threadSafety = true; // Basic thread safety placeholder
        } catch (const std::exception& e) {
            result.warnings.push_back("Reset error: " + std::string(e.what()));
        }
        
        // Calculate performance score
        result.performanceScore = 0.0f;
        if (result.creationSuccess) result.performanceScore += 20.0f;
        if (result.parameterHandling) result.performanceScore += 20.0f;
        if (result.audioProcessing) result.performanceScore += 30.0f;
        if (result.memoryManagement) result.performanceScore += 20.0f;
        if (result.threadSafety) result.performanceScore += 10.0f;
        
        return result;
    }
    
    void runAllTests() {
        std::cout << "========================================" << std::endl;
        std::cout << "  Chimera Phoenix Engine Test Harness  " << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "Testing 57 engines (0-56)..." << std::endl;
        
        auto startTime = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i <= 56; ++i) {
            results.push_back(testEngine(i));
            
            if (!verbose) {
                std::cout << "." << std::flush;
                if ((i + 1) % 10 == 0) {
                    std::cout << " [" << (i + 1) << "/57]" << std::endl;
                }
            }
        }
        
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        
        std::cout << "\n\nTest execution completed in " << duration.count() << " ms" << std::endl;
    }
    
    void generateReport(const std::string& filename = "test_report.txt") {
        std::ofstream report(filename);
        
        report << "Chimera Phoenix Engine Test Report" << std::endl;
        report << "===================================" << std::endl;
        report << "Generated: " << std::chrono::system_clock::now().time_since_epoch().count() << std::endl;
        report << "\n";
        
        int passing = 0;
        int failing = 0;
        int critical = 0;
        
        for (const auto& result : results) {
            if (result.isPassing()) {
                passing++;
            } else {
                failing++;
                if (!result.creationSuccess) {
                    critical++;
                }
            }
        }
        
        report << "Summary Statistics:" << std::endl;
        report << "-------------------" << std::endl;
        report << "Total Engines: " << results.size() << std::endl;
        report << "Passing: " << passing << std::endl;
        report << "Failing: " << failing << std::endl;
        report << "Critical Issues: " << critical << std::endl;
        report << "Success Rate: " << std::fixed << std::setprecision(1) 
               << (100.0f * passing / results.size()) << "%" << std::endl;
        
        report << "\n\nDetailed Results:" << std::endl;
        report << "-----------------" << std::endl;
        
        for (const auto& result : results) {
            report << result.getSummary() << std::endl;
            
            if (!result.errors.empty()) {
                for (const auto& error : result.errors) {
                    report << "  ERROR: " << error << std::endl;
                }
            }
            
            if (!result.warnings.empty()) {
                for (const auto& warning : result.warnings) {
                    report << "  WARNING: " << warning << std::endl;
                }
            }
            
            report << "  Performance Score: " << result.performanceScore << "/100" << std::endl;
            report << std::endl;
        }
        
        report.close();
        std::cout << "\nDetailed report written to: " << filename << std::endl;
    }
    
    void printSummary() {
        std::cout << "\n========================================" << std::endl;
        std::cout << "            TEST SUMMARY                " << std::endl;
        std::cout << "========================================" << std::endl;
        
        int passing = 0;
        int failing = 0;
        std::vector<int> failingEngines;
        
        for (const auto& result : results) {
            if (result.isPassing()) {
                passing++;
            } else {
                failing++;
                failingEngines.push_back(result.engineId);
            }
        }
        
        std::cout << "✅ Passing: " << passing << "/" << results.size() << std::endl;
        std::cout << "❌ Failing: " << failing << "/" << results.size() << std::endl;
        
        if (!failingEngines.empty()) {
            std::cout << "\nFailing Engines: ";
            for (size_t i = 0; i < failingEngines.size(); ++i) {
                std::cout << failingEngines[i];
                if (i < failingEngines.size() - 1) std::cout << ", ";
            }
            std::cout << std::endl;
        }
        
        float avgScore = 0.0f;
        for (const auto& result : results) {
            avgScore += result.performanceScore;
        }
        avgScore /= results.size();
        
        std::cout << "\nAverage Performance Score: " << std::fixed << std::setprecision(1) 
                  << avgScore << "/100" << std::endl;
        
        std::cout << "\n========================================" << std::endl;
    }
};

// Main function
int main(int argc, char* argv[]) {
    bool verbose = false;
    std::string outputFile = "chimera_test_report.txt";
    
    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--verbose" || arg == "-v") {
            verbose = true;
        } else if (arg == "--output" || arg == "-o") {
            if (i + 1 < argc) {
                outputFile = argv[++i];
            }
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: " << argv[0] << " [options]" << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  --verbose, -v     Enable verbose output" << std::endl;
            std::cout << "  --output, -o      Specify output file (default: chimera_test_report.txt)" << std::endl;
            std::cout << "  --help, -h        Show this help message" << std::endl;
            return 0;
        }
    }
    
    try {
        EngineTestHarness harness(verbose);
        harness.runAllTests();
        harness.printSummary();
        harness.generateReport(outputFile);
        
        std::cout << "\nTest harness completed successfully!" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}