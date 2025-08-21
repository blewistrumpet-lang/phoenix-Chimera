// Fixed shiftSpectrum function for PitchShifter.cpp
// This replaces lines 392-444 in the original file

void shiftSpectrum(ChannelState& ch, float pitch, float formant) {
    // Temporary buffer for shifted spectrum
    alignas(16) std::array<std::complex<float>, FFT_SIZE> shifted{};
    
    // CRITICAL FIX: Update phase for ALL bins, not just non-zero ones
    // The phase accumulator needs to track phase evolution even for silent bins
    for (int bin = 0; bin <= FFT_SIZE/2; ++bin) {
        // Calculate the frequency for this bin after pitch shifting
        const double shiftedFreq = ch.frequency[bin] * pitch;
        
        // Update phase accumulator for this frequency
        ch.phaseSum[bin] += 2.0 * M_PI * shiftedFreq * HOP_SIZE / sampleRate;
        
        // Wrap phase to [-pi, pi] to prevent numerical overflow
        while (ch.phaseSum[bin] > M_PI) ch.phaseSum[bin] -= 2.0 * M_PI;
        while (ch.phaseSum[bin] < -M_PI) ch.phaseSum[bin] += 2.0 * M_PI;
    }
    
    // Now apply spectral reconstruction with optional formant shift
    if (std::abs(formant - 1.0f) < 0.001f) {
        // No formant shift - direct reconstruction with pitch-shifted phases
        for (int bin = 0; bin <= FFT_SIZE/2; ++bin) {
            if (ch.magnitude[bin] > 1e-10f) {
                // Use original magnitude with pitch-shifted phase
                const float mag = ch.magnitude[bin];
                const float phase = static_cast<float>(ch.phaseSum[bin]);
                shifted[bin] = std::polar(mag, phase);
                
                // Maintain Hermitian symmetry for real output
                if (bin > 0 && bin < FFT_SIZE/2) {
                    shifted[FFT_SIZE - bin] = std::conj(shifted[bin]);
                }
            }
        }
    } else {
        // Apply formant shift by remapping magnitude envelope
        // while preserving pitch-shifted phases
        for (int bin = 0; bin <= FFT_SIZE/2; ++bin) {
            // Find source bin for magnitude (formant shift)
            const int sourceBin = static_cast<int>(bin / formant + 0.5f);
            
            if (sourceBin >= 0 && sourceBin <= FFT_SIZE/2 && 
                ch.magnitude[sourceBin] > 1e-10f) {
                // Use magnitude from formant-shifted source
                // but phase from pitch-shifted target bin
                const float mag = ch.magnitude[sourceBin];
                const float phase = static_cast<float>(ch.phaseSum[bin]);
                shifted[bin] = std::polar(mag, phase);
                
                // Maintain Hermitian symmetry
                if (bin > 0 && bin < FFT_SIZE/2) {
                    shifted[FFT_SIZE - bin] = std::conj(shifted[bin]);
                }
            }
        }
    }
    
    ch.spectrum = shifted;
}

/* EXPLANATION OF THE FIX:

The original bug was that phase accumulation only happened for bins with 
magnitude > 1e-10f. This meant:

1. Silent or near-silent bins didn't get their phase updated
2. When formant shifting remapped bins, it would use stale phase values
3. The pitch shift effectively did nothing because phases weren't evolving

The fix:
1. Update phase for ALL bins regardless of magnitude
2. Phase represents the frequency evolution over time
3. Even silent bins need phase tracking for when they become audible
4. Formant shift now correctly uses updated phases

This is a classic phase vocoder bug - the phase accumulator must run
continuously for all frequencies to maintain phase coherence.
*/