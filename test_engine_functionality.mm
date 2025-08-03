// Comprehensive functional testing for Chimera engines
// Tests that engines actually process audio correctly
#import <Foundation/Foundation.h>
#import <AudioToolbox/AudioToolbox.h>
#import <Accelerate/Accelerate.h>
#include <iostream>
#include <vector>
#include <iomanip>
#include <fstream>
#include <cmath>
#include <complex>

// FFT Analysis for frequency response testing
class FFTAnalyzer {
public:
    FFTAnalyzer(size_t size) : fftSize(size), fftSetup(nullptr) {
        log2n = log2(fftSize);
        fftSetup = vDSP_create_fftsetup(log2n, kFFTRadix2);
        realp.realp = (float*)malloc(fftSize/2 * sizeof(float));
        realp.imagp = (float*)malloc(fftSize/2 * sizeof(float));
    }
    
    ~FFTAnalyzer() {
        if (fftSetup) vDSP_destroy_fftsetup(fftSetup);
        free(realp.realp);
        free(realp.imagp);
    }
    
    std::vector<float> getMagnitudeSpectrum(const float* signal) {
        // Window the signal
        std::vector<float> windowed(fftSize);
        for (size_t i = 0; i < fftSize; i++) {
            float window = 0.5f - 0.5f * cosf(2.0f * M_PI * i / (fftSize - 1)); // Hann window
            windowed[i] = signal[i] * window;
        }
        
        // Pack for FFT
        vDSP_ctoz((DSPComplex*)windowed.data(), 2, &realp, 1, fftSize/2);
        
        // Perform FFT
        vDSP_fft_zrip(fftSetup, &realp, 1, log2n, kFFTDirection_Forward);
        
        // Calculate magnitude
        std::vector<float> magnitude(fftSize/2);
        vDSP_zvabs(&realp, 1, magnitude.data(), 1, fftSize/2);
        
        // Normalize
        float scale = 2.0f / fftSize;
        vDSP_vsmul(magnitude.data(), 1, &scale, magnitude.data(), 1, fftSize/2);
        
        return magnitude;
    }
    
private:
    size_t fftSize;
    vDSP_Length log2n;
    FFTSetup fftSetup;
    DSPSplitComplex realp;
};

// Test signal generators
void generateSineWave(float* buffer, int samples, float frequency, float sampleRate, float amplitude = 0.5f) {
    for (int i = 0; i < samples; i++) {
        buffer[i] = amplitude * sinf(2.0f * M_PI * frequency * i / sampleRate);
    }
}

void generateWhiteNoise(float* buffer, int samples, float amplitude = 0.5f) {
    for (int i = 0; i < samples; i++) {
        buffer[i] = amplitude * ((rand() / (float)RAND_MAX) * 2.0f - 1.0f);
    }
}

void generateImpulse(float* buffer, int samples, float amplitude = 1.0f) {
    memset(buffer, 0, samples * sizeof(float));
    buffer[0] = amplitude;
}

void generateSweep(float* buffer, int samples, float startFreq, float endFreq, float sampleRate, float amplitude = 0.5f) {
    for (int i = 0; i < samples; i++) {
        float t = i / sampleRate;
        float freq = startFreq + (endFreq - startFreq) * t * sampleRate / samples;
        float phase = 2.0f * M_PI * (startFreq * t + (endFreq - startFreq) * t * t / (2.0f * samples / sampleRate));
        buffer[i] = amplitude * sinf(phase);
    }
}

// Analysis functions
float calculateRMS(const float* buffer, int samples) {
    float sum = 0;
    for (int i = 0; i < samples; i++) {
        sum += buffer[i] * buffer[i];
    }
    return sqrtf(sum / samples);
}

float calculatePeak(const float* buffer, int samples) {
    float peak = 0;
    for (int i = 0; i < samples; i++) {
        float abs = fabsf(buffer[i]);
        if (abs > peak) peak = abs;
    }
    return peak;
}

