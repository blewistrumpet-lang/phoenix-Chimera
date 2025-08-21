#include <iostream>
#include <cmath>

// Critical diagnosis of PitchShifter complete failure

void diagnoseSignalFlow() {
    std::cout << "=== PITCHSHIFTER SIGNAL FLOW DIAGNOSIS ===" << std::endl;
    
    std::cout << "\n1. MIX PARAMETER (Line 298):" << std::endl;
    std::cout << "   data[i] = input * (1.0f - mix) + output * mix" << std::endl;
    std::cout << "   When mix = 1.0:" << std::endl;
    std::cout << "   data[i] = input * 0.0 + output * 1.0 = output only" << std::endl;
    std::cout << "   ❌ If output is 0, NO SOUND!" << std::endl;
    
    std::cout << "\n2. OUTPUT GENERATION (Line 285):" << std::endl;
    std::cout << "   output = ch.readOutput()" << std::endl;
    std::cout << "   Reads from outputRing buffer" << std::endl;
    std::cout << "   ❌ If outputRing is empty/zero, output = 0!" << std::endl;
    
    std::cout << "\n3. OUTPUT RING BUFFER FILLED BY (Line 345):" << std::endl;
    std::cout << "   ch.scatterFrame(ch.spectrum.data(), outputScale)" << std::endl;
    std::cout << "   Only called from processSpectralFrame" << std::endl;
    std::cout << "   ❌ If processSpectralFrame doesn't run, no output!" << std::endl;
    
    std::cout << "\n4. PROCESS SPECTRAL FRAME CALLED (Line 280-284):" << std::endl;
    std::cout << "   if (ch.hopCounter >= HOP_SIZE) {" << std::endl;
    std::cout << "       processSpectralFrame(ch, pitch, formant, gate, window);" << std::endl;
    std::cout << "   }" << std::endl;
    std::cout << "   HOP_SIZE = FFT_SIZE / OVERLAP_FACTOR = 4096 / 4 = 1024" << std::endl;
    std::cout << "   ❌ Takes 1024 samples before first frame!" << std::endl;
    
    std::cout << "\n5. FFT PROCESSING (Line 318):" << std::endl;
    std::cout << "   ch.fft->perform(ch.spectrum.data(), ch.spectrum.data(), false)" << std::endl;
    std::cout << "   Forward FFT" << std::endl;
    std::cout << "   ❌ If FFT fails or spectrum is zero, no output!" << std::endl;
    
    std::cout << "\n6. CRITICAL BUG - OUTPUT SCALING (Line 214):" << std::endl;
    std::cout << "   outputScale = 1.0f / (FFT_SIZE * OVERLAP_FACTOR * 2.0f)" << std::endl;
    std::cout << "   outputScale = 1.0f / (4096 * 4 * 2) = 1.0f / 32768" << std::endl;
    std::cout << "   outputScale = 0.00003052" << std::endl;
    std::cout << "   ❌ OUTPUT IS SCALED DOWN BY 32768x!" << std::endl;
    
    std::cout << "\n7. SCATTER FRAME (Line 166):" << std::endl;
    std::cout << "   outputRing[idx] += fftOut[i].real() * synthesisWindow[i] * scale" << std::endl;
    std::cout << "   With scale = 0.00003052, output is nearly zero!" << std::endl;
}

void identifyRootCause() {
    std::cout << "\n=== ROOT CAUSES IDENTIFIED ===" << std::endl;
    
    std::cout << "\n1. OUTPUT SCALING IS WRONG!" << std::endl;
    std::cout << "   Current: 1.0f / 32768 = 0.00003052" << std::endl;
    std::cout << "   Should be closer to: 1.0f / 4 = 0.25" << std::endl;
    std::cout << "   The signal is being attenuated by 8000x!" << std::endl;
    
    std::cout << "\n2. INITIALIZATION DELAY" << std::endl;
    std::cout << "   Need 1024 samples before first output" << std::endl;
    std::cout << "   At 44.1kHz = 23ms delay" << std::endl;
    
    std::cout << "\n3. PHASE VOCODER MAY BE BROKEN" << std::endl;
    std::cout << "   Even with fixed scaling, if phase reconstruction is wrong," << std::endl;
    std::cout << "   the output will be noise or silence" << std::endl;
}

void proposeFixes() {
    std::cout << "\n=== IMMEDIATE FIXES NEEDED ===" << std::endl;
    
    std::cout << "\n1. FIX OUTPUT SCALING:" << std::endl;
    std::cout << "   Line 214: outputScale = 1.0f / OVERLAP_FACTOR;" << std::endl;
    std::cout << "   This gives 1.0f / 4 = 0.25" << std::endl;
    
    std::cout << "\n2. ADD DEBUG OUTPUT:" << std::endl;
    std::cout << "   Log RMS of input buffer" << std::endl;
    std::cout << "   Log RMS of spectrum after FFT" << std::endl;
    std::cout << "   Log RMS of output buffer" << std::endl;
    
    std::cout << "\n3. TEST BYPASS MODE:" << std::endl;
    std::cout << "   Set pitch ratio to 1.0 (no shift)" << std::endl;
    std::cout << "   Should pass through unchanged" << std::endl;
    
    std::cout << "\n4. CHECK FFT IMPLEMENTATION:" << std::endl;
    std::cout << "   JUCE FFT may expect different scaling" << std::endl;
    std::cout << "   May need to normalize after FFT" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "PITCHSHIFTER COMPLETE FAILURE DIAGNOSIS" << std::endl;
    std::cout << "========================================" << std::endl;
    
    diagnoseSignalFlow();
    identifyRootCause();
    proposeFixes();
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "CRITICAL: Output scaling is 8000x too small!" << std::endl;
    std::cout << "Fix: outputScale = 1.0f / OVERLAP_FACTOR" << std::endl;
    std::cout << "========================================" << std::endl;
    
    return 0;
}