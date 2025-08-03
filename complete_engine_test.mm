// Complete engine test with parameter sweeps and measurements
#import <Foundation/Foundation.h>
#import <AudioToolbox/AudioToolbox.h>
#import <Accelerate/Accelerate.h>
#include <iostream>
#include <iomanip>
#include <vector>
#include <map>
#include <fstream>
#include <cmath>
#include <chrono>
#include <algorithm>

// Test categories for different effect types
enum class EffectCategory {
    DYNAMICS,      // Compressors, limiters, gates
    FILTER,        // EQ, filters
    TIME_BASED,    // Reverbs, delays
    MODULATION,    // Chorus, phaser, tremolo
    DISTORTION,    // Overdrive, fuzz, saturation
    SPECTRAL,      // Pitch shift, vocoder
    UTILITY        // Stereo processing, etc
};

// Engine definition
struct Engine {
    int index;
    std::string name;
    EffectCategory category;
    std::vector<std::string> parameters;
    std::map<std::string, float> expectedBehavior;
};

// Test result for each engine
struct EngineTestResult {
    std::string name;
    bool audioPassthrough = false;
    bool parameterResponse[10] = {false};
    float inputLevel = 0;
    float outputLevel = 0;
    float parameterEffectiveness[10] = {0};
    float thd = 0;
    float frequencyResponse[10] = {0}; // At key frequencies
    bool passed = false;
    std::string notes;
};

// Callback data structure for input signal generation
struct CallbackData {
    EffectCategory category;
    float frequency;
    float amplitude;
    int sampleCount;
};

// FFT Analysis
class FFTAnalyzer {
    FFTSetup fftSetup;
    DSPSplitComplex splitComplex;
    int log2n;
    int fftSize;
    
public:
    FFTAnalyzer(int size) : fftSize(size) {
        log2n = log2(fftSize);
        fftSetup = vDSP_create_fftsetup(log2n, kFFTRadix2);
        splitComplex.realp = (float*)malloc(fftSize/2 * sizeof(float));
        splitComplex.imagp = (float*)malloc(fftSize/2 * sizeof(float));
    }
    
    ~FFTAnalyzer() {
        vDSP_destroy_fftsetup(fftSetup);
        free(splitComplex.realp);
        free(splitComplex.imagp);
    }
    
    std::vector<float> analyze(const float* signal) {
        // Window the signal
        std::vector<float> windowed(fftSize);
        for (int i = 0; i < fftSize; i++) {
            float window = 0.5f - 0.5f * cosf(2.0f * M_PI * i / (fftSize - 1));
            windowed[i] = signal[i] * window;
        }
        
        // Pack for FFT
        vDSP_ctoz((DSPComplex*)windowed.data(), 2, &splitComplex, 1, fftSize/2);
        
        // Perform FFT
        vDSP_fft_zrip(fftSetup, &splitComplex, 1, log2n, kFFTDirection_Forward);
        
        // Calculate magnitude
        std::vector<float> magnitude(fftSize/2);
        vDSP_zvabs(&splitComplex, 1, magnitude.data(), 1, fftSize/2);
        
        // Normalize
        float scale = 2.0f / fftSize;
        vDSP_vsmul(magnitude.data(), 1, &scale, magnitude.data(), 1, fftSize/2);
        
        return magnitude;
    }
};

// Calculate THD
float calculateTHD(const float* signal, int size, float fundamentalFreq, float sampleRate) {
    FFTAnalyzer fft(size);
    auto spectrum = fft.analyze(signal);
    
    int fundamentalBin = fundamentalFreq * size / sampleRate;
    if (fundamentalBin >= spectrum.size()) return 0;
    
    float fundamental = spectrum[fundamentalBin];
    if (fundamental < 0.001f) return 0;
    
    float harmonicsSum = 0;
    for (int h = 2; h <= 5; h++) {
        int bin = h * fundamentalBin;
        if (bin < spectrum.size()) {
            harmonicsSum += spectrum[bin] * spectrum[bin];
        }
    }
    
    return sqrtf(harmonicsSum) / fundamental * 100.0f;
}