float calculateTHD(const float* buffer, int samples, float fundamentalFreq, float sampleRate) {
    FFTAnalyzer fft(samples);
    auto spectrum = fft.getMagnitudeSpectrum(buffer);
    
    int fundamentalBin = fundamentalFreq * samples / sampleRate;
    float fundamental = spectrum[fundamentalBin];
    
    float harmonicsSum = 0;
    for (int h = 2; h <= 5; h++) {
        int bin = h * fundamentalBin;
        if (bin < spectrum.size()) {
            harmonicsSum += spectrum[bin] * spectrum[bin];
        }
    }
    
    return sqrtf(harmonicsSum) / fundamental * 100.0f; // THD in percentage
}

bool detectSignalPresence(const float* buffer, int samples, float threshold = 0.001f) {
    return calculateRMS(buffer, samples) > threshold;
}

// Engine-specific test functions
struct EngineTestResult {
    std::string engineName;
    bool processesAudio = false;
    bool passesSilenceTest = false;
    bool correctFrequencyResponse = false;
    bool correctDynamicResponse = false;
    bool parameterResponse = false;
    float inputRMS = 0;
    float outputRMS = 0;
    float gainChange = 0;
    float thd = 0;
    float latency = 0;
    std::string notes;
};

EngineTestResult testCompressor(AudioUnit audioUnit, AudioUnitParameterID engineParam, int engineIndex) {
    EngineTestResult result;
    result.engineName = "Compressor";
    
    // Set to compressor engine
    AudioUnitSetParameter(audioUnit, engineParam, kAudioUnitScope_Global, 0, engineIndex, 0);
    
    // Test 1: Process loud signal - should compress
    float testSignal[1024];
    generateSineWave(testSignal, 1024, 1000, 48000, 0.9f); // Loud signal
    
    AudioBufferList bufferList;
    bufferList.mNumberBuffers = 2;
    float leftData[1024], rightData[1024];
    memcpy(leftData, testSignal, 1024 * sizeof(float));
    memcpy(rightData, testSignal, 1024 * sizeof(float));
    
    bufferList.mBuffers[0].mNumberChannels = 1;
    bufferList.mBuffers[0].mDataByteSize = 1024 * sizeof(float);
    bufferList.mBuffers[0].mData = leftData;
    bufferList.mBuffers[1].mNumberChannels = 1;
    bufferList.mBuffers[1].mDataByteSize = 1024 * sizeof(float);
    bufferList.mBuffers[1].mData = rightData;
    
    AudioUnitRenderActionFlags flags = 0;
    AudioTimeStamp timeStamp = {0};
    timeStamp.mFlags = kAudioTimeStampSampleTimeValid;
    
    AudioUnitRender(audioUnit, &flags, &timeStamp, 0, 1024, &bufferList);
    
    result.inputRMS = calculateRMS(testSignal, 1024);
    result.outputRMS = calculateRMS(leftData, 1024);
    result.gainChange = 20 * log10f(result.outputRMS / result.inputRMS);
    
    // Compressor should reduce level of loud signals
    result.correctDynamicResponse = (result.gainChange < -1.0f);
    result.processesAudio = detectSignalPresence(leftData, 1024);
    
    if (result.correctDynamicResponse) {
        result.notes = "Compression detected (gain reduction: " + std::to_string(result.gainChange) + " dB)";
    } else {
        result.notes = "No compression detected";
    }
    
    return result;
}

