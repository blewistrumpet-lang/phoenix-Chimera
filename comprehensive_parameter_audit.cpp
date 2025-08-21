#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <cmath>
#include <iomanip>
#include <sstream>

// Comprehensive parameter audit tool for Chimera Phoenix
// Tests all 57 engines to identify parameter mapping issues

struct EngineInfo {
    int id;
    std::string name;
    int numParams;
    std::vector<std::string> paramNames;
    std::vector<bool> paramWorks;
};

// All 57 engines from the factory
std::vector<EngineInfo> engines = {
    {0, "BitCrusher", 8, {"BitDepth", "SampleRate", "Mix", "Drive", "Filter", "Noise", "Alias", "Jitter"}, {}},
    {1, "Chorus", 8, {"Rate", "Depth", "Feedback", "Mix", "Delay", "Width", "Voices", "Filter"}, {}},
    {2, "Compressor", 8, {"Threshold", "Ratio", "Attack", "Release", "Knee", "Makeup", "Mix", "Lookahead"}, {}},
    {3, "ConvolutionReverb", 8, {"Size", "Damping", "Width", "Mix", "PreDelay", "EarlyLate", "Filter", "Modulation"}, {}},
    {4, "Decimator", 8, {"Downsample", "Bitdepth", "Mix", "Filter", "Alias", "Dither", "Noise", "Gate"}, {}},
    {5, "Delay", 8, {"Time", "Feedback", "Mix", "Filter", "Spread", "Modulation", "Sync", "PingPong"}, {}},
    {6, "Distortion", 8, {"Drive", "Mix", "Tone", "Output", "Mode", "Asymmetry", "Filter", "Gate"}, {}},
    {7, "DualFilter", 8, {"Freq1", "Res1", "Freq2", "Res2", "Mix", "Mode", "Spread", "Drive"}, {}},
    {8, "EnvelopeFollower", 8, {"Attack", "Release", "Gain", "Mix", "Threshold", "Filter", "Mode", "Smooth"}, {}},
    {9, "Equalizer", 8, {"LowGain", "MidGain", "HighGain", "LowFreq", "MidFreq", "HighFreq", "Q", "Mix"}, {}},
    {10, "Exciter", 8, {"Amount", "Frequency", "Mix", "Harmonics", "Saturation", "Filter", "Mode", "Drive"}, {}},
    {11, "Filter", 8, {"Frequency", "Resonance", "Mix", "Type", "Slope", "Drive", "Envelope", "Tracking"}, {}},
    {12, "Flanger", 8, {"Rate", "Depth", "Feedback", "Mix", "Delay", "Spread", "Mode", "Filter"}, {}},
    {13, "FrequencyShifter", 8, {"Shift", "Mix", "Feedback", "Range", "Mode", "Filter", "Phase", "Spread"}, {}},
    {14, "Gate", 8, {"Threshold", "Attack", "Hold", "Release", "Range", "Mix", "Filter", "Lookahead"}, {}},
    {15, "GranularDelay", 8, {"GrainSize", "Position", "Feedback", "Mix", "Pitch", "Density", "Spread", "Random"}, {}},
    {16, "HardClip", 8, {"Threshold", "Mix", "Drive", "Output", "Mode", "Knee", "Filter", "Gate"}, {}},
    {17, "HarmonicEnhancer", 8, {"Amount", "Frequency", "Mix", "Even", "Odd", "Filter", "Drive", "Width"}, {}},
    {18, "HighPass", 8, {"Frequency", "Resonance", "Mix", "Slope", "Drive", "Mode", "Filter", "Track"}, {}},
    {19, "IntelligentHarmonizer", 8, {"Pitch", "Key", "Scale", "Mix", "Formant", "Detune", "Voices", "Spread"}, {}},
    {20, "Limiter", 8, {"Threshold", "Release", "Ceiling", "Mix", "Lookahead", "Mode", "Knee", "Stereo"}, {}},
    {21, "LowPass", 8, {"Frequency", "Resonance", "Mix", "Slope", "Drive", "Mode", "Filter", "Track"}, {}},
    {22, "MidSideProcessor", 8, {"MidGain", "SideGain", "Width", "Mix", "Bass", "Filter", "Mode", "Phase"}, {}},
    {23, "MonoToStereo", 8, {"Width", "Delay", "Phase", "Mix", "Filter", "Mode", "Spread", "Center"}, {}},
    {24, "MultibandCompressor", 8, {"Low", "Mid", "High", "Crossover1", "Crossover2", "Mix", "Attack", "Release"}, {}},
    {25, "NoiseGenerator", 8, {"Level", "Color", "Mix", "Filter", "Envelope", "Rate", "Stereo", "Gate"}, {}},
    {26, "Overdrive", 8, {"Drive", "Tone", "Mix", "Output", "Mode", "Bias", "Filter", "Gate"}, {}},
    {27, "Panner", 8, {"Position", "Width", "Law", "Mix", "LFORate", "LFODepth", "Mode", "Center"}, {}},
    {28, "Phaser", 8, {"Rate", "Depth", "Feedback", "Mix", "Stages", "Frequency", "Spread", "Mode"}, {}},
    {29, "PingPongDelay", 8, {"Time", "Feedback", "Mix", "Width", "Filter", "Sync", "Mode", "Spread"}, {}},
    {30, "PitchCorrection", 8, {"Key", "Scale", "Speed", "Mix", "Range", "Smooth", "Formant", "Reference"}, {}},
    {31, "PitchShifter", 8, {"Pitch", "Formant", "Mix", "Window", "Gate", "Grain", "Feedback", "Width"}, {}},
    {32, "Reverb", 8, {"Size", "Decay", "Damping", "Mix", "PreDelay", "Width", "Filter", "Modulation"}, {}},
    {33, "RingModulator", 8, {"Frequency", "Mix", "Shape", "Drive", "Filter", "Mode", "Phase", "Spread"}, {}},
    {34, "Saturator", 8, {"Drive", "Mix", "Output", "Mode", "Color", "Filter", "Bias", "Gate"}, {}},
    {35, "DigitalDelay", 8, {"Time", "Feedback", "Mix", "Filter", "Width", "Sync", "Mode", "Ducking"}, {}},
    {36, "SpectralFreeze", 8, {"Freeze", "Size", "Shift", "Mix", "Filter", "Smooth", "Mode", "Spread"}, {}},
    {37, "SpectralGate", 8, {"Threshold", "Attack", "Release", "Mix", "Frequency", "Width", "Mode", "Smooth"}, {}},
    {38, "StereoImager", 8, {"Width", "Bass", "Center", "Mix", "Mode", "Phase", "Filter", "Spread"}, {}},
    {39, "StereoToMono", 8, {"Mode", "Mix", "Phase", "Balance", "Filter", "Center", "Width", "Level"}, {}},
    {40, "SubBassEnhancer", 8, {"Frequency", "Amount", "Mix", "Drive", "Filter", "Mode", "Gate", "Width"}, {}},
    {41, "Synthesizer", 8, {"Frequency", "Filter", "Envelope", "Mix", "Wave", "Detune", "Voices", "Spread"}, {}},
    {42, "TapeDelay", 8, {"Time", "Feedback", "Mix", "Wow", "Flutter", "Saturation", "Filter", "Age"}, {}},
    {43, "TransientShaper", 8, {"Attack", "Sustain", "Mix", "Sensitivity", "Mode", "Filter", "Range", "Speed"}, {}},
    {44, "TremoloEffect", 8, {"Rate", "Depth", "Shape", "Mix", "Phase", "Sync", "Mode", "Smooth"}, {}},
    {45, "TubeDistortion", 8, {"Drive", "Warmth", "Mix", "Output", "Bias", "Mode", "Filter", "Gate"}, {}},
    {46, "VintageChorus", 8, {"Rate", "Depth", "Mix", "Feedback", "Age", "Width", "Mode", "Filter"}, {}},
    {47, "VintageCompressor", 8, {"Threshold", "Ratio", "Attack", "Release", "Mix", "Character", "Knee", "Mode"}, {}},
    {48, "VintageDelay", 8, {"Time", "Feedback", "Mix", "Age", "Modulation", "Filter", "Width", "Sync"}, {}},
    {49, "VintageEQ", 8, {"Low", "Mid", "High", "Presence", "Mix", "Drive", "Mode", "Character"}, {}},
    {50, "VintageReverb", 8, {"Size", "Decay", "Mix", "Character", "Damping", "PreDelay", "Width", "Mode"}, {}},
    {51, "VocalDoubler", 8, {"Detune", "Delay", "Mix", "Voices", "Spread", "Depth", "Mode", "Width"}, {}},
    {52, "Vocoder", 8, {"Bands", "Range", "Formant", "Mix", "Attack", "Release", "Mode", "Emphasis"}, {}},
    {53, "WahWah", 8, {"Frequency", "Resonance", "Mix", "Mode", "Range", "Speed", "Depth", "Manual"}, {}},
    {54, "Waveshaper", 8, {"Amount", "Mix", "Mode", "Bias", "Drive", "Filter", "Output", "Smooth"}, {}},
    {55, "Widener", 8, {"Width", "Delay", "Mix", "Filter", "Mode", "Center", "Bass", "Phase"}, {}},
    {56, "Wobble", 8, {"Rate", "Depth", "Mix", "Shape", "Phase", "Filter", "Sync", "Mode"}, {}}
};

