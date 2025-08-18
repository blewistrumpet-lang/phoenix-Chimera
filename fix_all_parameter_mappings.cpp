/**
 * Complete Parameter Mapping Fix Guide
 * Based on actual test results
 */

// CRITICAL FIXES NEEDED in getMixParameterIndex():

// ❌ OUT OF RANGE - These will crash:
case ENGINE_NONE: return -1;              // Has 0 params, was returning 6
case ENGINE_DYNAMIC_EQ: return 3;          // Has 4 params (0-3), was returning 6  
case ENGINE_ENVELOPE_FILTER: return 5;     // Has 6 params (0-5), was returning 7
case ENGINE_LADDER_FILTER: return 4;       // Has 5 params (0-4), was returning 8
case ENGINE_STEREO_CHORUS: return 4;       // Has 5 params (0-4), was returning 6
case ENGINE_VINTAGE_FLANGER: return 6;     // Has 7 params (0-6), was returning 7
case ENGINE_RING_MODULATOR: return -1;     // No mix param (utility)
case ENGINE_PITCH_SHIFTER: return 1;       // Has 2 params (0-1), was returning 2
case ENGINE_TAPE_ECHO: return 3;           // Has 4 params (0-3), was returning 4
case ENGINE_PLATE_REVERB: return 3;        // Has 4 params (0-3), was returning 6 ✅ FIXED
case ENGINE_SPRING_REVERB: return 7;       // Has 8 params (0-7), was returning 9 ✅ FIXED
case ENGINE_GATED_REVERB: return 5;        // Has 6 params (0-5), was returning 8

// ⚠️ WRONG PARAMETER - These point to wrong param:
case ENGINE_CLASSIC_COMPRESSOR: return 6;  // Was 4 (Knee), Mix is at 6 ✅ FIXED
case ENGINE_VINTAGE_OPTO_COMPRESSOR: return 6; // Was 4 (Knee), Mix likely at 6
case ENGINE_NOISE_GATE_PLATINUM: return -1;   // No actual Mix param
case ENGINE_TRANSIENT_SHAPER: return 9;    // Was 5 (Makeup), Mix at 9
case ENGINE_PARAMETRIC_EQ: return 2;       // Was 8 (Band 1 Gain), Mix at 2
case ENGINE_VINTAGE_CONSOLE_EQ: return -1; // No Mix param
case ENGINE_ANALOG_PHASER: return -1;      // No Mix param  
case ENGINE_FORMANT_FILTER: return -1;     // No Mix param
case ENGINE_VINTAGE_TUBE: return -1;       // No Mix param
case ENGINE_BIT_CRUSHER: return -1;        // No Mix param
case ENGINE_FREQUENCY_SHIFTER: return -1;  // No Mix param
case ENGINE_BUCKET_BRIGADE_DELAY: return -1; // No Mix param
case ENGINE_MAGNETIC_DRUM_ECHO: return -1;  // No Mix param
case ENGINE_CONVOLUTION_REVERB: return -1;  // No Mix param

// Engines that should NOT have mix (utilities):
case ENGINE_GAIN_UTILITY: return -1;
case ENGINE_MONO_MAKER: return -1;
case ENGINE_MID_SIDE_PROCESSOR: return -1;
case ENGINE_PHASE_ALIGN: return -1;
case ENGINE_SPECTRAL_FREEZE: return -1;
case ENGINE_GRANULAR_CLOUD: return -1;