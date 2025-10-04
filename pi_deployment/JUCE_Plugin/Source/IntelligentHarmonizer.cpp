// IntelligentHarmonizer using SMBPitchShiftFixed
// Real-time pitch shifting for harmony generation with < 0.0005% frequency error

#include "IntelligentHarmonizer.h"
#include "IntelligentHarmonizerChords.h"
#include "SMBPitchShiftFixed.h"
#include <algorithm>
#include <cmath>
#include <memory>
#include <atomic>
#include <random>

namespace {

// Utilities
template<typename T>
inline T flushDenorm(T v) noexcept {
    constexpr T tiny = static_cast<T>(1.0e-38);
    return std::fabs(v) < tiny ? static_cast<T>(0) : v;
}

// Parameter smoothing
class SmoothedParam {
    std::atomic<float> target{0.0f};
    float current{0.0f};
    float coeff{0.9995f};
    
public:
    void setSmoothingTime(float timeMs, double sampleRate) noexcept {
        float samples = timeMs * 0.001f * sampleRate;
        coeff = std::exp(-1.0f / samples);
    }
    
    void set(float value) noexcept {
        target.store(value, std::memory_order_relaxed);
    }
    
    void snap(float value) noexcept {
        target.store(value, std::memory_order_relaxed);
        current = value;
    }
    
    float tick() noexcept {
        float t = target.load(std::memory_order_relaxed);
        current = t + coeff * (current - t);
        return current;
    }
    
    float get() const noexcept { return current; }
};

// Scale definitions
const std::vector<std::vector<int>> scales = {
    {0, 2, 4, 5, 7, 9, 11},           // Major
    {0, 2, 3, 5, 7, 8, 10},           // Natural Minor
    {0, 2, 3, 5, 7, 8, 11},           // Harmonic Minor
    {0, 2, 3, 5, 7, 9, 11},           // Melodic Minor
    {0, 2, 3, 5, 7, 9, 10},           // Dorian
    {0, 1, 3, 5, 7, 8, 10},           // Phrygian
    {0, 2, 4, 6, 7, 9, 11},           // Lydian
    {0, 2, 4, 5, 7, 9, 10},           // Mixolydian
    {0, 1, 3, 5, 6, 8, 10},           // Locrian
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11} // Chromatic
};

int quantizeToScale(int semitones, int scaleIndex, int /*key*/) {
    if (scaleIndex < 0 || scaleIndex >= scales.size()) {
        return semitones;
    }
    
    const auto& scale = scales[scaleIndex];
    if (scale.empty() || scaleIndex == 9) { // Chromatic
        return semitones;
    }
    
    int octave = semitones / 12;
    int chroma = ((semitones % 12) + 12) % 12;
    
    int minDist = 12;
    int closest = chroma;
    
    for (int note : scale) {
        int dist = std::abs(chroma - note);
        if (dist < minDist) {
            minDist = dist;
            closest = note;
        }
    }
    
    return octave * 12 + closest;
}

float intervalToRatio(int semitones) {
    return std::pow(2.0f, semitones / 12.0f);
}

} // namespace

// Implementation using Signalsmith Stretch
class IntelligentHarmonizer::Impl {
public:
    // SMB Pitch Shift engines (one per voice)
    std::array<std::unique_ptr<SMBPitchShiftFixed>, 3> pitchShifters_;
    
    // Parameters - Voice pitches
    SmoothedParam pitchRatio1_;
    SmoothedParam pitchRatio2_;
    SmoothedParam pitchRatio3_;
    
    // Parameters - Voice volumes
    SmoothedParam voice1Volume_;
    SmoothedParam voice2Volume_;
    SmoothedParam voice3Volume_;
    
    // Parameters - Voice formants
    SmoothedParam voice1Formant_;
    SmoothedParam voice2Formant_;
    SmoothedParam voice3Formant_;
    
    // Parameters - Global
    SmoothedParam masterMix_;
    SmoothedParam humanize_;
    SmoothedParam width_;
    
    // Settings
    int numVoices_ = 3;  // Default to 3 voices for full chords
    int chordIndex_ = 0;  // Major chord by default
    int rootKey_ = 0;     // C by default
    int scaleIndex_ = 9;  // Chromatic by default
    int transposeOctaves_ = 0;
    bool lowLatencyMode_ = false;  // Default to high-quality mode with SMBPitchShift
    
    // Engine state
    double sampleRate_ = 48000.0;
    int blockSize_ = 512;
    bool prepared_ = false;
    