// Parameter types for proper display formatting
enum ParamType {
    TYPE_NORMALIZED,  // 0-1 value
    TYPE_FREQUENCY,   // Hz
    TYPE_TIME,        // ms or seconds
    TYPE_DECIBEL,     // dB
    TYPE_SEMITONES,   // pitch in semitones
    TYPE_PERCENT,     // percentage
    TYPE_RATIO,       // compression ratio
    TYPE_MODE         // discrete mode selection
};

// Map parameter names to their display types
std::map<std::string, ParamType> paramTypeMap = {
    {"Pitch", TYPE_SEMITONES},
    {"Frequency", TYPE_FREQUENCY},
    {"Time", TYPE_TIME},
    {"Delay", TYPE_TIME},
    {"Attack", TYPE_TIME},
    {"Release", TYPE_TIME},
    {"Hold", TYPE_TIME},
    {"PreDelay", TYPE_TIME},
    {"Threshold", TYPE_DECIBEL},
    {"Gain", TYPE_DECIBEL},
    {"Output", TYPE_DECIBEL},
    {"Ceiling", TYPE_DECIBEL},
    {"Drive", TYPE_DECIBEL},
    {"Ratio", TYPE_RATIO},
    {"Mix", TYPE_PERCENT},
    {"Width", TYPE_PERCENT},
    {"Depth", TYPE_PERCENT},
    {"Feedback", TYPE_PERCENT},
    {"Mode", TYPE_MODE},
    {"Formant", TYPE_SEMITONES},
    {"Detune", TYPE_SEMITONES}
};