EngineTestResult testFilter(AudioUnit audioUnit, AudioUnitParameterID engineParam, int engineIndex, const std::string& filterName) {
    EngineTestResult result;
    result.engineName = filterName;
    
    AudioUnitSetParameter(audioUnit, engineParam, kAudioUnitScope_Global, 0, engineIndex, 0);
    
    // Test with white noise to see frequency response
    float testSignal[4096];
    generateWhiteNoise(testSignal, 4096, 0.5f);
    
    AudioBufferList bufferList;
    bufferList.mNumberBuffers = 2;
    float leftData[4096], rightData[4096];
    memcpy(leftData, testSignal, 4096 * sizeof(float));
    memcpy(rightData, testSignal, 4096 * sizeof(float));
    
    bufferList.mBuffers[0].mNumberChannels = 1;
    bufferList.mBuffers[0].mDataByteSize = 4096 * sizeof(float);
    bufferList.mBuffers[0].mData = leftData;
    bufferList.mBuffers[1].mNumberChannels = 1;
    bufferList.mBuffers[1].mDataByteSize = 4096 * sizeof(float);
    bufferList.mBuffers[1].mData = rightData;
    
    AudioUnitRenderActionFlags flags = 0;
    AudioTimeStamp timeStamp = {0};
    timeStamp.mFlags = kAudioTimeStampSampleTimeValid;
    
    AudioUnitRender(audioUnit, &flags, &timeStamp, 0, 4096, &bufferList);
    
    // Analyze frequency response
    FFTAnalyzer fft(4096);
    auto inputSpectrum = fft.getMagnitudeSpectrum(testSignal);
    auto outputSpectrum = fft.getMagnitudeSpectrum(leftData);
    
    // Check if filter is affecting frequency content
    float spectralDifference = 0;
    for (int i = 10; i < 2048; i++) { // Skip DC and very low frequencies
        float diff = fabsf(outputSpectrum[i] - inputSpectrum[i]);
        spectralDifference += diff;
    }
    
    result.processesAudio = detectSignalPresence(leftData, 4096);
    result.correctFrequencyResponse = (spectralDifference > 10.0f); // Significant spectral change
    
    if (result.correctFrequencyResponse) {
        result.notes = "Filter is modifying frequency content";
    } else {
        result.notes = "No frequency modification detected";
    }
    
    return result;
}

EngineTestResult testReverb(AudioUnit audioUnit, AudioUnitParameterID engineParam, int engineIndex, const std::string& reverbName) {
    EngineTestResult result;
    result.engineName = reverbName;
    
    AudioUnitSetParameter(audioUnit, engineParam, kAudioUnitScope_Global, 0, engineIndex, 0);
    
    // Test with impulse to check for reverb tail
    float testSignal[8192];
    generateImpulse(testSignal, 8192, 0.8f);
    
    AudioBufferList bufferList;
    bufferList.mNumberBuffers = 2;
    float leftData[8192], rightData[8192];
    memcpy(leftData, testSignal, 8192 * sizeof(float));
    memcpy(rightData, testSignal, 8192 * sizeof(float));
    
    bufferList.mBuffers[0].mNumberChannels = 1;
    bufferList.mBuffers[0].mDataByteSize = 8192 * sizeof(float);
    bufferList.mBuffers[0].mData = leftData;
    bufferList.mBuffers[1].mNumberChannels = 1;
    bufferList.mBuffers[1].mDataByteSize = 8192 * sizeof(float);
    bufferList.mBuffers[1].mData = rightData;
    
    AudioUnitRenderActionFlags flags = 0;
    AudioTimeStamp timeStamp = {0};
    timeStamp.mFlags = kAudioTimeStampSampleTimeValid;
    
    AudioUnitRender(audioUnit, &flags, &timeStamp, 0, 8192, &bufferList);
    
    // Check for reverb tail - signal should continue after impulse
    float tailEnergy = calculateRMS(leftData + 1000, 7192); // Check energy after initial impulse
    float initialEnergy = fabsf(leftData[0]);
    
    result.processesAudio = detectSignalPresence(leftData, 8192);
    result.correctDynamicResponse = (tailEnergy > 0.001f); // Reverb tail present
    
    // Calculate rough RT60 (time for 60dB decay)
    int rt60Samples = 0;
    float threshold = initialEnergy * 0.001f; // -60dB
    for (int i = 1; i < 8192; i++) {
        if (fabsf(leftData[i]) < threshold) {
            rt60Samples = i;
            break;
        }
    }
    
    float rt60 = rt60Samples / 48000.0f;
    
    if (result.correctDynamicResponse) {
        result.notes = "Reverb tail detected (RT60: ~" + std::to_string(rt60) + "s)";
    } else {
        result.notes = "No reverb tail detected";
    }
    
    return result;
}