    // Humanization
    std::mt19937 rng_{std::random_device{}()};
    std::uniform_real_distribution<float> pitchDist_{-0.02f, 0.02f};
    std::uniform_real_distribution<float> timeDist_{0.0f, 0.001f};
    
    // Processing buffers
    std::vector<float> inputBuffer_;
    std::vector<float> outputBuffer_;
    std::vector<float> delayBuffer_;
    int delayWritePos_ = 0;
    
    void processLowLatency(const float* input, float* output, int numSamples) {
        // Simple variable-speed playback for pitch shifting
        // Zero latency but with artifacts
        static float readPos1 = 0.0f;
        static float readPos2 = 0.0f;
        static float readPos3 = 0.0f;
        const int bufferSize = 4096;
        
        if (delayBuffer_.size() != bufferSize) {
            delayBuffer_.resize(bufferSize, 0.0f);
        }
        
        // Get current parameters
        float ratio1 = pitchRatio1_.tick();
        float ratio2 = pitchRatio2_.tick();
        float ratio3 = pitchRatio3_.tick();
        
        float vol1 = voice1Volume_.tick();
        float vol2 = voice2Volume_.tick();
        float vol3 = voice3Volume_.tick();
        
        float masterMix = masterMix_.tick();
        float humanizeAmt = humanize_.tick();
        
        for (int i = 0; i < numSamples; ++i) {
            // Write to circular buffer
            delayBuffer_[delayWritePos_] = input[i];
            delayWritePos_ = (delayWritePos_ + 1) % bufferSize;
            
            float wetSignal = 0.0f;
            
            // Voice 1
            if (numVoices_ >= 1 && vol1 > 0.01f) {
                // Add humanization
                float pitchMod = 1.0f;
                if (humanizeAmt > 0.01f) {
                    pitchMod = 1.0f + (pitchDist_(rng_) * humanizeAmt);
                }
                
                int readIdx = static_cast<int>(readPos1);
                float frac = readPos1 - readIdx;
                
                int idx0 = readIdx % bufferSize;
                int idx1 = (readIdx + 1) % bufferSize;
                
                float voice1 = delayBuffer_[idx0] * (1.0f - frac) + delayBuffer_[idx1] * frac;
                wetSignal += voice1 * vol1;
                
                readPos1 += (1.0f / ratio1) * pitchMod;
                if (readPos1 >= bufferSize) readPos1 -= bufferSize;
            }
            
            // Voice 2
            if (numVoices_ >= 2 && vol2 > 0.01f) {
                float pitchMod = 1.0f;
                if (humanizeAmt > 0.01f) {
                    pitchMod = 1.0f + (pitchDist_(rng_) * humanizeAmt * 0.7f);
                }
                
                int readIdx = static_cast<int>(readPos2);
                float frac = readPos2 - readIdx;
                
                int idx0 = readIdx % bufferSize;
                int idx1 = (readIdx + 1) % bufferSize;
                
                float voice2 = delayBuffer_[idx0] * (1.0f - frac) + delayBuffer_[idx1] * frac;
                wetSignal += voice2 * vol2;
                
                readPos2 += (1.0f / ratio2) * pitchMod;
                if (readPos2 >= bufferSize) readPos2 -= bufferSize;
            }
            
            // Voice 3
            if (numVoices_ >= 3 && vol3 > 0.01f) {
                float pitchMod = 1.0f;
                if (humanizeAmt > 0.01f) {
                    pitchMod = 1.0f + (pitchDist_(rng_) * humanizeAmt * 0.5f);
                }
                
                int readIdx = static_cast<int>(readPos3);
                float frac = readPos3 - readIdx;
                
                int idx0 = readIdx % bufferSize;
                int idx1 = (readIdx + 1) % bufferSize;
                
                float voice3 = delayBuffer_[idx0] * (1.0f - frac) + delayBuffer_[idx1] * frac;
                wetSignal += voice3 * vol3;
                
                readPos3 += (1.0f / ratio3) * pitchMod;
                if (readPos3 >= bufferSize) readPos3 -= bufferSize;
            }
            
            // Mix dry and wet
            output[i] = input[i] * (1.0f - masterMix) + wetSignal * masterMix;
        }
    }
    
