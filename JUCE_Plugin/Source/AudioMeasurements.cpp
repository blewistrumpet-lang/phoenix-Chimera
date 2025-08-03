#include "AudioMeasurements.h"
#include <algorithm>
#include <numeric>

float AudioMeasurements::measureRMS(const juce::AudioBuffer<float>& buffer) {
    float sumSquares = 0.0f;
    int totalSamples = 0;
    
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
        const float* data = buffer.getReadPointer(channel);
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            sumSquares += data[i] * data[i];
            totalSamples++;
        }
    }
    
    return std::sqrt(sumSquares / totalSamples);
}

float AudioMeasurements::measurePeak(const juce::AudioBuffer<float>& buffer) {
    float peak = 0.0f;
    
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
        auto range = buffer.findMinMax(channel, 0, buffer.getNumSamples());
        peak = std::max(peak, std::max(std::abs(range.getStart()), std::abs(range.getEnd())));
    }
    
    return peak;
}

float AudioMeasurements::measureTHD(const juce::AudioBuffer<float>& buffer, float fundamentalFreq, float sampleRate) {
    auto harmonics = measureHarmonicContent(buffer, fundamentalFreq, sampleRate);
    return harmonics.thd;
}

float AudioMeasurements::measureSNR(const juce::AudioBuffer<float>& signal, const juce::AudioBuffer<float>& noise) {
    float signalRMS = measureRMS(signal);
    float noiseRMS = measureRMS(noise);
    
    if (noiseRMS < 0.000001f) return 100.0f; // Very high SNR
    
    return 20.0f * std::log10(signalRMS / noiseRMS);
}

float AudioMeasurements::measureGainReduction(const juce::AudioBuffer<float>& input, const juce::AudioBuffer<float>& output) {
    float inputRMS = measureRMS(input);
    float outputRMS = measureRMS(output);
    
    if (inputRMS < 0.000001f) return 0.0f;
    
    return 20.0f * std::log10(outputRMS / inputRMS);
}

std::pair<float, float> AudioMeasurements::measureEnvelopeTiming(const juce::AudioBuffer<float>& buffer, float sampleRate) {
    const float* data = buffer.getReadPointer(0);
    int numSamples = buffer.getNumSamples();
    
    // Find peak
    float peak = 0.0f;
    int peakIndex = 0;
    for (int i = 0; i < numSamples; ++i) {
        float absValue = std::abs(data[i]);
        if (absValue > peak) {
            peak = absValue;
            peakIndex = i;
        }
    }
    
    // Measure attack (10% to 90% of peak)
    float threshold10 = peak * 0.1f;
    float threshold90 = peak * 0.9f;
    
    int attack10Index = 0;
    int attack90Index = peakIndex;
    
    for (int i = 0; i < peakIndex; ++i) {
        float absValue = std::abs(data[i]);
        if (absValue >= threshold10 && attack10Index == 0) {
            attack10Index = i;
        }
        if (absValue >= threshold90) {
            attack90Index = i;
            break;
        }
    }
    
    float attackTime = (attack90Index - attack10Index) / sampleRate * 1000.0f; // ms
    
    // Measure release (90% to 10% after peak)
    int release90Index = peakIndex;
    int release10Index = numSamples - 1;
    
    for (int i = peakIndex; i < numSamples; ++i) {
        float absValue = std::abs(data[i]);
        if (absValue <= threshold90 && release90Index == peakIndex) {
            release90Index = i;
        }
        if (absValue <= threshold10) {
            release10Index = i;
            break;
        }
    }
    
    float releaseTime = (release10Index - release90Index) / sampleRate * 1000.0f; // ms
    
    return { attackTime, releaseTime };
}