EngineTestResult testDistortion(AudioUnit audioUnit, AudioUnitParameterID engineParam, int engineIndex, const std::string& distName) {
    EngineTestResult result;
    result.engineName = distName;
    
    AudioUnitSetParameter(audioUnit, engineParam, kAudioUnitScope_Global, 0, engineIndex, 0);
    
    // Test with sine wave to measure THD
    float testSignal[2048];
    generateSineWave(testSignal, 2048, 1000, 48000, 0.5f);
    
    AudioBufferList bufferList;
    bufferList.mNumberBuffers = 2;
    float leftData[2048], rightData[2048];
    memcpy(leftData, testSignal, 2048 * sizeof(float));
    memcpy(rightData, testSignal, 2048 * sizeof(float));
    
    bufferList.mBuffers[0].mNumberChannels = 1;
    bufferList.mBuffers[0].mDataByteSize = 2048 * sizeof(float);
    bufferList.mBuffers[0].mData = leftData;
    bufferList.mBuffers[1].mNumberChannels = 1;
    bufferList.mBuffers[1].mDataByteSize = 2048 * sizeof(float);
    bufferList.mBuffers[1].mData = rightData;
    
    AudioUnitRenderActionFlags flags = 0;
    AudioTimeStamp timeStamp = {0};
    timeStamp.mFlags = kAudioTimeStampSampleTimeValid;
    
    AudioUnitRender(audioUnit, &flags, &timeStamp, 0, 2048, &bufferList);
    
    // Calculate THD
    result.thd = calculateTHD(leftData, 2048, 1000, 48000);
    result.processesAudio = detectSignalPresence(leftData, 2048);
    result.correctDynamicResponse = (result.thd > 1.0f); // Distortion should add harmonics
    
    if (result.correctDynamicResponse) {
        result.notes = "Distortion detected (THD: " + std::to_string(result.thd) + "%)";
    } else {
        result.notes = "No harmonic distortion detected";
    }
    
    return result;
}

EngineTestResult testModulation(AudioUnit audioUnit, AudioUnitParameterID engineParam, int engineIndex, const std::string& modName) {
    EngineTestResult result;
    result.engineName = modName;
    
    AudioUnitSetParameter(audioUnit, engineParam, kAudioUnitScope_Global, 0, engineIndex, 0);
    
    // Test with steady tone to detect modulation
    float testSignal[4096];
    generateSineWave(testSignal, 4096, 440, 48000, 0.5f);
    
    AudioBufferList bufferList;
    bufferList.mNumberBuffers = 2;
    float leftData[4096], rightData[4096];
    memcpy(leftData, testSignal, 4096 * sizeof(float));
    memcpy(rightData, testSignal, 4096 * sizeof(float));
    
    bufferList.mBuffers[0].mNumberChannels = 1;
    bufferList.mBuffers[0].mDataByteSize = 4096 * sizeof(float);
    bufferList.mBuffers[0].mData = leftData;
    bufferList.mBuffers[1].mNumberChannels = 1;
    bufferList.mBuffers[1].mDataByteSize = 4096 * sizeof(float);
    bufferList.mBuffers[1].mData = rightData;
    
    AudioUnitRenderActionFlags flags = 0;
    AudioTimeStamp timeStamp = {0};
    timeStamp.mFlags = kAudioTimeStampSampleTimeValid;
    
    AudioUnitRender(audioUnit, &flags, &timeStamp, 0, 4096, &bufferList);
    
    // Check for amplitude modulation by analyzing envelope variation
    float maxLevel = 0, minLevel = 1.0f;
    int windowSize = 128;
    for (int i = 0; i < 4096 - windowSize; i += windowSize) {
        float windowRMS = calculateRMS(leftData + i, windowSize);
        if (windowRMS > maxLevel) maxLevel = windowRMS;
        if (windowRMS < minLevel) minLevel = windowRMS;
    }
    
    float modDepth = (maxLevel - minLevel) / (maxLevel + minLevel);
    
    result.processesAudio = detectSignalPresence(leftData, 4096);
    result.correctDynamicResponse = (modDepth > 0.05f); // 5% modulation depth
    
    if (result.correctDynamicResponse) {
        result.notes = "Modulation detected (depth: " + std::to_string(modDepth * 100) + "%)";
    } else {
        result.notes = "No modulation detected";
    }
    
    return result;
}