    void prepare(double sampleRate, int samplesPerBlock) {
        sampleRate_ = sampleRate;
        blockSize_ = samplesPerBlock;
        
        // Initialize SMB Pitch Shifters for each voice
        for (int i = 0; i < 3; ++i) {
            if (!pitchShifters_[i]) {
                pitchShifters_[i] = std::make_unique<SMBPitchShiftFixed>();
            }
            pitchShifters_[i]->prepare(sampleRate, samplesPerBlock);
        }
        
        // Setup smoothing for all parameters
        const float smoothTime = 10.0f;
        
        // Voice pitches
        pitchRatio1_.setSmoothingTime(smoothTime, sampleRate);
        pitchRatio2_.setSmoothingTime(smoothTime, sampleRate);
        pitchRatio3_.setSmoothingTime(smoothTime, sampleRate);
        
        // Voice volumes
        voice1Volume_.setSmoothingTime(smoothTime, sampleRate);
        voice2Volume_.setSmoothingTime(smoothTime, sampleRate);
        voice3Volume_.setSmoothingTime(smoothTime, sampleRate);
        
        // Voice formants
        voice1Formant_.setSmoothingTime(smoothTime, sampleRate);
        voice2Formant_.setSmoothingTime(smoothTime, sampleRate);
        voice3Formant_.setSmoothingTime(smoothTime, sampleRate);
        
        // Global parameters
        masterMix_.setSmoothingTime(smoothTime, sampleRate);
        humanize_.setSmoothingTime(smoothTime, sampleRate);
        width_.setSmoothingTime(smoothTime, sampleRate);
        
        // Initialize to defaults - Major chord (3rd, 5th, octave)
        pitchRatio1_.snap(1.26f);   // Major 3rd (4 semitones)
        pitchRatio2_.snap(1.5f);    // Fifth (7 semitones)
        pitchRatio3_.snap(2.0f);    // Octave (12 semitones)
        
        voice1Volume_.snap(1.0f);
        voice2Volume_.snap(0.7f);
        voice3Volume_.snap(0.5f);
        
        voice1Formant_.snap(0.5f);  // No shift
        voice2Formant_.snap(0.5f);
        voice3Formant_.snap(0.5f);
        
        masterMix_.snap(0.5f);  // Default to 50% wet for unity gain
        humanize_.snap(0.0f);
        width_.snap(0.0f);
        
        // Allocate buffers
        inputBuffer_.resize(blockSize_);
        outputBuffer_.resize(blockSize_);
        
        prepared_ = true;
    }
    
    void processBlock(const float* input, float* output, int numSamples) {
        if (!prepared_) {
            std::copy(input, input + numSamples, output);
            return;
        }
        
        // Get current mix level
        float masterMix = masterMix_.tick();
        
        #ifdef DEBUG
        static int debugCounter = 0;
        if (debugCounter++ % 100 == 0) {
            std::cout << "[processBlock] masterMix = " << masterMix << std::endl;
        }
        #endif
        
        // Early return for dry signal (0% mix)
        if (masterMix < 0.001f) {
            // Dry only - pass through unchanged
            if (input != output) {
                std::copy(input, input + numSamples, output);
            }
            return;
        }
        
        if (lowLatencyMode_) {
            // Low-latency mode: Use the processLowLatency method
            processLowLatency(input, output, numSamples);
        } else {
            // High-quality mode: Use SMBPitchShiftFixed for each voice
            // Copy input to temp buffer since input and output may be the same
            std::vector<float> inputCopy(input, input + numSamples);
            std::fill(output, output + numSamples, 0.0f);
            
            // Process each voice separately
            #ifdef DEBUG
            static int dbgCnt = 0;
            if (dbgCnt++ % 100 == 0) {
                std::cout << "[HQ] numVoices=" << numVoices_ << " mode=" << !lowLatencyMode_ << std::endl;
            }
            #endif
            
            for (int voiceIdx = 0; voiceIdx < numVoices_; ++voiceIdx) {
                float ratio = 1.0f;
                float volume = 0.0f;
                
                switch (voiceIdx) {
                    case 0:
                        ratio = pitchRatio1_.tick();
                        volume = voice1Volume_.tick();
                        break;
                    case 1:
                        ratio = pitchRatio2_.tick();
                        volume = voice2Volume_.tick();
                        break;
                    case 2:
                        ratio = pitchRatio3_.tick();
                        volume = voice3Volume_.tick();
                        break;
                }
                
                if (volume > 0.01f) {
                    if (std::fabs(ratio - 1.0f) > 0.001f && voiceIdx < 3 && pitchShifters_[voiceIdx]) {
                        // Pitched voice - use SMBPitchShift
                        #ifdef DEBUG
                        static int psCnt = 0;
                        if (psCnt++ % 100 == 0) {
                            std::cout << "[PS] voice=" << voiceIdx << " ratio=" << ratio << " vol=" << volume << std::endl;
                        }
                        #endif
                        // Set pitch shift and process
                        float semitones = 12.0f * std::log2(ratio);
                        pitchShifters_[voiceIdx]->setPitchShift(semitones);
                        
                        std::vector<float> tempOutput(numSamples);
                        pitchShifters_[voiceIdx]->process(inputCopy.data(), tempOutput.data(), numSamples);
                        
                        // Add to output with volume scaling
                        for (int i = 0; i < numSamples; ++i) {
                            output[i] += tempOutput[i] * volume;
                        }
                    } else {
                        // Unison voice (ratio = 1.0), just add with volume
                        for (int i = 0; i < numSamples; ++i) {
                            output[i] += inputCopy[i] * volume;
                        }
                    }
                }
            }
            
            // Apply master mix
            for (int i = 0; i < numSamples; ++i) {
                output[i] = inputCopy[i] * (1.0f - masterMix) + output[i] * masterMix;
            }
        }
        
        // Gentle limiting
        for (int i = 0; i < numSamples; ++i) {
            float x = output[i];
            if (std::fabs(x) > 0.95f) {
                float sign = (x > 0) ? 1.0f : -1.0f;
                x = sign * 0.95f;
            }
            output[i] = flushDenorm(x);
        }
    }
    
