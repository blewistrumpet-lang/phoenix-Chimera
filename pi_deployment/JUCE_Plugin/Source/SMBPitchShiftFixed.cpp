#include "SMBPitchShiftFixed.h"
#include "signalsmith-stretch.h"

// Implementation class that isolates signalsmith-stretch compilation issues
class SMBPitchShiftFixed::Impl {
private:
    signalsmith::stretch::SignalsmithStretch<float> stretcher;
    std::vector<std::vector<float>> channelBuffers;
    std::vector<float*> inputPtrs;
    std::vector<float*> outputPtrs;
    float currentPitchRatio = 1.0f;
    double sampleRate = 44100.0;
    int maxBlockSize = 512;
    static constexpr int numChannels = 1; // Mono processing
    
public:
    Impl() {
        channelBuffers.resize(numChannels);
        inputPtrs.resize(numChannels);
        outputPtrs.resize(numChannels);
    }
    
    void prepare(double sr, int blockSize) {
        sampleRate = sr;
        maxBlockSize = blockSize;
        
        // Allocate channel buffers
        for (int ch = 0; ch < numChannels; ++ch) {
            channelBuffers[ch].resize(blockSize);
            channelBuffers[ch].shrink_to_fit();
        }
        
        // Configure stretcher
        stretcher.configure(numChannels, blockSize, blockSize / 4);
        stretcher.setTransposeFactor(1.0f);
    }
    
    void reset() {
        stretcher.reset();
        currentPitchRatio = 1.0f;
        for (auto& buffer : channelBuffers) {
            std::fill(buffer.begin(), buffer.end(), 0.0f);
        }
    }
    
    void setPitchShift(float semitones) {
        currentPitchRatio = std::pow(2.0f, semitones / 12.0f);
        stretcher.setTransposeFactor(currentPitchRatio);
    }
    
    void process(const float* input, float* output, int numSamples) {
        if (std::abs(currentPitchRatio - 1.0f) < 0.001f) {
            // No pitch shift - just copy
            if (input != output) {
                std::copy(input, input + numSamples, output);
            }
            return;
        }
        
        // Copy input to channel buffer
        std::copy(input, input + numSamples, channelBuffers[0].data());
        
        // Set up pointers
        inputPtrs[0] = channelBuffers[0].data();
        outputPtrs[0] = channelBuffers[0].data(); // In-place processing
        
        // Process using pointer arrays (what signalsmith expects)
        auto inputArray = inputPtrs.data();
        auto outputArray = outputPtrs.data();
        stretcher.process(inputArray, numSamples, outputArray, numSamples);
        
        // Copy result to output
        std::copy(channelBuffers[0].data(), channelBuffers[0].data() + numSamples, output);
    }
    
    float getLatencySamples() const {
        return stretcher.inputLatency() + stretcher.outputLatency();
    }
};

// Public interface implementation
SMBPitchShiftFixed::SMBPitchShiftFixed() 
    : pimpl(std::make_unique<Impl>()) {
}

SMBPitchShiftFixed::~SMBPitchShiftFixed() = default;

void SMBPitchShiftFixed::prepare(double sampleRate, int maxBlockSize) {
    pimpl->prepare(sampleRate, maxBlockSize);
}

void SMBPitchShiftFixed::reset() {
    pimpl->reset();
}

void SMBPitchShiftFixed::setPitchShift(float semitones) {
    pimpl->setPitchShift(semitones);
}

void SMBPitchShiftFixed::process(float* buffer, int numSamples) {
    pimpl->process(buffer, buffer, numSamples);
}

void SMBPitchShiftFixed::process(const float* input, float* output, int numSamples) {
    pimpl->process(input, output, numSamples);
}

float SMBPitchShiftFixed::getLatencySamples() const {
    return pimpl->getLatencySamples();
}