// Test chord interval calculations
#include "JUCE_Plugin/Source/IntelligentHarmonizerChords.h"
#include <iostream>
#include <cmath>

float intervalToRatio(int semitones) {
    return std::pow(2.0f, semitones / 12.0f);
}

int main() {
    std::cout << "=== Testing Chord Interval Calculations ===" << std::endl;
    
    // Test Major chord (first preset)
    float chordNorm = 0.0f;  // First chord
    
    auto chordIntervals = IntelligentHarmonizerChords::getChordIntervals(chordNorm);
    std::string chordName = IntelligentHarmonizerChords::getChordName(chordNorm);
    
    std::cout << "\nChord: " << chordName << std::endl;
    std::cout << "Intervals: " << chordIntervals[0] << ", " 
              << chordIntervals[1] << ", " << chordIntervals[2] << " semitones" << std::endl;
    
    std::cout << "\nPitch ratios from 440 Hz:" << std::endl;
    for (int i = 0; i < 3; ++i) {
        float ratio = intervalToRatio(chordIntervals[i]);
        float targetFreq = 440.0f * ratio;
        std::cout << "  Voice " << (i+1) << ": " << chordIntervals[i] 
                  << " semitones -> ratio " << ratio 
                  << " -> " << targetFreq << " Hz" << std::endl;
    }
    
    // Test a few more chords
    std::cout << "\n=== Testing Various Chords ===" << std::endl;
    
    float testNorms[] = {0.0f, 0.05f, 0.1f, 0.5f, 1.0f};
    
    for (float norm : testNorms) {
        auto intervals = IntelligentHarmonizerChords::getChordIntervals(norm);
        std::string name = IntelligentHarmonizerChords::getChordName(norm);
        
        std::cout << "\nNormalized: " << norm << " -> " << name << std::endl;
        std::cout << "  Intervals: [" << intervals[0] << ", " 
                  << intervals[1] << ", " << intervals[2] << "]" << std::endl;
    }
    
    // Test voice count mapping
    std::cout << "\n=== Testing Voice Count Mapping ===" << std::endl;
    
    float voiceNorms[] = {0.0f, 0.16f, 0.33f, 0.5f, 0.66f, 0.8f, 1.0f};
    
    for (float norm : voiceNorms) {
        int count = IntelligentHarmonizerChords::getVoiceCount(norm);
        std::string display = IntelligentHarmonizerChords::getVoiceCountDisplay(norm);
        std::cout << "Normalized " << norm << " -> " << count 
                  << " voices (" << display << ")" << std::endl;
    }
    
    return 0;
}