    void reset() {
        // Reset all pitch shifters
        for (auto& shifter : pitchShifters_) {
            if (shifter) {
                shifter->reset();
            }
        }
        inputBuffer_.clear();
        outputBuffer_.clear();
        delayBuffer_.clear();
        delayWritePos_ = 0;
    }
    
    void setPitchRatio(float ratio) { 
        pitchRatio1_.set(ratio); 
    }
    
    void setPitchRatio2(float ratio) { 
        pitchRatio2_.set(ratio); 
    }
    
    void setPitchRatio3(float ratio) { 
        pitchRatio3_.set(ratio); 
    }
    
    void setMasterMix(float m) { 
        // For mix parameter, use snap for immediate response when going to 0
        if (m < 0.001f) {
            masterMix_.snap(m);
        } else {
            masterMix_.set(m);
        }
    }
    
    void setVoice1Volume(float v) { 
        voice1Volume_.set(v); 
    }
    
    void setVoice2Volume(float v) { 
        voice2Volume_.set(v); 
    }
    
    void setVoice3Volume(float v) { 
        voice3Volume_.set(v); 
    }
    
    void setHumanize(float h) { 
        humanize_.set(h); 
    }
    
    void setPitchRatio1(float ratio) { 
        pitchRatio1_.set(ratio); 
    }
    
    void setVoice1Formant(float f) { 
        voice1Formant_.set(f); 
    }
    
    void setVoice2Formant(float f) { 
        voice2Formant_.set(f); 
    }
    
    void setVoice3Formant(float f) { 
        voice3Formant_.set(f); 
    }
    
    void setWidth(float w) { 
        width_.set(w); 
    }
    
    void setScaleIndex(int idx) { 
        scaleIndex_ = idx; 
    }
    
    void snapParameters(float ratio, float m) {
        pitchRatio1_.snap(ratio);
        masterMix_.snap(m);
    }
    
    int getLatencySamples() const {
        if (lowLatencyMode_) {
            return 0;  // Zero latency in low-latency mode
        }
        if (prepared_ && pitchShifters_[0]) {
            return pitchShifters_[0]->getLatencySamples();
        }
        return 0;
    }
    
    void setLowLatencyMode(bool enable) {
        lowLatencyMode_ = enable;
    }
};

// Public interface
IntelligentHarmonizer::IntelligentHarmonizer() : pimpl(std::make_unique<Impl>()) {}
IntelligentHarmonizer::~IntelligentHarmonizer() = default;

void IntelligentHarmonizer::prepareToPlay(double sampleRate, int samplesPerBlock) {
    pimpl->prepare(sampleRate, samplesPerBlock);
}

void IntelligentHarmonizer::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    if (numChannels == 0 || numSamples == 0) return;
    
    // Process first channel
    const float* input = buffer.getReadPointer(0);
    float* output = buffer.getWritePointer(0);
    
    pimpl->processBlock(input, output, numSamples);
    
    // Copy to other channels
    for (int ch = 1; ch < numChannels; ++ch) {
        buffer.copyFrom(ch, 0, output, numSamples);
    }
}

