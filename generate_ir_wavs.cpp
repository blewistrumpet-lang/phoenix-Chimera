#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <cstdint>
#include <random>

// WAV file header structure
struct WavHeader {
    char riff[4] = {'R', 'I', 'F', 'F'};
    uint32_t fileSize;
    char wave[4] = {'W', 'A', 'V', 'E'};
    char fmt[4] = {'f', 'm', 't', ' '};
    uint32_t fmtSize = 16;
    uint16_t format = 1; // PCM
    uint16_t channels = 2; // Stereo
    uint32_t sampleRate = 44100;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample = 16;
    char data[4] = {'d', 'a', 't', 'a'};
    uint32_t dataSize;
};

void writeWav(const std::string& filename, const std::vector<float>& left, const std::vector<float>& right) {
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to create " << filename << std::endl;
        return;
    }
    
    WavHeader header;
    header.byteRate = header.sampleRate * header.channels * (header.bitsPerSample / 8);
    header.blockAlign = header.channels * (header.bitsPerSample / 8);
    header.dataSize = left.size() * header.blockAlign;
    header.fileSize = 36 + header.dataSize;
    
    file.write((char*)&header, sizeof(header));
    
    // Interleave and write samples
    for (size_t i = 0; i < left.size(); i++) {
        int16_t leftSample = (int16_t)(left[i] * 32767.0f);
        int16_t rightSample = (int16_t)(right[i] * 32767.0f);
        file.write((char*)&leftSample, 2);
        file.write((char*)&rightSample, 2);
    }
    
    file.close();
    std::cout << "Created " << filename << " (" << left.size() << " samples)" << std::endl;
}

void generateConcertHall(std::vector<float>& left, std::vector<float>& right, int sampleRate) {
    int numSamples = sampleRate * 3; // 3 seconds
    left.resize(numSamples, 0.0f);
    right.resize(numSamples, 0.0f);
    
    std::mt19937 gen(42);
    std::normal_distribution<float> dist(0.0f, 1.0f);
    
    // Early reflections
    const float times[] = {0.015f, 0.022f, 0.035f, 0.045f, 0.058f, 0.072f, 0.089f, 0.108f};
    const float gains[] = {0.7f, 0.65f, 0.6f, 0.55f, 0.5f, 0.45f, 0.4f, 0.35f};
    
    for (int i = 0; i < 8; i++) {
        int pos = (int)(times[i] * sampleRate);
        if (pos < numSamples) {
            left[pos] = gains[i] * (i % 2 ? 0.8f : -0.8f);
            right[pos] = gains[i] * (i % 2 ? -0.7f : 0.9f);
        }
    }
    
    // Diffuse tail
    float rt60 = 2.8f;
    for (int i = sampleRate / 10; i < numSamples; i++) {
        float t = i / (float)sampleRate;
        float env = std::exp(-3.0f * t / rt60);
        float noise = dist(gen) * 0.1f;
        left[i] += noise * env * (1.0f - t/3.0f * 0.5f);
        right[i] += noise * env * (1.0f - t/3.0f * 0.6f) * 0.95f;
    }
}

void generateEMTPlate(std::vector<float>& left, std::vector<float>& right, int sampleRate) {
    int numSamples = sampleRate * 2; // 2 seconds
    left.resize(numSamples, 0.0f);
    right.resize(numSamples, 0.0f);
    
    std::mt19937 gen(123);
    std::normal_distribution<float> dist(0.0f, 1.0f);
    
    // Dense immediate onset
    float rt60 = 1.8f;
    for (int i = 1; i < numSamples; i++) {
        float t = i / (float)sampleRate;
        float env = std::exp(-3.0f * t / rt60);
        
        // Metallic resonances
        float metallic = std::sin(i * 0.00523f) * 0.3f +
                        std::sin(i * 0.00234f) * 0.25f +
                        std::sin(i * 0.00445f) * 0.2f;
        
        left[i] = metallic * env * (1.0f - t/2.0f * 0.3f);
        right[i] = metallic * env * (1.0f - t/2.0f * 0.35f) * 1.05f;
        
        // Add some noise
        if (i % 5 == 0) {
            float noise = dist(gen) * 0.05f;
            left[i] += noise * env;
            right[i] += noise * env * 0.9f;
        }
    }
}