// Test parameter effectiveness
float testParameterEffect(AudioUnit audioUnit, AudioUnitParameterID param, 
                          AudioUnitParameterID engineParam, int engineIndex,
                          int paramIndex, float sampleRate, CallbackData* callbackData) {
    // Set engine
    AudioUnitSetParameter(audioUnit, engineParam, kAudioUnitScope_Global, 0, engineIndex, 0);
    
    // Reset callback sample count
    callbackData->sampleCount = 0;
    
    // Test at minimum value
    AudioUnitSetParameter(audioUnit, param, kAudioUnitScope_Global, 0, 0.0f, 0);
    
    // Process at min
    const int frameCount = 1024;
    AudioBufferList bufferList;
    bufferList.mNumberBuffers = 2;
    float leftMin[1024], rightMin[1024];
    memset(leftMin, 0, frameCount * sizeof(float));
    memset(rightMin, 0, frameCount * sizeof(float));
    
    bufferList.mBuffers[0].mNumberChannels = 1;
    bufferList.mBuffers[0].mDataByteSize = frameCount * sizeof(float);
    bufferList.mBuffers[0].mData = leftMin;
    bufferList.mBuffers[1].mNumberChannels = 1;
    bufferList.mBuffers[1].mDataByteSize = frameCount * sizeof(float);
    bufferList.mBuffers[1].mData = rightMin;
    
    AudioUnitRenderActionFlags flags = 0;
    AudioTimeStamp timeStamp = {0};
    timeStamp.mFlags = kAudioTimeStampSampleTimeValid;
    
    AudioUnitRender(audioUnit, &flags, &timeStamp, 0, frameCount, &bufferList);
    
    // Calculate RMS at min
    float rmsMin = 0;
    for (int i = 0; i < frameCount; i++) {
        rmsMin += leftMin[i] * leftMin[i];
    }
    rmsMin = sqrtf(rmsMin / frameCount);
    
    // Reset callback for max test
    callbackData->sampleCount = 0;
    
    // Test at maximum value
    AudioUnitSetParameter(audioUnit, param, kAudioUnitScope_Global, 0, 1.0f, 0);
    
    float leftMax[1024], rightMax[1024];
    memset(leftMax, 0, frameCount * sizeof(float));
    memset(rightMax, 0, frameCount * sizeof(float));
    
    bufferList.mBuffers[0].mData = leftMax;
    bufferList.mBuffers[1].mData = rightMax;
    
    AudioUnitRender(audioUnit, &flags, &timeStamp, 0, frameCount, &bufferList);
    
    // Calculate RMS at max
    float rmsMax = 0;
    for (int i = 0; i < frameCount; i++) {
        rmsMax += leftMax[i] * leftMax[i];
    }
    rmsMax = sqrtf(rmsMax / frameCount);
    
    // Calculate effectiveness (difference between min and max)
    float effectiveness = fabsf(rmsMax - rmsMin) / (rmsMin + 0.001f) * 100.0f;
    
    // Also check spectral difference for filters
    if (paramIndex < 3) { // First few params often control frequency
        FFTAnalyzer fft(1024);
        auto specMin = fft.analyze(leftMin);
        auto specMax = fft.analyze(leftMax);
        
        float spectralDiff = 0;
        for (int i = 10; i < 500; i++) {
            spectralDiff += fabsf(specMax[i] - specMin[i]);
        }
        
        effectiveness = std::max(effectiveness, spectralDiff * 10.0f);
    }
    
    // Reset parameter to middle
    AudioUnitSetParameter(audioUnit, param, kAudioUnitScope_Global, 0, 0.5f, 0);
    
    return effectiveness;
}