void IntelligentHarmonizer::reset() {
    pimpl->reset();
}

void IntelligentHarmonizer::updateParameters(const std::map<int, float>& params) {
    // The plugin sends 15 parameters (indices 0-14) in normalized 0-1 range
    // New chord-based parameter structure
    
    auto getParam = [&params](int index, float defaultValue) -> float {
        auto it = params.find(index);
        return it != params.end() ? it->second : defaultValue;
    };
    
    // Parameter 0: Number of voices (1-3)
    float voicesNorm = getParam(kVoices, 0.0f);
    int numVoices = 1;
    if (voicesNorm > 0.66f) {
        numVoices = 3;
    } else if (voicesNorm > 0.33f) {
        numVoices = 2;
    }
    pimpl->numVoices_ = numVoices;
    
    // Parameter 1: Chord type (maps to chord presets)
    float chordNorm = getParam(kChordType, 0.0f);
    pimpl->chordIndex_ = IntelligentHarmonizerChords::getChordIndex(chordNorm);
    
    // Parameter 2: Root key (C-B)
    float keyNorm = getParam(kRootKey, 0.0f);
    pimpl->rootKey_ = IntelligentHarmonizerChords::getKeyIndex(keyNorm);
    
    // Parameter 3: Scale type (0-9)
    float scaleNorm = getParam(kScale, 0.0f);
    int scaleIndex = IntelligentHarmonizerChords::getScaleIndex(scaleNorm);
    pimpl->setScaleIndex(scaleIndex);
    
    // Parameter 4: Master Mix (dry/wet)
    float masterMixNorm = getParam(kMasterMix, 0.5f);
    pimpl->setMasterMix(masterMixNorm);
    
    // Debug: Print what we're setting
    #ifdef DEBUG
    if (params.find(kMasterMix) != params.end()) {
        std::cout << "[IntelligentHarmonizer] Setting master mix to: " << masterMixNorm << std::endl;
    }
    #endif
    
    // Parameter 5: Voice 1 Volume
    float voice1VolNorm = getParam(kVoice1Volume, 1.0f);
    pimpl->setVoice1Volume(voice1VolNorm);
    
    // Parameter 6: Voice 1 Formant
    float voice1FormantNorm = getParam(kVoice1Formant, 0.5f);
    pimpl->setVoice1Formant(voice1FormantNorm);
    
    // Parameter 7: Voice 2 Volume
    float voice2VolNorm = getParam(kVoice2Volume, 0.7f);
    pimpl->setVoice2Volume(voice2VolNorm);
    
    // Parameter 8: Voice 2 Formant
    float voice2FormantNorm = getParam(kVoice2Formant, 0.5f);
    pimpl->setVoice2Formant(voice2FormantNorm);
    
    // Parameter 9: Voice 3 Volume
    float voice3VolNorm = getParam(kVoice3Volume, 0.5f);
    pimpl->setVoice3Volume(voice3VolNorm);
    
    // Parameter 10: Voice 3 Formant
    float voice3FormantNorm = getParam(kVoice3Formant, 0.5f);
    pimpl->setVoice3Formant(voice3FormantNorm);
    
    // Parameter 11: Quality mode (0 = low latency, 1 = high quality)
    // Default to high quality mode for proper pitch shifting
    float qualityNorm = getParam(kQuality, 1.0f);  // Default to 1.0 = high quality
    pimpl->setLowLatencyMode(qualityNorm < 0.5f);
    
    // Parameter 12: Humanize amount
    float humanizeNorm = getParam(kHumanize, 0.0f);
    pimpl->setHumanize(humanizeNorm);
    
    // Parameter 13: Stereo width
    float widthNorm = getParam(kWidth, 0.0f);
    pimpl->setWidth(widthNorm);
    
    // Parameter 14: Global transpose (snap to octaves)
    float transposeNorm = getParam(kTranspose, 0.5f);  // 0.5 = no transpose
    int transposeOctaves = 0;
    if (transposeNorm < 0.2f) {
        transposeOctaves = -2;  // -2 octaves
    } else if (transposeNorm < 0.4f) {
        transposeOctaves = -1;  // -1 octave
    } else if (transposeNorm > 0.8f) {
        transposeOctaves = 2;   // +2 octaves
    } else if (transposeNorm > 0.6f) {
        transposeOctaves = 1;   // +1 octave
    }
    pimpl->transposeOctaves_ = transposeOctaves;
    
    // Calculate chord-based pitch ratios
    // Get chord intervals from the chord presets
    float chordNormalized = getParam(kChordType, 0.0f);
    
    auto chordIntervals = IntelligentHarmonizerChords::getChordIntervals(chordNormalized);
    
    // Apply scale quantization if not chromatic
    if (pimpl->scaleIndex_ != 9) { // Not chromatic
        for (int i = 0; i < 3; ++i) {
            chordIntervals[i] = IntelligentHarmonizerChords::quantizeToScale(
                chordIntervals[i], pimpl->scaleIndex_, pimpl->rootKey_
            );
        }
    } else {
        // Apply root key transposition for chromatic mode
        for (int i = 0; i < 3; ++i) {
            chordIntervals[i] += pimpl->rootKey_;
        }
    }
    
    // Apply global transpose (in octaves)
    int transposeOctavesInSemitones = pimpl->transposeOctaves_ * 12;
    for (int i = 0; i < 3; ++i) {
        chordIntervals[i] += transposeOctavesInSemitones;
    }
    
    // Convert intervals to pitch ratios and set them
    float ratio1 = intervalToRatio(chordIntervals[0]);
    float ratio2 = intervalToRatio(chordIntervals[1]);
    float ratio3 = intervalToRatio(chordIntervals[2]);
    
    pimpl->setPitchRatio(ratio1);    // Voice 1
    pimpl->setPitchRatio2(ratio2);   // Voice 2  
    pimpl->setPitchRatio3(ratio3);   // Voice 3
}

