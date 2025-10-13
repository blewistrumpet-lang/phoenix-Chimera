#include <JuceHeader.h>
#include "../../JUCE_Plugin/Source/PluginProcessor.h"

using namespace juce;

static AudioBuffer<float> loadWav(const File& f, double& sr) {
    WavAudioFormat wavFormat;
    std::unique_ptr<AudioFormatReader> reader(
        wavFormat.createReaderFor(f.createInputStream().release(), true));
    if (!reader) { std::cerr << "Failed to load: " << f.getFullPathName() << std::endl; return AudioBuffer<float>(1, 0); }
    sr = reader->sampleRate;
    AudioBuffer<float> buf(1, (int)reader->lengthInSamples);
    reader->read(&buf, 0, buf.getNumSamples(), 0, true, false);
    return buf;
}

static void saveWav(const File& f, const AudioBuffer<float>& buffer, double sampleRate) {
    WavAudioFormat wavFormat;
    std::unique_ptr<FileOutputStream> outputStream(f.createOutputStream());
    if (!outputStream) { std::cerr << "Failed to create output" << std::endl; return; }
    std::unique_ptr<AudioFormatWriter> writer(wavFormat.createWriterFor(outputStream.release(), sampleRate, 1, 24, {}, 0));
    if (writer) writer->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples());
}

int main(int argc, char** argv) {
    if (argc < 4) { std::cout << "Usage: OfflineRender <input.wav> <output.wav> <engineID>" << std::endl; return 1; }
    File inputFile = File(argv[1]);
    File outputFile = File(argv[2]);
    int engineID = String(argv[3]).getIntValue();
    
    double sampleRate = 48000.0;
    auto inputBuffer = loadWav(inputFile, sampleRate);
    if (inputBuffer.getNumSamples() == 0) { std::cerr << "Failed to load" << std::endl; return 1; }
    
    ChimeraAudioProcessor processor;
    processor.setRateAndBufferSizeDetails(sampleRate, 512);
    processor.prepareToPlay(sampleRate, 512);
    processor.setSlotEngine(0, engineID);
    
    AudioBuffer<float> outputBuffer(2, inputBuffer.getNumSamples());
    outputBuffer.clear();
    for (int i = 0; i < inputBuffer.getNumSamples(); ++i) {
        outputBuffer.setSample(0, i, inputBuffer.getSample(0, i));
        outputBuffer.setSample(1, i, inputBuffer.getSample(0, i));
    }
    
    MidiBuffer midiBuffer;
    int pos = 0;
    while (pos < outputBuffer.getNumSamples()) {
        int numSamples = jmin(512, outputBuffer.getNumSamples() - pos);
        AudioBuffer<float> block(outputBuffer.getArrayOfWritePointers(), 2, pos, numSamples);
        processor.processBlock(block, midiBuffer);
        pos += numSamples;
    }
    
    AudioBuffer<float> monoOutput(1, outputBuffer.getNumSamples());
    for (int i = 0; i < outputBuffer.getNumSamples(); ++i) {
        monoOutput.setSample(0, i, (outputBuffer.getSample(0, i) + outputBuffer.getSample(1, i)) * 0.5f);
    }
    
    saveWav(outputFile, monoOutput, sampleRate);
    std::cout << "Done: " << outputFile.getFullPathName() << std::endl;
    return 0;
}