void generateHTMLReport(const std::vector<EngineTestResult>& results) {
    std::ofstream html("functional_test_report.html");
    
    html << "<!DOCTYPE html><html><head><title>Chimera Engine Functional Test Report</title>";
    html << "<style>";
    html << "body { font-family: 'Segoe UI', Arial; margin: 20px; background: #1a1a2e; color: #eee; }";
    html << ".container { max-width: 1400px; margin: 0 auto; background: #16213e; border-radius: 15px; padding: 30px; }";
    html << "h1 { color: #0f4c75; text-align: center; padding: 20px; background: linear-gradient(135deg, #667eea, #764ba2); -webkit-background-clip: text; -webkit-text-fill-color: transparent; }";
    html << "table { width: 100%; border-collapse: collapse; margin: 20px 0; }";
    html << "th { background: #0f4c75; color: white; padding: 15px; text-align: left; }";
    html << "td { padding: 12px; border-bottom: 1px solid #2a2a3e; }";
    html << "tr:hover { background: #1f2940; }";
    html << ".pass { color: #4ade80; }";
    html << ".fail { color: #f87171; }";
    html << ".warning { color: #fbbf24; }";
    html << ".metric { font-family: monospace; color: #60a5fa; }";
    html << ".badge { display: inline-block; padding: 3px 8px; border-radius: 4px; font-size: 0.85em; margin-left: 5px; }";
    html << ".badge-pass { background: #065f46; color: #4ade80; }";
    html << ".badge-fail { background: #7f1d1d; color: #f87171; }";
    html << ".summary { display: grid; grid-template-columns: repeat(auto-fit, minmax(250px, 1fr)); gap: 20px; margin: 30px 0; }";
    html << ".stat-card { background: #0f4c75; padding: 20px; border-radius: 10px; text-align: center; }";
    html << ".stat-value { font-size: 2.5em; font-weight: bold; color: #60a5fa; }";
    html << ".stat-label { font-size: 0.9em; color: #94a3b8; text-transform: uppercase; margin-top: 5px; }";
    html << "</style></head><body>";
    
    html << "<div class='container'>";
    html << "<h1>🔬 Chimera Engine Functional Test Report</h1>";
    html << "<p style='text-align: center; color: #94a3b8;'>Testing actual audio processing functionality</p>";
    
    // Calculate summary stats
    int totalTests = results.size();
    int processing = 0, correctResponse = 0;
    float avgTHD = 0;
    int thdCount = 0;
    
    for (const auto& r : results) {
        if (r.processesAudio) processing++;
        if (r.correctDynamicResponse || r.correctFrequencyResponse) correctResponse++;
        if (r.thd > 0) {
            avgTHD += r.thd;
            thdCount++;
        }
    }
    
    if (thdCount > 0) avgTHD /= thdCount;
    
    // Summary cards
    html << "<div class='summary'>";
    html << "<div class='stat-card'><div class='stat-value'>" << totalTests << "</div><div class='stat-label'>Engines Tested</div></div>";
    html << "<div class='stat-card'><div class='stat-value'>" << processing << "</div><div class='stat-label'>Processing Audio</div></div>";
    html << "<div class='stat-card'><div class='stat-value'>" << correctResponse << "</div><div class='stat-label'>Correct Response</div></div>";
    html << "<div class='stat-card'><div class='stat-value'>" << std::fixed << std::setprecision(1) << avgTHD << "%</div><div class='stat-label'>Avg THD (Distortion)</div></div>";
    html << "</div>";
    
    // Detailed results
    html << "<h2>Detailed Functional Test Results</h2>";
    html << "<table>";
    html << "<tr>";
    html << "<th>Engine</th>";
    html << "<th>Processes Audio</th>";
    html << "<th>Correct Response</th>";
    html << "<th>Gain Change (dB)</th>";
    html << "<th>THD (%)</th>";
    html << "<th>Notes</th>";
    html << "</tr>";
    
    for (const auto& r : results) {
        html << "<tr>";
        html << "<td><strong>" << r.engineName << "</strong></td>";
        
        // Processes Audio
        html << "<td class='" << (r.processesAudio ? "pass" : "fail") << "'>";
        html << (r.processesAudio ? "✓ Yes" : "✗ No");
        html << "</td>";
        
        // Correct Response
        bool hasCorrectResponse = r.correctDynamicResponse || r.correctFrequencyResponse;
        html << "<td class='" << (hasCorrectResponse ? "pass" : "warning") << "'>";
        if (r.correctDynamicResponse) html << "✓ Dynamic";
        else if (r.correctFrequencyResponse) html << "✓ Frequency";
        else html << "⚠ None";
        html << "</td>";
        
        // Gain Change
        html << "<td class='metric'>";
        if (r.gainChange != 0) {
            html << std::fixed << std::setprecision(2) << r.gainChange;
            if (r.gainChange < -3) html << "<span class='badge badge-pass'>Compressed</span>";
            else if (r.gainChange > 3) html << "<span class='badge badge-pass'>Boosted</span>";
        } else {
            html << "-";
        }
        html << "</td>";
        
        // THD
        html << "<td class='metric'>";
        if (r.thd > 0) {
            html << std::fixed << std::setprecision(1) << r.thd;
            if (r.thd > 10) html << "<span class='badge badge-pass'>High</span>";
            else if (r.thd > 1) html << "<span class='badge badge-pass'>Moderate</span>";
        } else {
            html << "-";
        }
        html << "</td>";
        
        // Notes
        html << "<td style='color: #94a3b8; font-size: 0.9em;'>" << r.notes << "</td>";
        
        html << "</tr>";
    }
    
    html << "</table>";
    
    html << "<div style='margin-top: 40px; padding: 20px; background: #0f3460; border-radius: 10px;'>";
    html << "<h3>Test Methodology</h3>";
    html << "<ul style='color: #94a3b8;'>";
    html << "<li><strong>Compressors:</strong> Tested with loud signals (0.9 amplitude) to verify gain reduction</li>";
    html << "<li><strong>Filters:</strong> Tested with white noise to analyze frequency response changes</li>";
    html << "<li><strong>Reverbs:</strong> Tested with impulse to detect reverb tail and measure decay time</li>";
    html << "<li><strong>Distortion:</strong> Tested with sine waves to measure Total Harmonic Distortion (THD)</li>";
    html << "<li><strong>Modulation:</strong> Tested with steady tones to detect amplitude/frequency modulation</li>";
    html << "</ul>";
    html << "</div>";
    
    html << "</div></body></html>";
    html.close();
}