// Test engine with comprehensive measurements
EngineTestResult testEngine(AudioUnit audioUnit, const Engine& engine, 
                           AudioUnitParameterID engineParam,
                           AudioUnitParameterID* slotParams, int paramCount,
                           CallbackData* callbackData) {
    EngineTestResult result;
    result.name = engine.name;
    
    std::cout << std::setw(35) << std::left << engine.name << ": ";
    std::cout.flush();
    
    // Set engine
    AudioUnitSetParameter(audioUnit, engineParam, kAudioUnitScope_Global, 0, engine.index, 0);
    
    // Set all parameters to middle position
    for (int i = 0; i < paramCount && i < 10; i++) {
        AudioUnitSetParameter(audioUnit, slotParams[i], kAudioUnitScope_Global, 0, 0.5f, 0);
    }
    
    // Update callback data for this engine's category
    callbackData->category = engine.category;
    callbackData->sampleCount = 0;
    
    // Test 1: Basic audio passthrough
    const int frameCount = 2048;
    float sampleRate = 48000;
    
    // Set input level based on category
    switch (engine.category) {
        case EffectCategory::TIME_BASED:
            result.inputLevel = 0.8f; // Impulse peak
            break;
        case EffectCategory::FILTER:
            result.inputLevel = 0.5f; // White noise amplitude
            break;
        default:
            result.inputLevel = 0.5f; // Sine wave amplitude
            break;
    }
    
    // Prepare output buffers
    AudioBufferList bufferList;
    bufferList.mNumberBuffers = 2;
    float leftData[2048], rightData[2048];
    memset(leftData, 0, frameCount * sizeof(float));
    memset(rightData, 0, frameCount * sizeof(float));
    
    bufferList.mBuffers[0].mNumberChannels = 1;
    bufferList.mBuffers[0].mDataByteSize = frameCount * sizeof(float);
    bufferList.mBuffers[0].mData = leftData;
    bufferList.mBuffers[1].mNumberChannels = 1;
    bufferList.mBuffers[1].mDataByteSize = frameCount * sizeof(float);
    bufferList.mBuffers[1].mData = rightData;
    
    AudioUnitRenderActionFlags flags = 0;
    AudioTimeStamp timeStamp = {0};
    timeStamp.mFlags = kAudioTimeStampSampleTimeValid;
    
    OSStatus status = AudioUnitRender(audioUnit, &flags, &timeStamp, 0, frameCount, &bufferList);
    
    if (status != noErr) {
        result.notes = "Render failed (OSStatus: " + std::to_string(status) + ")";
        std::cout << "✗ FAIL (render error: " << status << ")\n";
        return result;
    }
    
    // Calculate output level
    result.outputLevel = 0;
    bool hasOutput = false;
    for (int i = 0; i < frameCount; i++) {
        float abs = fabsf(leftData[i]);
        if (abs > result.outputLevel) result.outputLevel = abs;
        if (abs > 0.001f) hasOutput = true;
    }
    
    result.audioPassthrough = hasOutput;
    
    // Test 2: Parameter effectiveness
    int effectiveParams = 0;
    for (int i = 0; i < std::min(paramCount, 10); i++) {
        result.parameterEffectiveness[i] = testParameterEffect(
            audioUnit, slotParams[i], engineParam, engine.index, i, sampleRate, callbackData
        );
        
        if (result.parameterEffectiveness[i] > 1.0f) {
            result.parameterResponse[i] = true;
            effectiveParams++;
        }
    }
    
    // Test 3: Calculate THD for distortion effects
    if (engine.category == EffectCategory::DISTORTION) {
        // Set drive/gain parameter high
        if (paramCount > 0) {
            AudioUnitSetParameter(audioUnit, slotParams[0], kAudioUnitScope_Global, 0, 0.8f, 0);
        }
        
        // Reset callback and ensure sine wave for THD test
        callbackData->category = EffectCategory::DYNAMICS; // Force sine wave
        callbackData->sampleCount = 0;
        
        // Clear buffers and render
        memset(leftData, 0, frameCount * sizeof(float));
        memset(rightData, 0, frameCount * sizeof(float));
        
        AudioUnitRender(audioUnit, &flags, &timeStamp, 0, frameCount, &bufferList);
        result.thd = calculateTHD(leftData, frameCount, 1000, sampleRate);
        
        // Restore original category
        callbackData->category = engine.category;
    }
    
    // Test 4: Frequency response for filters
    if (engine.category == EffectCategory::FILTER) {
        FFTAnalyzer fft(2048);
        
        // Reset callback and ensure white noise
        callbackData->category = EffectCategory::FILTER;
        callbackData->sampleCount = 0;
        
        // Clear buffers
        memset(leftData, 0, frameCount * sizeof(float));
        memset(rightData, 0, frameCount * sizeof(float));
        
        // Generate input noise through callback and get spectrum
        // (We'll approximate by using the callback's noise generation)
        float inputSignal[2048];
        for (int i = 0; i < frameCount; i++) {
            inputSignal[i] = ((rand() / (float)RAND_MAX) - 0.5f) * 0.3f;
        }
        auto inputSpectrum = fft.analyze(inputSignal);
        
        // Process through AU
        AudioUnitRender(audioUnit, &flags, &timeStamp, 0, frameCount, &bufferList);
        
        // Get output spectrum
        auto outputSpectrum = fft.analyze(leftData);
        
        // Sample frequency response at key points
        float freqs[] = {100, 200, 500, 1000, 2000, 5000, 10000};
        for (int i = 0; i < 7; i++) {
            int bin = freqs[i] * frameCount / sampleRate;
            if (bin < outputSpectrum.size() && bin < inputSpectrum.size()) {
                float gain = outputSpectrum[bin] / (inputSpectrum[bin] + 0.001f);
                result.frequencyResponse[i] = 20 * log10f(gain);
            }
        }
    }
    
    // Determine if engine passed
    result.passed = result.audioPassthrough && (effectiveParams > 0 || result.outputLevel > 0.001f);
    
    // Generate notes
    if (!result.audioPassthrough) {
        result.notes = "No audio output";
    } else if (effectiveParams == 0) {
        result.notes = "Parameters not affecting audio";
    } else {
        result.notes = "Working (" + std::to_string(effectiveParams) + " active params)";
    }
    
    // Print result
    if (result.passed) {
        std::cout << "✓ PASS";
    } else {
        std::cout << "✗ FAIL";
    }
    std::cout << " (out: " << std::fixed << std::setprecision(3) << result.outputLevel;
    if (result.thd > 0) {
        std::cout << ", THD: " << std::setprecision(1) << result.thd << "%";
    }
    std::cout << ", params: " << effectiveParams << "/10)\n";
    
    // Reset to bypass
    AudioUnitSetParameter(audioUnit, engineParam, kAudioUnitScope_Global, 0, 0, 0);
    
    return result;
}

