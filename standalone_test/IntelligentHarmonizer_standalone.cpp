// Standalone IntelligentHarmonizer implementation for debugging
// This version has extensive debug output

#include "IntelligentHarmonizer_standalone.h"
#include "SMBPitchShiftFixed_standalone.h"
#include "IntelligentHarmonizerChords.h"
#include <algorithm>
#include <cmath>
#include <memory>
#include <atomic>
#include <random>
#include <iostream>

namespace {

template<typename T>
inline T flushDenorm(T v) noexcept {
    constexpr T tiny = static_cast<T>(1.0e-38);
    return std::fabs(v) < tiny ? static_cast<T>(0) : v;
}

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

float intervalToRatio(int semitones) {
    return std::pow(2.0f, semitones / 12.0f);
}

} // namespace

class IntelligentHarmonizer_Standalone::Impl {
public:
    std::array<std::unique_ptr<SMBPitchShiftFixed>, 3> pitchShifters_;

    SmoothedParam pitchRatio1_;
    SmoothedParam pitchRatio2_;
    SmoothedParam pitchRatio3_;

    SmoothedParam voice1Volume_;
    SmoothedParam voice2Volume_;
    SmoothedParam voice3Volume_;

    SmoothedParam masterMix_;

    int numVoices_ = 3;
    int chordIndex_ = 0;
    int rootKey_ = 0;
    int scaleIndex_ = 9;
    int transposeOctaves_ = 0;
    bool lowLatencyMode_ = false;

    double sampleRate_ = 48000.0;
    int blockSize_ = 512;
    bool prepared_ = false;

    void prepare(double sampleRate, int samplesPerBlock) {
        sampleRate_ = sampleRate;
        blockSize_ = samplesPerBlock;

        std::cout << "[Harmonizer::prepare] sampleRate=" << sampleRate
                  << " blockSize=" << samplesPerBlock << std::endl;

        for (int i = 0; i < 3; ++i) {
            if (!pitchShifters_[i]) {
                pitchShifters_[i] = std::make_unique<SMBPitchShiftFixed>();
            }
            pitchShifters_[i]->prepare(sampleRate, samplesPerBlock);
        }

        const float smoothTime = 10.0f;
        pitchRatio1_.setSmoothingTime(smoothTime, sampleRate);
        pitchRatio2_.setSmoothingTime(smoothTime, sampleRate);
        pitchRatio3_.setSmoothingTime(smoothTime, sampleRate);

        voice1Volume_.setSmoothingTime(smoothTime, sampleRate);
        voice2Volume_.setSmoothingTime(smoothTime, sampleRate);
        voice3Volume_.setSmoothingTime(smoothTime, sampleRate);

        masterMix_.setSmoothingTime(smoothTime, sampleRate);

        pitchRatio1_.snap(1.26f);
        pitchRatio2_.snap(1.5f);
        pitchRatio3_.snap(2.0f);

        voice1Volume_.snap(1.0f);
        voice2Volume_.snap(0.7f);
        voice3Volume_.snap(0.5f);

        masterMix_.snap(0.5f);

        prepared_ = true;
        std::cout << "[Harmonizer::prepare] Complete!" << std::endl;
    }

    void processBlock(const float* input, float* output, int numSamples) {
        if (!prepared_) {
            std::copy(input, input + numSamples, output);
            return;
        }

        float masterMix = masterMix_.tick();

        static int debugCounter = 0;
        if (debugCounter++ % 100 == 0) {
            std::cout << "[processBlock] masterMix=" << masterMix
                      << " numVoices=" << numVoices_ << std::endl;
        }

        if (masterMix < 0.001f) {
            if (input != output) {
                std::copy(input, input + numSamples, output);
            }
            return;
        }

        // High-quality mode
        std::vector<float> inputCopy(input, input + numSamples);
        std::fill(output, output + numSamples, 0.0f);

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

            if (debugCounter % 100 == 0) {
                std::cout << "[Voice " << voiceIdx << "] ratio=" << ratio
                          << " volume=" << volume << std::endl;
            }

            if (volume > 0.01f) {
                if (std::fabs(ratio - 1.0f) > 0.001f && voiceIdx < 3 && pitchShifters_[voiceIdx]) {
                    std::vector<float> tempOutput(numSamples);

                    if (debugCounter % 100 == 0) {
                        std::cout << "[Voice " << voiceIdx << "] Pitch shifting with ratio=" << ratio << std::endl;
                    }

                    pitchShifters_[voiceIdx]->process(inputCopy.data(), tempOutput.data(), numSamples, ratio);

                    // Check tempOutput for zeros
                    float tempRMS = 0.0f;
                    for (int i = 0; i < numSamples; ++i) {
                        tempRMS += tempOutput[i] * tempOutput[i];
                    }
                    tempRMS = std::sqrt(tempRMS / numSamples);

                    if (debugCounter % 100 == 0) {
                        std::cout << "[Voice " << voiceIdx << "] Pitch shifter output RMS=" << tempRMS << std::endl;
                    }

                    for (int i = 0; i < numSamples; ++i) {
                        output[i] += tempOutput[i] * volume;
                    }
                } else {
                    for (int i = 0; i < numSamples; ++i) {
                        output[i] += inputCopy[i] * volume;
                    }
                }
            }
        }

        // Check wet signal
        float wetRMS = 0.0f;
        for (int i = 0; i < numSamples; ++i) {
            wetRMS += output[i] * output[i];
        }
        wetRMS = std::sqrt(wetRMS / numSamples);

        if (debugCounter % 100 == 0) {
            std::cout << "[processBlock] Wet signal RMS=" << wetRMS << std::endl;
        }