int main() {
    @autoreleasepool {
        std::cout << "=========================================\n";
        std::cout << "Chimera Engine Functional Testing\n";
        std::cout << "=========================================\n\n";
        
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
        
        // Test specific engines with appropriate test methods
        std::vector<EngineTestResult> results;
        
        std::cout << "Testing engine functionality...\n";
        std::cout << "-------------------------------\n";
        
        // Test compressors
        std::cout << "Testing Classic Compressor... ";
        results.push_back(testCompressor(audioUnit, slot1EngineParam, 15));
        std::cout << (results.back().correctDynamicResponse ? "✓" : "✗") << "\n";
        
        // Test filters
        std::cout << "Testing Ladder Filter... ";
        results.push_back(testFilter(audioUnit, slot1EngineParam, 13, "Ladder Filter"));
        std::cout << (results.back().correctFrequencyResponse ? "✓" : "✗") << "\n";
        
        std::cout << "Testing State Variable Filter... ";
        results.push_back(testFilter(audioUnit, slot1EngineParam, 16, "State Variable Filter"));
        std::cout << (results.back().correctFrequencyResponse ? "✓" : "✗") << "\n";
        
        // Test reverbs
        std::cout << "Testing Plate Reverb... ";
        results.push_back(testReverb(audioUnit, slot1EngineParam, 3, "Plate Reverb"));
        std::cout << (results.back().correctDynamicResponse ? "✓" : "✗") << "\n";
        
        std::cout << "Testing Shimmer Reverb... ";
        results.push_back(testReverb(audioUnit, slot1EngineParam, 29, "Shimmer Reverb"));
        std::cout << (results.back().correctDynamicResponse ? "✓" : "✗") << "\n";
        
        // Test distortion
        std::cout << "Testing K-Style Overdrive... ";
        results.push_back(testDistortion(audioUnit, slot1EngineParam, 1, "K-Style Overdrive"));
        std::cout << (results.back().correctDynamicResponse ? "✓" : "✗") << "\n";
        
        std::cout << "Testing Muff Fuzz... ";
        results.push_back(testDistortion(audioUnit, slot1EngineParam, 5, "Muff Fuzz"));
        std::cout << (results.back().correctDynamicResponse ? "✓" : "✗") << "\n";
        
        std::cout << "Testing Bit Crusher... ";
        results.push_back(testDistortion(audioUnit, slot1EngineParam, 26, "Bit Crusher"));
        std::cout << (results.back().correctDynamicResponse ? "✓" : "✗") << "\n";
        
        // Test modulation
        std::cout << "Testing Classic Tremolo... ";
        results.push_back(testModulation(audioUnit, slot1EngineParam, 6, "Classic Tremolo"));
        std::cout << (results.back().correctDynamicResponse ? "✓" : "✗") << "\n";
        
        std::cout << "Testing Stereo Chorus... ";
        results.push_back(testModulation(audioUnit, slot1EngineParam, 17, "Stereo Chorus"));
        std::cout << (results.back().correctDynamicResponse ? "✓" : "✗") << "\n";
        
        std::cout << "Testing Analog Phaser... ";
        results.push_back(testModulation(audioUnit, slot1EngineParam, 33, "Analog Phaser"));
        std::cout << (results.back().correctDynamicResponse ? "✓" : "✗") << "\n";
        
        // Reset to bypass
        AudioUnitSetParameter(audioUnit, slot1EngineParam, kAudioUnitScope_Global, 0, 0, 0);
        
        // Cleanup
        free(paramList);
        AudioUnitUninitialize(audioUnit);
        AudioComponentInstanceDispose(audioUnit);
        
        // Summary
        std::cout << "\n=========================================\n";
        std::cout << "Functional Test Complete\n";
        std::cout << "=========================================\n";
        
        int functioning = 0;
        for (const auto& r : results) {
            if (r.correctDynamicResponse || r.correctFrequencyResponse) functioning++;
        }
        
        std::cout << "Engines tested: " << results.size() << "\n";
        std::cout << "Functioning correctly: " << functioning << "\n";
        std::cout << "Pass rate: " << (functioning * 100.0f / results.size()) << "%\n";
        
        // Generate HTML report
        generateHTMLReport(results);
        std::cout << "\nHTML report saved to: functional_test_report.html\n";
        
        // Open report
        system("open functional_test_report.html");
    }
    
    return 0;
}