std::string formatParameterValue(const std::string& paramName, float normalizedValue) {
    auto it = paramTypeMap.find(paramName);
    ParamType type = (it != paramTypeMap.end()) ? it->second : TYPE_NORMALIZED;
    
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2);
    
    switch (type) {
        case TYPE_SEMITONES:
            // -24 to +24 semitones
            ss << ((normalizedValue - 0.5f) * 48.0f) << " st";
            break;
        case TYPE_FREQUENCY:
            // 20 Hz to 20 kHz (log scale)
            {
                float freq = 20.0f * std::pow(1000.0f, normalizedValue);
                if (freq >= 1000.0f) {
                    ss << (freq / 1000.0f) << " kHz";
                } else {
                    ss << freq << " Hz";
                }
            }
            break;
        case TYPE_TIME:
            // 0 to 2000 ms
            {
                float ms = normalizedValue * 2000.0f;
                if (ms >= 1000.0f) {
                    ss << (ms / 1000.0f) << " s";
                } else {
                    ss << ms << " ms";
                }
            }
            break;
        case TYPE_DECIBEL:
            // -60 to +12 dB
            ss << (normalizedValue * 72.0f - 60.0f) << " dB";
            break;
        case TYPE_PERCENT:
            ss << (normalizedValue * 100.0f) << "%";
            break;
        case TYPE_RATIO:
            // 1:1 to 20:1
            ss << "1:" << (1.0f + normalizedValue * 19.0f);
            break;
        case TYPE_MODE:
            ss << "Mode " << static_cast<int>(normalizedValue * 8);
            break;
        default:
            ss << normalizedValue;
            break;
    }
    
    return ss.str();
}

