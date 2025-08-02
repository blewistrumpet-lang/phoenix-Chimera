#include "EngineFactory.h"
#include "EngineTypes.h"
#include "BypassEngine.h"
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
#include "RotarySpeaker.h"
#include "DetuneDoubler.h"
#include "LadderFilter.h"
#include "FormantFilter.h"
#include "ClassicCompressor.h"
#include "StateVariableFilter.h"
#include "StereoChorus.h"
#include "SpectralFreeze.h"
#include "GranularCloud.h"
#include "AnalogRingModulator.h"
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
#include "TransientShaper.h"
#include "DimensionExpander.h"
#include "AnalogPhaser.h"
#include "EnvelopeFilter.h"
#include "GatedReverb.h"
#include "HarmonicExciter.h"
#include "FeedbackNetwork.h"
#include "IntelligentHarmonizer.h"
#include "ParametricEQ.h"
#include "MasteringLimiter.h"
#include "NoiseGate.h"
#include "VintageOptoCompressor.h"
#include "SpectralGate.h"
#include "ChaosGenerator.h"
#include "BufferRepeat.h"
#include "VintageConsoleEQ.h"
#include "MidSideProcessor.h"
#include "VintageTubePreamp.h"
#include "SpringReverb.h"
#include "ResonantChorus.h"
#include "StereoWidener.h"
#include "DynamicEQ.h"
#include "StereoImager.h"

std::unique_ptr<EngineBase> EngineFactory::createEngine(int engineID) {
    switch (engineID) {
        case ENGINE_K_STYLE:
            return std::make_unique<KStyleOverdrive>();
            
        case ENGINE_TAPE_ECHO:
            return std::make_unique<TapeEcho>();
            
        case ENGINE_PLATE_REVERB:
            return std::make_unique<PlateReverb>();
            
        case ENGINE_RODENT_DISTORTION:
            return std::make_unique<RodentDistortion>();
            
        case ENGINE_MUFF_FUZZ:
            return std::make_unique<MuffFuzz>();
            
        case ENGINE_CLASSIC_TREMOLO:
            return std::make_unique<ClassicTremolo>();
            
        case ENGINE_MAGNETIC_DRUM_ECHO:
            return std::make_unique<MagneticDrumEcho>();
            
        case ENGINE_BUCKET_BRIGADE_DELAY:
            return std::make_unique<BucketBrigadeDelay>();
            
        case ENGINE_DIGITAL_DELAY:
            return std::make_unique<DigitalDelay>();
            
        case ENGINE_HARMONIC_TREMOLO:
            return std::make_unique<HarmonicTremolo>();
            
        case ENGINE_ROTARY_SPEAKER:
            return std::make_unique<RotarySpeaker>();
            
        case ENGINE_DETUNE_DOUBLER:
            return std::make_unique<DetuneDoubler>();
            
        case ENGINE_LADDER_FILTER:
            return std::make_unique<LadderFilter>();
            
        case ENGINE_FORMANT_FILTER:
            return std::make_unique<FormantFilter>();
            
        case ENGINE_CLASSIC_COMPRESSOR:
            return std::make_unique<ClassicCompressor>();
            
        case ENGINE_STATE_VARIABLE_FILTER:
            return std::make_unique<StateVariableFilter>();
            
        case ENGINE_STEREO_CHORUS:
            return std::make_unique<StereoChorus>();
            
        case ENGINE_SPECTRAL_FREEZE:
            return std::make_unique<SpectralFreeze>();
            
        case ENGINE_GRANULAR_CLOUD:
            return std::make_unique<GranularCloud>();
            
        case ENGINE_ANALOG_RING_MODULATOR:
            return std::make_unique<AnalogRingModulator>();
            
        case ENGINE_MULTIBAND_SATURATOR:
            return std::make_unique<MultibandSaturator>();
            
        case ENGINE_COMB_RESONATOR:
            return std::make_unique<CombResonator>();
            
        case ENGINE_PITCH_SHIFTER:
            return std::make_unique<PitchShifter>();
            
        case ENGINE_PHASED_VOCODER:
            return std::make_unique<PhasedVocoder>();
            
        case ENGINE_CONVOLUTION_REVERB:
            return std::make_unique<ConvolutionReverb>();
            
        case ENGINE_BIT_CRUSHER:
            return std::make_unique<BitCrusher>();
            
        case ENGINE_FREQUENCY_SHIFTER:
            return std::make_unique<FrequencyShifter>();
            
        case ENGINE_WAVE_FOLDER:
            return std::make_unique<WaveFolder>();
            
        case ENGINE_SHIMMER_REVERB:
            return std::make_unique<ShimmerReverb>();
            
        case ENGINE_VOCAL_FORMANT_FILTER:
            return std::make_unique<VocalFormantFilter>();
            
        case ENGINE_TRANSIENT_SHAPER:
            return std::make_unique<TransientShaper>();
            
        case ENGINE_DIMENSION_EXPANDER:
            return std::make_unique<DimensionExpander>();
            
        case ENGINE_ANALOG_PHASER:
            return std::make_unique<AnalogPhaser>();
            
        case ENGINE_ENVELOPE_FILTER:
            return std::make_unique<EnvelopeFilter>();
            
        case ENGINE_GATED_REVERB:
            return std::make_unique<GatedReverb>();
            
        case ENGINE_HARMONIC_EXCITER:
            return std::make_unique<HarmonicExciter>();
            
        case ENGINE_FEEDBACK_NETWORK:
            return std::make_unique<FeedbackNetwork>();
            
        case ENGINE_INTELLIGENT_HARMONIZER:
            return std::make_unique<IntelligentHarmonizer>();
            
        case ENGINE_PARAMETRIC_EQ:
            return std::make_unique<ParametricEQ>();
            
        case ENGINE_MASTERING_LIMITER:
            return std::make_unique<MasteringLimiter>();
            
        case ENGINE_NOISE_GATE:
            return std::make_unique<NoiseGate>();
            
        case ENGINE_VINTAGE_OPTO_COMPRESSOR:
            return std::make_unique<VintageOptoCompressor>();
            
        case ENGINE_SPECTRAL_GATE:
            return std::make_unique<SpectralGate>();
            
        case ENGINE_CHAOS_GENERATOR:
            return std::make_unique<ChaosGenerator>();
            
        case ENGINE_BUFFER_REPEAT:
            return std::make_unique<BufferRepeat>();
            
        case ENGINE_VINTAGE_CONSOLE_EQ:
            return std::make_unique<VintageConsoleEQ>();
            
        case ENGINE_MID_SIDE_PROCESSOR:
            return std::make_unique<MidSideProcessor>();
            
        case ENGINE_STEREO_WIDENER:
            return std::make_unique<StereoWidener>();
            
        case ENGINE_STEREO_IMAGER:
            return std::make_unique<StereoImager>();
            
        case ENGINE_DYNAMIC_EQ:
            return std::make_unique<DynamicEQ>();
            
        case ENGINE_VINTAGE_TUBE_PREAMP:
            return std::make_unique<VintageTubePreamp>();
            
        case ENGINE_SPRING_REVERB:
            return std::make_unique<SpringReverb>();
            
        case ENGINE_RESONANT_CHORUS:
            return std::make_unique<ResonantChorus>();
            
        case ENGINE_BYPASS:
        default:
            return std::make_unique<BypassEngine>();
    }
}