void generateStairwell(std::vector<float>& left, std::vector<float>& right, int sampleRate) {
    int numSamples = sampleRate * 2; // 2 seconds
    left.resize(numSamples, 0.0f);
    right.resize(numSamples, 0.0f);
    
    std::mt19937 gen(456);
    std::normal_distribution<float> dist(0.0f, 1.0f);
    
    // Flutter echoes
    float flutterPeriod = 0.012f;
    int flutterSamples = (int)(flutterPeriod * sampleRate);
    float rt60 = 1.2f;
    
    for (int echo = 0; echo < 150; echo++) {
        int pos = echo * flutterSamples;
        if (pos >= numSamples) break;
        
        float gain = std::pow(0.85f, (float)echo);
        left[pos] += gain;
        right[pos] += gain * 0.95f;
        
        // Add diffusion around each echo
        for (int j = 1; j < 20; j++) {
            int diffPos = pos + j * 3;
            if (diffPos < numSamples) {
                float diffGain = gain * 0.05f * (1.0f - j/20.0f);
                left[diffPos] += dist(gen) * diffGain;
                right[diffPos] += dist(gen) * diffGain * 1.1f;
            }
        }
    }
    
    // Overall decay envelope
    for (int i = 0; i < numSamples; i++) {
        float t = i / (float)sampleRate;
        float env = std::exp(-3.0f * t / rt60);
        left[i] *= env;
        right[i] *= env;
    }
}

void generateCloudChamber(std::vector<float>& left, std::vector<float>& right, int sampleRate) {
    int numSamples = sampleRate * 4; // 4 seconds - long ethereal
    left.resize(numSamples, 0.0f);
    right.resize(numSamples, 0.0f);
    
    std::mt19937 gen(789);
    std::normal_distribution<float> dist(0.0f, 1.0f);
    std::uniform_real_distribution<float> uniform(0.0f, 1.0f);
    
    // Granular texture
    for (int g = 0; g < 500; g++) {
        int pos = (int)(uniform(gen) * numSamples * 0.8f);
        float gain = uniform(gen) * 0.3f;
        int grainSize = 100 + (int)(uniform(gen) * 400);
        
        for (int i = 0; i < grainSize && (pos + i) < numSamples; i++) {
            float envelope = std::sin((float)i / grainSize * M_PI);
            float sample = dist(gen) * envelope * gain;
            left[pos + i] += sample;
            right[pos + i] += sample * (0.5f + uniform(gen) * 0.5f);
        }
    }
    
    // Long decay with modulation
    float rt60 = 4.5f;
    for (int i = 0; i < numSamples; i++) {
        float t = i / (float)sampleRate;
        float env = std::exp(-3.0f * t / rt60);
        float mod = 1.0f + std::sin(t * 2.0f * M_PI) * 0.3f;
        left[i] *= env * mod;
        right[i] *= env * mod * 1.05f;
    }
}

int main() {
    const int sampleRate = 44100;
    
    std::cout << "Generating IR WAV files..." << std::endl;
    
    // Generate Concert Hall
    {
        std::vector<float> left, right;
        generateConcertHall(left, right, sampleRate);
        writeWav("ConcertHall.wav", left, right);
    }
    
    // Generate EMT Plate
    {
        std::vector<float> left, right;
        generateEMTPlate(left, right, sampleRate);
        writeWav("EMTPlate.wav", left, right);
    }
    
    // Generate Stairwell
    {
        std::vector<float> left, right;
        generateStairwell(left, right, sampleRate);
        writeWav("Stairwell.wav", left, right);
    }
    
    // Generate Cloud Chamber
    {
        std::vector<float> left, right;
        generateCloudChamber(left, right, sampleRate);
        writeWav("CloudChamber.wav", left, right);
    }
    
    std::cout << "\nAll IR files generated successfully!" << std::endl;
    return 0;
}