AudioMeasurements::FrequencyResponse AudioMeasurements::computeFrequencyResponse(
    const juce::AudioBuffer<float>& buffer, float sampleRate) {
    
    int fftSize = 2048;
    auto fftData = performFFT(buffer, fftSize);
    auto magnitudes = computeMagnitudeSpectrum(fftData);
    auto phases = computePhaseSpectrum(fftData);
    
    FrequencyResponse response;
    int numBins = fftSize / 2;
    response.frequencies.resize(numBins);
    response.magnitudes.resize(numBins);
    response.phases.resize(numBins);
    
    for (int i = 0; i < numBins; ++i) {
        response.frequencies[i] = (i * sampleRate) / fftSize;
        response.magnitudes[i] = magnitudes[i];
        response.phases[i] = phases[i];
    }
    
    return response;
}

float AudioMeasurements::measureDelayTime(const juce::AudioBuffer<float>& input, 
                                         const juce::AudioBuffer<float>& output, 
                                         float sampleRate) {
    const float* inputData = input.getReadPointer(0);
    const float* outputData = output.getReadPointer(0);
    int length = std::min(input.getNumSamples(), output.getNumSamples());
    
    int delaySamples = findDelayUsingCorrelation(inputData, outputData, length);
    return delaySamples / sampleRate * 1000.0f; // Convert to ms
}

float AudioMeasurements::measureRT60(const juce::AudioBuffer<float>& impulseResponse, float sampleRate) {
    const float* data = impulseResponse.getReadPointer(0);
    int numSamples = impulseResponse.getNumSamples();
    
    // Calculate energy decay curve
    std::vector<float> energyDecay(numSamples);
    float totalEnergy = 0.0f;
    
    // Calculate backward integrated energy
    for (int i = numSamples - 1; i >= 0; --i) {
        totalEnergy += data[i] * data[i];
        energyDecay[i] = totalEnergy;
    }
    
    // Normalize and convert to dB
    float maxEnergy = energyDecay[0];
    for (int i = 0; i < numSamples; ++i) {
        if (energyDecay[i] > 0.0f) {
            energyDecay[i] = 10.0f * std::log10(energyDecay[i] / maxEnergy);
        } else {
            energyDecay[i] = -100.0f;
        }
    }
    
    // Find -5dB and -35dB points for T30 measurement
    int idx5dB = 0, idx35dB = 0;
    for (int i = 0; i < numSamples; ++i) {
        if (energyDecay[i] <= -5.0f && idx5dB == 0) idx5dB = i;
        if (energyDecay[i] <= -35.0f && idx35dB == 0) {
            idx35dB = i;
            break;
        }
    }
    
    if (idx35dB > idx5dB && idx5dB > 0) {
        float t30 = (idx35dB - idx5dB) / sampleRate;
        return t30 * 2.0f; // RT60 = T30 * 2
    }
    
    return 0.0f;
}

AudioMeasurements::ModulationProfile AudioMeasurements::extractModulationProfile(
    const juce::AudioBuffer<float>& buffer, float sampleRate) {
    
    const float* data = buffer.getReadPointer(0);
    int numSamples = buffer.getNumSamples();
    
    // Extract envelope
    std::vector<float> envelope(numSamples);
    for (int i = 0; i < numSamples; ++i) {
        envelope[i] = std::abs(data[i]);
    }
    
    // Smooth envelope
    float smooth = 0.99f;
    for (int i = 1; i < numSamples; ++i) {
        envelope[i] = envelope[i] * (1.0f - smooth) + envelope[i-1] * smooth;
    }
    
    // Find modulation frequency using autocorrelation
    std::vector<float> autocorr(numSamples / 2);
    for (int lag = 0; lag < numSamples / 2; ++lag) {
        float sum = 0.0f;
        for (int i = 0; i < numSamples - lag; ++i) {
            sum += envelope[i] * envelope[i + lag];
        }
        autocorr[lag] = sum;
    }
    
    // Find first peak after zero lag
    int peakLag = 0;
    float maxCorr = 0.0f;
    for (int lag = static_cast<int>(sampleRate / 20.0f); lag < numSamples / 2; ++lag) { // Skip below 20Hz
        if (autocorr[lag] > maxCorr) {
            maxCorr = autocorr[lag];
            peakLag = lag;
        }
    }
    
    ModulationProfile profile;
    profile.rate = peakLag > 0 ? sampleRate / peakLag : 0.0f;
    
    // Calculate depth
    float minEnv = *std::min_element(envelope.begin(), envelope.end());
    float maxEnv = *std::max_element(envelope.begin(), envelope.end());
    profile.depth = (maxEnv - minEnv) / (maxEnv + minEnv + 0.0001f);
    
    profile.phase = 0.0f; // Would need reference signal to calculate
    
    return profile;
}