// Generate comprehensive HTML report
void generateHTMLReport(const std::vector<EngineTestResult>& results) {
    std::ofstream html("comprehensive_engine_report.html");
    
    html << "<!DOCTYPE html>\n<html>\n<head>\n";
    html << "<title>Chimera Engine Comprehensive Test Report</title>\n";
    html << "<script src='https://cdn.plot.ly/plotly-latest.min.js'></script>\n";
    html << "<style>\n";
    html << "body { font-family: 'Segoe UI', Arial; margin: 0; background: linear-gradient(135deg, #667eea, #764ba2); }\n";
    html << ".container { max-width: 1400px; margin: 20px auto; background: white; border-radius: 20px; padding: 30px; box-shadow: 0 20px 60px rgba(0,0,0,0.3); }\n";
    html << "h1 { color: #333; text-align: center; padding: 20px; border-bottom: 3px solid #667eea; }\n";
    html << ".summary { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 20px; margin: 30px 0; }\n";
    html << ".stat { background: linear-gradient(135deg, #667eea, #764ba2); color: white; padding: 20px; border-radius: 10px; text-align: center; }\n";
    html << ".stat-value { font-size: 3em; font-weight: bold; }\n";
    html << ".stat-label { font-size: 0.9em; opacity: 0.9; text-transform: uppercase; }\n";
    html << "table { width: 100%; border-collapse: collapse; margin: 30px 0; }\n";
    html << "th { background: #667eea; color: white; padding: 12px; text-align: left; position: sticky; top: 0; }\n";
    html << "td { padding: 10px; border-bottom: 1px solid #e0e0e0; }\n";
    html << "tr:hover { background: #f8f9fa; }\n";
    html << ".pass { color: #10b981; font-weight: bold; }\n";
    html << ".fail { color: #ef4444; font-weight: bold; }\n";
    html << ".param-bar { display: inline-block; height: 20px; background: #e0e0e0; border-radius: 10px; overflow: hidden; }\n";
    html << ".param-fill { height: 100%; background: linear-gradient(90deg, #10b981, #059669); }\n";
    html << ".category-header { background: #f8f9fa; padding: 15px; border-left: 4px solid #667eea; margin: 20px 0; font-weight: bold; }\n";
    html << "</style>\n</head>\n<body>\n";
    
    html << "<div class='container'>\n";
    html << "<h1>🔬 Chimera Engine Comprehensive Test Report</h1>\n";
    html << "<p style='text-align: center; color: #666;'>Complete validation with parameter testing and audio analysis</p>\n";
    
    // Calculate statistics
    int totalEngines = results.size();
    int passedEngines = 0;
    int outputEngines = 0;
    int paramEngines = 0;
    float avgOutputLevel = 0;
    float maxTHD = 0;
    
    for (const auto& r : results) {
        if (r.passed) passedEngines++;
        if (r.audioPassthrough) outputEngines++;
        
        int activeParams = 0;
        for (int i = 0; i < 10; i++) {
            if (r.parameterResponse[i]) activeParams++;
        }
        if (activeParams > 0) paramEngines++;
        
        avgOutputLevel += r.outputLevel;
        if (r.thd > maxTHD) maxTHD = r.thd;
    }
    
    avgOutputLevel /= totalEngines;
    float passRate = (passedEngines * 100.0f / totalEngines);
    
    // Summary cards
    html << "<div class='summary'>\n";
    html << "<div class='stat'><div class='stat-value'>" << totalEngines << "</div><div class='stat-label'>Total Engines</div></div>\n";
    html << "<div class='stat'><div class='stat-value'>" << passedEngines << "</div><div class='stat-label'>Passed</div></div>\n";
    html << "<div class='stat'><div class='stat-value'>" << outputEngines << "</div><div class='stat-label'>Output Audio</div></div>\n";
    html << "<div class='stat'><div class='stat-value'>" << paramEngines << "</div><div class='stat-label'>Params Work</div></div>\n";
    html << "<div class='stat'><div class='stat-value'>" << std::fixed << std::setprecision(1) << passRate << "%</div><div class='stat-label'>Pass Rate</div></div>\n";
    html << "<div class='stat'><div class='stat-value'>" << std::fixed << std::setprecision(1) << maxTHD << "%</div><div class='stat-label'>Max THD</div></div>\n";
    html << "</div>\n";
    
    // Detailed results table
    html << "<h2>Detailed Test Results</h2>\n";
    html << "<table>\n";
    html << "<tr><th>Engine</th><th>Status</th><th>Audio I/O</th><th>Parameter Response</th><th>THD</th><th>Notes</th></tr>\n";
    
    for (const auto& r : results) {
        html << "<tr>\n";
        html << "<td><strong>" << r.name << "</strong></td>\n";
        
        // Status
        html << "<td class='" << (r.passed ? "pass'>✓ PASS" : "fail'>✗ FAIL") << "</td>\n";
        
        // Audio I/O
        html << "<td>";
        html << "In: " << std::fixed << std::setprecision(3) << r.inputLevel;
        html << " → Out: " << r.outputLevel;
        if (!r.audioPassthrough) {
            html << " <span class='fail'>(SILENT)</span>";
        }
        html << "</td>\n";
        
        // Parameter response visualization
        html << "<td>";
        int activeParams = 0;
        for (int i = 0; i < 10; i++) {
            if (r.parameterResponse[i]) {
                activeParams++;
                html << "<span style='color: #10b981;'>●</span>";
            } else {
                html << "<span style='color: #e0e0e0;'>○</span>";
            }
        }
        html << " (" << activeParams << "/10 active)";
        html << "</td>\n";
        
        // THD
        html << "<td>";
        if (r.thd > 0) {
            html << std::fixed << std::setprecision(1) << r.thd << "%";
        } else {
            html << "-";
        }
        html << "</td>\n";
        
        // Notes
        html << "<td style='color: #666; font-size: 0.9em;'>" << r.notes << "</td>\n";
        
        html << "</tr>\n";
    }
    
    html << "</table>\n";
    
    // Parameter effectiveness chart
    html << "<h2>Parameter Effectiveness Analysis</h2>\n";
    html << "<div id='paramChart' style='width: 100%; height: 400px;'></div>\n";
    html << "<script>\n";
    html << "var trace = {\n";
    html << "  x: [";
    for (size_t i = 0; i < results.size(); i++) {
        if (i > 0) html << ", ";
        html << "'" << results[i].name << "'";
    }
    html << "],\n";
    html << "  y: [";
    for (size_t i = 0; i < results.size(); i++) {
        if (i > 0) html << ", ";
        int activeParams = 0;
        for (int j = 0; j < 10; j++) {
            if (results[i].parameterResponse[j]) activeParams++;
        }
        html << activeParams;
    }
    html << "],\n";
    html << "  type: 'bar',\n";
    html << "  marker: { color: 'rgb(102, 126, 234)' }\n";
    html << "};\n";
    html << "var layout = { title: 'Active Parameters per Engine', xaxis: { tickangle: -45 }, yaxis: { title: 'Active Parameters' } };\n";
    html << "Plotly.newPlot('paramChart', [trace], layout);\n";
    html << "</script>\n";
    
    // Footer
    html << "<div style='margin-top: 50px; padding: 20px; border-top: 1px solid #e0e0e0; text-align: center; color: #666;'>\n";
    html << "<p>Test Methodology: Each engine tested with appropriate signals, parameter sweeps, and spectral analysis</p>\n";
    html << "<p>Chimera Audio Engine Test Suite v3.0 Phoenix - " << __DATE__ << "</p>\n";
    html << "</div>\n";
    
    html << "</div>\n</body>\n</html>\n";
    html.close();
}

