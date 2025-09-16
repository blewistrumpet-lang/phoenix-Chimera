#include <iostream>
#include <JuceHeader.h>

int main() {
    juce::ScopedJuceInitialiser_GUI juceInit;
    
    std::cout << "Testing JUCE Convolution directly\n";
    std::cout << "==================================\n";
    
    // Create a simple IR - just a few echoes
    juce::AudioBuffer<float> ir(1, 1000);
    ir.clear();
    ir.setSample(0, 0, 1.0f);      // Direct
    ir.setSample(0, 100, 0.5f);    // Echo at 100 samples
    ir.setSample(0, 200, 0.25f);   // Echo at 200 samples
    ir.setSample(0, 300, 0.125f);  // Echo at 300 samples
    
    // Setup convolution
    juce::dsp::Convolution convolution;
    
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = 44100;
    spec.maximumBlockSize = 512;
    spec.numChannels = 1;
    
    convolution.prepare(spec);
    
    // Load IR
    std::cout << "\nLoading impulse response...\n";
    convolution.loadImpulseResponse(juce::AudioBuffer<float>(ir),
                                   44100,
                                   juce::dsp::Convolution::Stereo::no,
                                   juce::dsp::Convolution::Trim::no,
                                   juce::dsp::Convolution::Normalise::no);
    
    std::cout << "Latency: " << convolution.getLatency() << " samples\n";
    
    // Create test signal
    juce::AudioBuffer<float> buffer(1, 512);
    buffer.clear();
    buffer.setSample(0, 0, 1.0f); // Impulse
    
    // Process
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    
    std::cout << "\nProcessing impulse through convolution...\n";
    convolution.process(context);
    
    // Check output
    std::cout << "\nFirst 10 output samples:\n";
    for (int i = 0; i < 10; i++) {
        std::cout << "  " << i << ": " << buffer.getSample(0, i) << "\n";
    }
    
    std::cout << "\nSamples around expected echoes:\n";
    std::cout << "  98-102: ";
    for (int i = 98; i < 103; i++) {
        std::cout << buffer.getSample(0, i) << " ";
    }
    std::cout << "\n";
    
    std::cout << "  198-202: ";
    for (int i = 198; i < 203; i++) {
        std::cout << buffer.getSample(0, i) << " ";
    }
    std::cout << "\n";
    
    float rms = buffer.getRMSLevel(0, 0, 512);
    std::cout << "\nRMS: " << rms << "\n";
    
    if (rms > 0.01f) {
        std::cout << "\n✓ JUCE Convolution is working!\n";
    } else {
        std::cout << "\n✗ JUCE Convolution not producing output\n";
    }
    
    return 0;
}