AudioMeasurements::HarmonicContent AudioMeasurements::measureHarmonicContent(
    const juce::AudioBuffer<float>& buffer, float fundamentalFreq, float sampleRate) {
    
    int fftSize = 4096;
    auto fftData = performFFT(buffer, fftSize);
    auto magnitudes = computeMagnitudeSpectrum(fftData);
    
    HarmonicContent content;
    
    // Find harmonics up to 10th
    for (int harmonic = 1; harmonic <= 10; ++harmonic) {
        float targetFreq = fundamentalFreq * harmonic;
        int bin = static_cast<int>(targetFreq * fftSize / sampleRate);
        
        if (bin < magnitudes.size()) {
            // Find peak around expected bin
            int searchRange = 3;
            float maxMag = 0.0f;
            int maxBin = bin;
            
            for (int b = std::max(0, bin - searchRange); 
                 b < std::min(static_cast<int>(magnitudes.size()), bin + searchRange + 1); ++b) {
                if (magnitudes[b] > maxMag) {
                    maxMag = magnitudes[b];
                    maxBin = b;
                }
            }
            
            content.harmonicAmplitudes.push_back(maxMag);
            content.harmonicFrequencies.push_back(maxBin * sampleRate / fftSize);
        }
    }
    
    // Calculate THD
    if (content.harmonicAmplitudes.size() > 1) {
        float fundamental = content.harmonicAmplitudes[0];
        float harmonicsSum = 0.0f;
        
        for (size_t i = 1; i < content.harmonicAmplitudes.size(); ++i) {
            harmonicsSum += content.harmonicAmplitudes[i] * content.harmonicAmplitudes[i];
        }
        
        content.thd = std::sqrt(harmonicsSum) / (fundamental + 0.0001f) * 100.0f; // Percentage
    } else {
        content.thd = 0.0f;
    }
    
    return content;
}

float AudioMeasurements::measureIMD(const juce::AudioBuffer<float>& buffer, 
                                   float freq1, float freq2, float sampleRate) {
    auto fftData = performFFT(buffer, 4096);
    auto magnitudes = computeMagnitudeSpectrum(fftData);
    
    // Find intermodulation products
    float sumFreq = freq1 + freq2;
    float diffFreq = std::abs(freq1 - freq2);
    
    int fftSize = 4096;
    int sumBin = static_cast<int>(sumFreq * fftSize / sampleRate);
    int diffBin = static_cast<int>(diffFreq * fftSize / sampleRate);
    int freq1Bin = static_cast<int>(freq1 * fftSize / sampleRate);
    int freq2Bin = static_cast<int>(freq2 * fftSize / sampleRate);
    
    float fundamental = (magnitudes[freq1Bin] + magnitudes[freq2Bin]) / 2.0f;
    float imdProducts = magnitudes[sumBin] + magnitudes[diffBin];
    
    return (imdProducts / (fundamental + 0.0001f)) * 100.0f; // Percentage
}

bool AudioMeasurements::detectSustainedOscillation(const juce::AudioBuffer<float>& buffer, float sampleRate) {
    // Check if signal maintains consistent amplitude over time
    int numSegments = 10;
    int segmentSize = buffer.getNumSamples() / numSegments;
    
    std::vector<float> segmentRMS;
    const float* data = buffer.getReadPointer(0);
    
    for (int seg = 0; seg < numSegments; ++seg) {
        float sumSquares = 0.0f;
        for (int i = seg * segmentSize; i < (seg + 1) * segmentSize; ++i) {
            sumSquares += data[i] * data[i];
        }
        segmentRMS.push_back(std::sqrt(sumSquares / segmentSize));
    }
    
    // Check if RMS is consistent (oscillation) or decaying (no oscillation)
    float firstRMS = segmentRMS[0];
    float lastRMS = segmentRMS.back();
    
    // If last segment is > 80% of first segment, likely oscillating
    return (lastRMS > firstRMS * 0.8f) && (lastRMS > 0.01f);
}