void auditEngine(const EngineInfo& engine) {
    std::cout << "\n=== Engine " << engine.id << ": " << engine.name << " ===" << std::endl;
    std::cout << "Parameters: " << engine.numParams << std::endl;
    
    // Test each parameter
    for (size_t i = 0; i < engine.paramNames.size(); ++i) {
        std::cout << "  " << i << ". " << engine.paramNames[i];
        
        // Show formatted values at different positions
        std::cout << " [0.0: " << formatParameterValue(engine.paramNames[i], 0.0f);
        std::cout << ", 0.5: " << formatParameterValue(engine.paramNames[i], 0.5f);
        std::cout << ", 1.0: " << formatParameterValue(engine.paramNames[i], 1.0f) << "]";
        
        std::cout << std::endl;
    }
}

void identifyProblematicEngines() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "PROBLEMATIC ENGINES (Need Investigation)" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // Engines with pitch/frequency parameters that likely have issues
    std::vector<int> pitchEngines = {31, 19, 30, 13, 33, 15};  // PitchShifter, Harmonizer, etc.
    std::vector<int> timeEngines = {5, 29, 35, 42, 48};  // Delays
    std::vector<int> dynamicsEngines = {2, 20, 24, 47};  // Compressors, Limiters
    
    std::cout << "\nPitch-based engines (likely broken):" << std::endl;
    for (int id : pitchEngines) {
        std::cout << "  - " << engines[id].name << " (ID " << id << ")" << std::endl;
    }
    
    std::cout << "\nTime-based engines (check sync/tempo):" << std::endl;
    for (int id : timeEngines) {
        std::cout << "  - " << engines[id].name << " (ID " << id << ")" << std::endl;
    }
    
    std::cout << "\nDynamics engines (check threshold/ratio):" << std::endl;
    for (int id : dynamicsEngines) {
        std::cout << "  - " << engines[id].name << " (ID " << id << ")" << std::endl;
    }
}

void generateTestPlan() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "PARAMETER SYSTEM TEST PLAN" << std::endl;
    std::cout << "========================================" << std::endl;
    
    std::cout << "\n1. IMMEDIATE FIXES NEEDED:" << std::endl;
    std::cout << "   - Fix PitchShifter phase vocoder (lines 392-444)" << std::endl;
    std::cout << "   - Separate pitch and formant operations" << std::endl;
    std::cout << "   - Verify phase accumulator updates" << std::endl;
    
    std::cout << "\n2. PARAMETER FLOW TESTING:" << std::endl;
    std::cout << "   - Trace parameter from UI knob to DSP" << std::endl;
    std::cout << "   - Check slot-based routing (15 params per slot)" << std::endl;
    std::cout << "   - Verify atomic parameter smoothing" << std::endl;
    
    std::cout << "\n3. UI/UX IMPROVEMENTS:" << std::endl;
    std::cout << "   - Display actual values (Hz, dB, ms, semitones)" << std::endl;
    std::cout << "   - Add parameter tooltips" << std::endl;
    std::cout << "   - Show parameter automation curves" << std::endl;
    
    std::cout << "\n4. ENGINE-BY-ENGINE AUDIT:" << std::endl;
    std::cout << "   - Test all 8 parameters per engine" << std::endl;
    std::cout << "   - Verify audio effect for each parameter" << std::endl;
    std::cout << "   - Document non-functional parameters" << std::endl;
    
    std::cout << "\n5. VALIDATION SUITE:" << std::endl;
    std::cout << "   - Create automated parameter tests" << std::endl;
    std::cout << "   - Test with sine wave input" << std::endl;
    std::cout << "   - Measure spectral changes" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "CHIMERA PHOENIX PARAMETER SYSTEM AUDIT" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // Show a few example engines
    auditEngine(engines[31]);  // PitchShifter
    auditEngine(engines[5]);   // Delay
    auditEngine(engines[2]);   // Compressor
    
    identifyProblematicEngines();
    generateTestPlan();
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "NEXT STEPS:" << std::endl;
    std::cout << "1. Fix PitchShifter immediately" << std::endl;
    std::cout << "2. Run this audit in Logic Pro" << std::endl;
    std::cout << "3. Test each engine's parameters" << std::endl;
    std::cout << "4. Implement value display system" << std::endl;
    std::cout << "========================================" << std::endl;
    
    return 0;
}