        // Apply master mix
        for (int i = 0; i < numSamples; ++i) {
            output[i] = inputCopy[i] * (1.0f - masterMix) + output[i] * masterMix;
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
        for (auto& shifter : pitchShifters_) {
            if (shifter) {
                shifter->reset();
            }
        }
    }

    void setPitchRatio(float ratio) { pitchRatio1_.set(ratio); }
    void setPitchRatio2(float ratio) { pitchRatio2_.set(ratio); }
    void setPitchRatio3(float ratio) { pitchRatio3_.set(ratio); }
    void setMasterMix(float m) {
        if (m < 0.001f) {
            masterMix_.snap(m);
        } else {
            masterMix_.set(m);
        }
    }
    void setVoice1Volume(float v) { voice1Volume_.set(v); }
    void setVoice2Volume(float v) { voice2Volume_.set(v); }
    void setVoice3Volume(float v) { voice3Volume_.set(v); }

    int getLatencySamples() const {
        if (prepared_ && pitchShifters_[0]) {
            return pitchShifters_[0]->getLatencySamples();
        }
        return 0;
    }
};

IntelligentHarmonizer_Standalone::IntelligentHarmonizer_Standalone()
    : pimpl(std::make_unique<Impl>()) {}

IntelligentHarmonizer_Standalone::~IntelligentHarmonizer_Standalone() = default;

void IntelligentHarmonizer_Standalone::prepareToPlay(double sampleRate, int samplesPerBlock) {
    pimpl->prepare(sampleRate, samplesPerBlock);
}

void IntelligentHarmonizer_Standalone::processBlock(const float* input, float* output, int numSamples) {
    pimpl->processBlock(input, output, numSamples);
}

void IntelligentHarmonizer_Standalone::reset() {
    pimpl->reset();
}

void IntelligentHarmonizer_Standalone::updateParameters(const std::map<int, float>& params) {
    auto getParam = [&params](int index, float defaultValue) -> float {
        auto it = params.find(index);
        return it != params.end() ? it->second : defaultValue;
    };

    float voicesNorm = getParam(kVoices, 0.0f);
    int numVoices = 1;
    if (voicesNorm > 0.66f) {
        numVoices = 3;
    } else if (voicesNorm > 0.33f) {
        numVoices = 2;
    }
    pimpl->numVoices_ = numVoices;

    float chordNorm = getParam(kChordType, 0.0f);
    pimpl->chordIndex_ = IntelligentHarmonizerChords::getChordIndex(chordNorm);

    float keyNorm = getParam(kRootKey, 0.0f);
    pimpl->rootKey_ = IntelligentHarmonizerChords::getKeyIndex(keyNorm);

    float scaleNorm = getParam(kScale, 0.0f);
    pimpl->scaleIndex_ = IntelligentHarmonizerChords::getScaleIndex(scaleNorm);

    float masterMixNorm = getParam(kMasterMix, 0.5f);
    pimpl->setMasterMix(masterMixNorm);

    std::cout << "[updateParameters] masterMix=" << masterMixNorm
              << " numVoices=" << numVoices << std::endl;

    float voice1VolNorm = getParam(kVoice1Volume, 1.0f);
    pimpl->setVoice1Volume(voice1VolNorm);

    float voice2VolNorm = getParam(kVoice2Volume, 0.7f);
    pimpl->setVoice2Volume(voice2VolNorm);

    float voice3VolNorm = getParam(kVoice3Volume, 0.5f);
    pimpl->setVoice3Volume(voice3VolNorm);

    float qualityNorm = getParam(kQuality, 1.0f);
    pimpl->lowLatencyMode_ = (qualityNorm < 0.5f);

    float transposeNorm = getParam(kTranspose, 0.5f);
    int transposeOctaves = 0;
    if (transposeNorm < 0.2f) {
        transposeOctaves = -2;
    } else if (transposeNorm < 0.4f) {
        transposeOctaves = -1;
    } else if (transposeNorm > 0.8f) {
        transposeOctaves = 2;
    } else if (transposeNorm > 0.6f) {
        transposeOctaves = 1;
    }
    pimpl->transposeOctaves_ = transposeOctaves;

    auto chordIntervals = IntelligentHarmonizerChords::getChordIntervals(chordNorm);

    if (pimpl->scaleIndex_ != 9) {
        for (int i = 0; i < 3; ++i) {
            chordIntervals[i] = IntelligentHarmonizerChords::quantizeToScale(
                chordIntervals[i], pimpl->scaleIndex_, pimpl->rootKey_
            );
        }
    } else {
        for (int i = 0; i < 3; ++i) {
            chordIntervals[i] += pimpl->rootKey_;
        }
    }

    int transposeOctavesInSemitones = pimpl->transposeOctaves_ * 12;
    for (int i = 0; i < 3; ++i) {
        chordIntervals[i] += transposeOctavesInSemitones;
    }

    float ratio1 = intervalToRatio(chordIntervals[0]);
    float ratio2 = intervalToRatio(chordIntervals[1]);
    float ratio3 = intervalToRatio(chordIntervals[2]);

    std::cout << "[updateParameters] Pitch ratios: " << ratio1 << ", " << ratio2 << ", " << ratio3 << std::endl;

    pimpl->setPitchRatio(ratio1);
    pimpl->setPitchRatio2(ratio2);
    pimpl->setPitchRatio3(ratio3);
}

void IntelligentHarmonizer_Standalone::snapParameters(const std::map<int, float>& params) {
    updateParameters(params);
    auto it = params.find(kMasterMix);
    if (it != params.end()) {
        pimpl->setMasterMix(it->second);
    }
}

int IntelligentHarmonizer_Standalone::getLatencySamples() const noexcept {
    return pimpl->getLatencySamples();
}