float AudioMeasurements::measureLatency(const juce::AudioBuffer<float>& input, 
                                       const juce::AudioBuffer<float>& output, 
                                       float sampleRate) {
    return measureDelayTime(input, output, sampleRate);
}

float AudioMeasurements::measureNoiseFloor(const juce::AudioBuffer<float>& buffer) {
    // Measure RMS of quietest portion
    const float* data = buffer.getReadPointer(0);
    int numSamples = buffer.getNumSamples();
    int windowSize = numSamples / 10;
    
    float minRMS = 1.0f;
    
    for (int start = 0; start < numSamples - windowSize; start += windowSize / 2) {
        float sumSquares = 0.0f;
        for (int i = start; i < start + windowSize; ++i) {
            sumSquares += data[i] * data[i];
        }
        float rms = std::sqrt(sumSquares / windowSize);
        minRMS = std::min(minRMS, rms);
    }
    
    return 20.0f * std::log10(minRMS + 0.00001f); // dB
}

std::vector<std::complex<float>> AudioMeasurements::performFFT(const juce::AudioBuffer<float>& buffer, int fftSize) {
    juce::dsp::FFT fft(static_cast<int>(std::log2(fftSize)));
    std::vector<std::complex<float>> fftData(fftSize);
    
    // Copy and window the data
    const float* inputData = buffer.getReadPointer(0);
    int numSamples = std::min(buffer.getNumSamples(), fftSize);
    
    for (int i = 0; i < numSamples; ++i) {
        // Hann window
        float window = 0.5f * (1.0f - std::cos(2.0f * M_PI * i / (numSamples - 1)));
        fftData[i] = std::complex<float>(inputData[i] * window, 0.0f);
    }
    
    // Zero pad if necessary
    for (int i = numSamples; i < fftSize; ++i) {
        fftData[i] = std::complex<float>(0.0f, 0.0f);
    }
    
    // Perform FFT
    fft.performFrequencyOnlyForwardTransform(reinterpret_cast<float*>(fftData.data()));
    
    return fftData;
}

std::vector<float> AudioMeasurements::computeMagnitudeSpectrum(const std::vector<std::complex<float>>& fftData) {
    std::vector<float> magnitudes;
    magnitudes.reserve(fftData.size() / 2);
    
    for (size_t i = 0; i < fftData.size() / 2; ++i) {
        magnitudes.push_back(std::abs(fftData[i]));
    }
    
    return magnitudes;
}

std::vector<float> AudioMeasurements::computePhaseSpectrum(const std::vector<std::complex<float>>& fftData) {
    std::vector<float> phases;
    phases.reserve(fftData.size() / 2);
    
    for (size_t i = 0; i < fftData.size() / 2; ++i) {
        phases.push_back(std::arg(fftData[i]));
    }
    
    return phases;
}

float AudioMeasurements::findPeakFrequency(const std::vector<float>& magnitudeSpectrum, float sampleRate) {
    auto maxIt = std::max_element(magnitudeSpectrum.begin(), magnitudeSpectrum.end());
    int bin = std::distance(magnitudeSpectrum.begin(), maxIt);
    return bin * sampleRate / (magnitudeSpectrum.size() * 2);
}

float AudioMeasurements::correlate(const float* signal1, const float* signal2, int length) {
    float sum = 0.0f;
    for (int i = 0; i < length; ++i) {
        sum += signal1[i] * signal2[i];
    }
    return sum;
}

int AudioMeasurements::findDelayUsingCorrelation(const float* input, const float* output, int length) {
    int maxLag = length / 2;
    float maxCorr = 0.0f;
    int bestLag = 0;
    
    for (int lag = 0; lag < maxLag; ++lag) {
        float corr = correlate(input, output + lag, length - lag);
        if (corr > maxCorr) {
            maxCorr = corr;
            bestLag = lag;
        }
    }
    
    return bestLag;
}