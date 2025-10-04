#include "EngineFactory.h"
#include "EngineTypes.h"
#include "ParameterDefinitions.h"
#include "NoneEngine.h"
#include "KStyleOverdrive.h"
#include "TapeEcho.h"
#include "PlateReverb.h"
#include "RodentDistortion.h"
#include "MuffFuzz.h"
#include "ClassicTremolo.h"
#include "MagneticDrumEcho.h"
#include "BucketBrigadeDelay.h"
#include "DigitalDelay.h"
#include "HarmonicTremolo.h"
#include "RotarySpeaker_Platinum.h"
#include "DetuneDoubler.h"
#include "LadderFilter.h"
#include "FormantFilter.h"
#include "ClassicCompressor.h"
#include "StateVariableFilter.h"
#include "StereoChorus.h"
#include "SpectralFreeze.h"
#include "GranularCloud.h"
#include "PlatinumRingModulator.h"
#include "MultibandSaturator.h"
#include "CombResonator.h"
#include "PitchShifter.h"
#include "PhasedVocoder.h"
#include "ConvolutionReverb.h"
#include "BitCrusher.h"
#include "FrequencyShifter.h"
#include "WaveFolder.h"
#include "ShimmerReverb.h"
#include "VocalFormantFilter.h"
#include "TransientShaper_Platinum.h"
#include "DimensionExpander.h"
#include "AnalogPhaser.h"
#include "EnvelopeFilter.h"
#include "GatedReverb.h"
#include "HarmonicExciter_Platinum.h"
#include "FeedbackNetwork.h"
#include "IntelligentHarmonizer.h"
#include "ParametricEQ_Studio.h"
#include "MasteringLimiter_Platinum.h"
#include "NoiseGate_Platinum.h"
#include "VintageOptoCompressor_Platinum.h"
#include "SpectralGate_Platinum.h"
#include "ChaosGenerator_Platinum.h"
#include "BufferRepeat_Platinum.h"
#include "VintageConsoleEQ_Studio.h"
#include "MidSideProcessor_Platinum.h"
#include "VintageTubePreamp_Studio.h"
#include "SpringReverb.h"
#include "ResonantChorus_Platinum.h"
#include "GainUtility_Platinum.h"
#include "MonoMaker_Platinum.h"
#include "PhaseAlign_Platinum.h"
#include "StereoWidener.h"
#include "DynamicEQ.h"
#include "StereoImager.h"