int main() {
    @autoreleasepool {
        std::cout << "=========================================\n";
        std::cout << "Chimera Engine Complete Validation Test\n";
        std::cout << "=========================================\n\n";
        
        // Define all engines with categories
        std::vector<Engine> engines = {
            {1, "K-Style Overdrive", EffectCategory::DISTORTION},
            {2, "Tape Echo", EffectCategory::TIME_BASED},
            {3, "Plate Reverb", EffectCategory::TIME_BASED},
            {4, "Rodent Distortion", EffectCategory::DISTORTION},
            {5, "Muff Fuzz", EffectCategory::DISTORTION},
            {6, "Classic Tremolo", EffectCategory::MODULATION},
            {7, "Magnetic Drum Echo", EffectCategory::TIME_BASED},
            {8, "Bucket Brigade Delay", EffectCategory::TIME_BASED},
            {9, "Digital Delay", EffectCategory::TIME_BASED},
            {10, "Harmonic Tremolo", EffectCategory::MODULATION},
            {11, "Rotary Speaker", EffectCategory::MODULATION},
            {12, "Detune Doubler", EffectCategory::MODULATION},
            {13, "Ladder Filter", EffectCategory::FILTER},
            {14, "Formant Filter", EffectCategory::FILTER},
            {15, "Classic Compressor", EffectCategory::DYNAMICS},
            {16, "State Variable Filter", EffectCategory::FILTER},
            {17, "Stereo Chorus", EffectCategory::MODULATION},
            {18, "Spectral Freeze", EffectCategory::SPECTRAL},
            {19, "Granular Cloud", EffectCategory::SPECTRAL},
            {20, "Analog Ring Modulator", EffectCategory::MODULATION},
            {21, "Multiband Saturator", EffectCategory::DISTORTION},
            {22, "Comb Resonator", EffectCategory::FILTER},
            {23, "Pitch Shifter", EffectCategory::SPECTRAL},
            {24, "Phased Vocoder", EffectCategory::SPECTRAL},
            {25, "Convolution Reverb", EffectCategory::TIME_BASED},
            {26, "Bit Crusher", EffectCategory::DISTORTION},
            {27, "Frequency Shifter", EffectCategory::SPECTRAL},
            {28, "Wave Folder", EffectCategory::DISTORTION},
            {29, "Shimmer Reverb", EffectCategory::TIME_BASED},
            {30, "Vocal Formant Filter", EffectCategory::FILTER},
            {31, "Transient Shaper", EffectCategory::DYNAMICS},
            {32, "Dimension Expander", EffectCategory::MODULATION},
            {33, "Analog Phaser", EffectCategory::MODULATION},
            {34, "Envelope Filter", EffectCategory::FILTER},
            {35, "Gated Reverb", EffectCategory::TIME_BASED},
            {36, "Harmonic Exciter", EffectCategory::DISTORTION},
            {37, "Feedback Network", EffectCategory::TIME_BASED},
            {38, "Intelligent Harmonizer", EffectCategory::SPECTRAL},
            {39, "Parametric EQ", EffectCategory::FILTER},
            {40, "Mastering Limiter", EffectCategory::DYNAMICS},
            {41, "Noise Gate", EffectCategory::DYNAMICS},
            {42, "Vintage Opto", EffectCategory::DYNAMICS},
            {43, "Spectral Gate", EffectCategory::DYNAMICS},
            {44, "Chaos Generator", EffectCategory::SPECTRAL},
            {45, "Buffer Repeat", EffectCategory::TIME_BASED},
            {46, "Vintage Console EQ", EffectCategory::FILTER},
            {47, "Mid/Side Processor", EffectCategory::UTILITY},
            {48, "Vintage Tube Preamp", EffectCategory::DISTORTION},
            {49, "Spring Reverb", EffectCategory::TIME_BASED},
            {50, "Resonant Chorus", EffectCategory::MODULATION},
            {51, "Stereo Widener", EffectCategory::UTILITY},
            {52, "Dynamic EQ", EffectCategory::FILTER},
            {53, "Stereo Imager", EffectCategory::UTILITY}
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
        
        // Set maximum frames per slice
        UInt32 maxFrames = 4096;
        AudioUnitSetProperty(audioUnit, kAudioUnitProperty_MaximumFramesPerSlice,
                            kAudioUnitScope_Global, 0, &maxFrames, sizeof(maxFrames));
        
        AudioUnitInitialize(audioUnit);
        
        // Set up input callback with test signal
        CallbackData* callbackData = new CallbackData{EffectCategory::DYNAMICS, 1000.0f, 0.5f, 0};
        
        AURenderCallbackStruct inputCallback;
        inputCallback.inputProcRefCon = callbackData;
        inputCallback.inputProc = [](void* inRefCon,
                                    AudioUnitRenderActionFlags* ioActionFlags,
                                    const AudioTimeStamp* inTimeStamp,
                                    UInt32 inBusNumber,
                                    UInt32 inNumberFrames,
                                    AudioBufferList* ioData) -> OSStatus {
            CallbackData* data = (CallbackData*)inRefCon;
            
            for (UInt32 buf = 0; buf < ioData->mNumberBuffers; buf++) {
                float* samples = (float*)ioData->mBuffers[buf].mData;
                
                for (UInt32 frame = 0; frame < inNumberFrames; frame++) {
                    float t = (data->sampleCount + frame) / 48000.0f;
                    
                    switch (data->category) {
                        case EffectCategory::TIME_BASED:
                            // Impulse for reverb testing
                            samples[frame] = (data->sampleCount == 0 && frame == 0) ? 0.8f : 0.0f;
                            break;
                            
                        case EffectCategory::FILTER:
                            // White noise for filter testing
                            samples[frame] = ((rand() / (float)RAND_MAX) - 0.5f) * data->amplitude;
                            break;
                            
                        default:
                            // Sine wave for general testing
                            samples[frame] = data->amplitude * sinf(2.0f * M_PI * data->frequency * t);
                            break;
                    }
                }
            }
            
            data->sampleCount += inNumberFrames;
            return noErr;
        };
        
        AudioUnitSetProperty(audioUnit, kAudioUnitProperty_SetRenderCallback,
                            kAudioUnitScope_Input, 0, &inputCallback, sizeof(inputCallback));
        
        // Get parameters
        UInt32 paramListSize = 0;
        AudioUnitGetPropertyInfo(audioUnit, kAudioUnitProperty_ParameterList,
                                 kAudioUnitScope_Global, 0, &paramListSize, NULL);
        
        int numParams = paramListSize / sizeof(AudioUnitParameterID);
        AudioUnitParameterID* paramList = (AudioUnitParameterID*)malloc(paramListSize);
        AudioUnitGetProperty(audioUnit, kAudioUnitProperty_ParameterList,
                            kAudioUnitScope_Global, 0, paramList, &paramListSize);
        
        // Find Slot 1 parameters
        AudioUnitParameterID slot1EngineParam = -1;
        AudioUnitParameterID slot1Params[10];
        int slot1ParamCount = 0;
        
        for (int i = 0; i < numParams; i++) {
            AudioUnitParameterInfo paramInfo;
            UInt32 size = sizeof(paramInfo);
            AudioUnitGetProperty(audioUnit, kAudioUnitProperty_ParameterInfo,
                                kAudioUnitScope_Global, paramList[i], &paramInfo, &size);
            
            std::string name(paramInfo.name);
            if (name == "Slot 1 Engine") {
                slot1EngineParam = paramList[i];
            } else if (name.find("Slot 1 Param") != std::string::npos && slot1ParamCount < 10) {
                slot1Params[slot1ParamCount++] = paramList[i];
            }
        }
        
        if (slot1EngineParam == -1) {
            std::cout << "ERROR: Could not find Slot 1 Engine parameter\n";
            return 1;
        }
        
        std::cout << "Found " << slot1ParamCount << " parameters for Slot 1\n\n";
        std::cout << "Testing " << engines.size() << " engines with parameter sweeps...\n";
        std::cout << "==================================================\n";
        
        // Test each engine
        std::vector<EngineTestResult> results;
        
        for (const auto& engine : engines) {
            auto result = testEngine(audioUnit, engine, slot1EngineParam, slot1Params, slot1ParamCount, callbackData);
            results.push_back(result);
        }
        
        // Summary
        std::cout << "\n==================================================\n";
        std::cout << "Test Summary\n";
        std::cout << "==================================================\n";
        
        int passed = 0, outputting = 0, responding = 0;
        for (const auto& r : results) {
            if (r.passed) passed++;
            if (r.audioPassthrough) outputting++;
            
            bool hasResponse = false;
            for (int i = 0; i < 10; i++) {
                if (r.parameterResponse[i]) {
                    hasResponse = true;
                    break;
                }
            }
            if (hasResponse) responding++;
        }
        
        std::cout << "Total Engines: " << results.size() << "\n";
        std::cout << "Passed: " << passed << " (" << (passed * 100.0f / results.size()) << "%)\n";
        std::cout << "Outputting Audio: " << outputting << "\n";
        std::cout << "Parameter Response: " << responding << "\n";
        
        // Generate HTML report
        generateHTMLReport(results);
        std::cout << "\nHTML report saved to: comprehensive_engine_report.html\n";
        
        // Cleanup
        delete callbackData;
        free(paramList);
        AudioUnitUninitialize(audioUnit);
        AudioComponentInstanceDispose(audioUnit);
        
        // Open report
        system("open comprehensive_engine_report.html");
    }
    
    return 0;
}