void IntelligentHarmonizer::snapParameters(const std::map<int, float>& params) {
    float ratio = 1.0f;
    float mix = 0.5f;
    
    for (const auto& [paramId, value] : params) {
        if (paramId == kMasterMix) {
            mix = value;
        }
        // For chord-based system, we'll snap to the root ratio
        // The chord intervals will be calculated from the chord type
    }
    
    pimpl->snapParameters(ratio, mix);
}

juce::String IntelligentHarmonizer::getParameterName(int index) const {
    switch (index) {
        case kVoices: return "Voices";
        case kChordType: return "Chord Type";
        case kRootKey: return "Root Key";
        case kScale: return "Scale";
        case kMasterMix: return "Master Mix";
        case kVoice1Volume: return "Voice 1 Vol";
        case kVoice1Formant: return "Voice 1 Formant";
        case kVoice2Volume: return "Voice 2 Vol";
        case kVoice2Formant: return "Voice 2 Formant";
        case kVoice3Volume: return "Voice 3 Vol";
        case kVoice3Formant: return "Voice 3 Formant";
        case kQuality: return "Quality";
        case kHumanize: return "Humanize";
        case kWidth: return "Width";
        case kTranspose: return "Transpose";
        default: return "";
    }
}

juce::String IntelligentHarmonizer::getParameterDisplayString(int index, float normalizedValue) const {
    switch (index) {
        case kVoices: // Number of voices
            return juce::String(IntelligentHarmonizerChords::getVoiceCountDisplay(normalizedValue));
            
        case kChordType: // Chord type
            return juce::String(IntelligentHarmonizerChords::getChordName(normalizedValue));
            
        case kRootKey: // Root key
            return juce::String(IntelligentHarmonizerChords::getKeyName(normalizedValue));
            
        case kScale: // Scale type
            return juce::String(IntelligentHarmonizerChords::getScaleName(normalizedValue));
            
        case kMasterMix: // Master mix
        case kVoice1Volume: // Voice volumes
        case kVoice2Volume:
        case kVoice3Volume:
            return juce::String(IntelligentHarmonizerChords::getVolumeDisplay(normalizedValue));
            
        case kVoice1Formant: // Formant shifts
        case kVoice2Formant:
        case kVoice3Formant:
            return juce::String(IntelligentHarmonizerChords::getFormantDisplay(normalizedValue));
            
        case kQuality: // Quality mode
            return juce::String(IntelligentHarmonizerChords::getQualityDisplay(normalizedValue));
            
        case kHumanize: // Humanize amount
            return juce::String(IntelligentHarmonizerChords::getHumanizeDisplay(normalizedValue));
            
        case kWidth: // Stereo width
            return juce::String(IntelligentHarmonizerChords::getWidthDisplay(normalizedValue));
            
        case kTranspose: // Global transpose
            return juce::String(IntelligentHarmonizerChords::getTransposeDisplay(normalizedValue));
            
        default:
            return juce::String(normalizedValue, 2);
    }
}

int IntelligentHarmonizer::getLatencySamples() const noexcept {
    return pimpl->getLatencySamples();
}