std::unique_ptr<EngineBase> EngineFactory::createEngine(int engineID) {
    DBG("EngineFactory::createEngine called with engineID: " + juce::String(engineID));
    
    switch (engineID) {
        // ENGINE_NONE (0)
        case 0:
            return std::make_unique<NoneEngine>();
            
        // DYNAMICS & COMPRESSION (1-6)
        case 1: // ENGINE_OPTO_COMPRESSOR
            return std::make_unique<VintageOptoCompressor_Platinum>();
            
        case 2: // ENGINE_VCA_COMPRESSOR
            return std::make_unique<ClassicCompressor>();
            
        case 3: // ENGINE_TRANSIENT_SHAPER
            return std::make_unique<TransientShaper_Platinum>();
            
        case 4: // ENGINE_NOISE_GATE
            return std::make_unique<NoiseGate_Platinum>();
            
        case 5: // ENGINE_MASTERING_LIMITER
            return std::make_unique<MasteringLimiter_Platinum>();
            
        case 6: // ENGINE_DYNAMIC_EQ
            return std::make_unique<DynamicEQ>();
            
        // FILTERS & EQ (7-14)
        case 7: // ENGINE_PARAMETRIC_EQ
            return std::make_unique<ParametricEQ_Studio>();
            
        case 8: // ENGINE_VINTAGE_CONSOLE_EQ
            return std::make_unique<VintageConsoleEQ_Studio>();
            
        case 9: // ENGINE_LADDER_FILTER
            return std::make_unique<LadderFilter>();
            
        case 10: // ENGINE_STATE_VARIABLE_FILTER
            return std::make_unique<StateVariableFilter>();
            
        case 11: // ENGINE_FORMANT_FILTER
            return std::make_unique<FormantFilter>();
            
        case 12: // ENGINE_ENVELOPE_FILTER
            return std::make_unique<EnvelopeFilter>();
            
        case 13: // ENGINE_COMB_RESONATOR
            return std::make_unique<CombResonator>();
            
        case 14: // ENGINE_VOCAL_FORMANT
            return std::make_unique<VocalFormantFilter>();
            
        // DISTORTION & SATURATION (15-22)
        case 15: // ENGINE_VINTAGE_TUBE
            return std::make_unique<VintageTubePreamp_Studio>();
            
        case 16: // ENGINE_WAVE_FOLDER
            return std::make_unique<WaveFolder>();
            
        case 17: // ENGINE_HARMONIC_EXCITER
            return std::make_unique<HarmonicExciter_Platinum>();
            
        case 18: // ENGINE_BIT_CRUSHER
            return std::make_unique<BitCrusher>();
            
        case 19: // ENGINE_MULTIBAND_SATURATOR
            return std::make_unique<MultibandSaturator>();
            
        case 20: // ENGINE_MUFF_FUZZ
            return std::make_unique<MuffFuzz>();
            
        case 21: // ENGINE_RODENT_DISTORTION
            return std::make_unique<RodentDistortion>();
            
        case 22: // ENGINE_K_STYLE
            return std::make_unique<KStyleOverdrive>();
            
        // MODULATION (23-33)
        case 23: // ENGINE_DIGITAL_CHORUS
            return std::make_unique<StereoChorus>();
            
        case 24: // ENGINE_RESONANT_CHORUS
            return std::make_unique<ResonantChorus_Platinum>();
            
        case 25: // ENGINE_ANALOG_PHASER
            return std::make_unique<AnalogPhaser>();
            
        case 26: // ENGINE_RING_MODULATOR
            return std::make_unique<PlatinumRingModulator>();
            
        case 27: // ENGINE_FREQUENCY_SHIFTER
            return std::make_unique<FrequencyShifter>();
            
        case 28: // ENGINE_HARMONIC_TREMOLO
            return std::make_unique<HarmonicTremolo>();
            
        case 29: // ENGINE_CLASSIC_TREMOLO
            return std::make_unique<ClassicTremolo>();
            
        case 30: // ENGINE_ROTARY_SPEAKER
            return std::make_unique<AudioDSP::RotarySpeaker_Platinum>();
            
        case 31: // ENGINE_PITCH_SHIFTER
            return std::make_unique<PitchShifter>();
            
        case 32: // ENGINE_DETUNE_DOUBLER
            return std::make_unique<AudioDSP::DetuneDoubler>();
            
        case 33: // ENGINE_INTELLIGENT_HARMONIZER
            return std::make_unique<IntelligentHarmonizer>();
            
        // REVERB & DELAY (34-43)
        case 34: // ENGINE_TAPE_ECHO
            return std::make_unique<TapeEcho>();
            
        case 35: // ENGINE_DIGITAL_DELAY
            return std::make_unique<AudioDSP::DigitalDelay>();
            
        case 36: // ENGINE_MAGNETIC_DRUM_ECHO
            return std::make_unique<MagneticDrumEcho>();
            
        case 37: // ENGINE_BUCKET_BRIGADE_DELAY
            return std::make_unique<BucketBrigadeDelay>();
            
        case 38: // ENGINE_BUFFER_REPEAT
            return std::make_unique<BufferRepeat_Platinum>();
            
        case 39: // ENGINE_PLATE_REVERB
            return std::make_unique<PlateReverb>();
            
        case 40: // ENGINE_SPRING_REVERB
            return std::make_unique<SpringReverb>();
            
        case 41: // ENGINE_CONVOLUTION_REVERB
            return std::make_unique<ConvolutionReverb>();
            
        case 42: // ENGINE_SHIMMER_REVERB
            return std::make_unique<ShimmerReverb>();
            
        case 43: // ENGINE_GATED_REVERB
            return std::make_unique<GatedReverb>();
            
        // SPATIAL & SPECIAL (44-52)
        case 44: // ENGINE_STEREO_WIDENER
            return std::make_unique<StereoWidener>();
            
        case 45: // ENGINE_STEREO_IMAGER
            return std::make_unique<StereoImager>();
            
        case 46: // ENGINE_DIMENSION_EXPANDER
            return std::make_unique<DimensionExpander>();
            
        case 47: // ENGINE_SPECTRAL_FREEZE
            return std::make_unique<SpectralFreeze>();
            
        case 48: // ENGINE_SPECTRAL_GATE
            return std::make_unique<SpectralGate_Platinum>();
            
        case 49: // ENGINE_PHASED_VOCODER
            return std::make_unique<PhasedVocoder>();
            
        case 50: // ENGINE_GRANULAR_CLOUD
            return std::make_unique<GranularCloud>();
            
        case 51: // ENGINE_CHAOS_GENERATOR
            return std::make_unique<ChaosGenerator_Platinum>();
            
        case 52: // ENGINE_FEEDBACK_NETWORK
            return std::make_unique<FeedbackNetwork>();
            
        // UTILITY (53-56)
        case 53: // ENGINE_MID_SIDE_PROCESSOR
            return std::make_unique<MidSideProcessor_Platinum>();
            
        case 54: // ENGINE_GAIN_UTILITY
            return std::make_unique<GainUtility_Platinum>();
            
        case 55: // ENGINE_MONO_MAKER
            return std::make_unique<MonoMaker_Platinum>();
            
        case 56: // ENGINE_PHASE_ALIGN
            return std::make_unique<PhaseAlign_Platinum>();
            
        default:
            // Return null for invalid engine types
            return nullptr;
    }
}