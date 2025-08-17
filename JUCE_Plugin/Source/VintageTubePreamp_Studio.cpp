#include "VintageTubePreamp_Studio.h"
#include <cstring>

// ---------- Halfband tables (already in header) ----------

VintageTubePreamp_Studio::VintageTubePreamp_Studio(){}
VintageTubePreamp_Studio::~VintageTubePreamp_Studio(){}

void VintageTubePreamp_Studio::prepareToPlay(double sampleRate,int samplesPerBlock){
    fs_ = std::max(1.0, sampleRate);
    blockSize_ = std::max(1, samplesPerBlock);

    V1_.setTypeAX7(); V2_.setTypeAX7(); V3_.setTypeAU7();
    V1_.Rp = 22000.f; V2_.Rp = 22000.f; V3_.Rp = 4700.f;

    C1_.setup(22e-9f, (float)fs_);   // ~22 nF classic coupling
    C2_.setup(47e-9f, (float)fs_);   // ~47 nF
    tone_.reset(); tone_.update(voicing_, bass_, mid_, treble_, (float)fs_);

    rail_.V0 = 300.f; rail_.Rsrc=150.f; rail_.C=47e-6f; rail_.reset();
    ot_.setTilt(1.0f, 0.97f); ot_.setNFB(0.1f); ot_.setPresence(presence_);

    V1_.reset(); V2_.reset(); V3_.reset();
    os4_.reset(); dc_[0].reset(); dc_[1].reset(); micro_.reset();

    ctrlPhase_ = 0; rnd_=0x1234567u;
}

void VintageTubePreamp_Studio::reset(){ prepareToPlay(fs_, blockSize_); }

void VintageTubePreamp_Studio::updateParameters(const std::map<int,float>& p){
    // Helper to get parameter value with default
    auto getParam = [&p](int id, float defaultVal) -> float {
        auto it = p.find(id);
        return (it != p.end()) ? it->second : defaultVal;
    };
    
    // Map slot parameters (0-14) to Tube Preamp:
    // 0: Drive
    // 1: Bass
    // 2: Mid
    // 3: Treble
    // 4: Presence
    // 5: Bright
    // 6: Voicing (0-0.33=Fender, 0.33-0.66=Marshall, 0.66-1=Vox)
    // 7: Microphonics/mechanical
    // 8: Ghost notes
    // 9: Noise amount
    // 10: Input trim
    // 11: Output trim
    // 12: Not used
    // 13: Mix
    // 14: Not used
    
    bypass_   = false; // Bypass handled by plugin framework
    drive_    = getParam(0, 0.4f);
    bass_     = getParam(1, 0.5f);
    mid_      = getParam(2, 0.5f);
    treble_   = getParam(3, 0.5f);
    presence_ = getParam(4, 0.3f);
    bright_   = getParam(5, 0.f);
    
    // Map voicing from normalized value
    float voiceNorm = getParam(6, 0.0f);
    if (voiceNorm < 0.33f) voicing_ = FENDER_DLUX;
    else if (voiceNorm < 0.66f) voicing_ = MARSHALL_PLEXI;
    else voicing_ = VOX_AC30;
    
    micMech_  = getParam(7, 0.f);
    ghost_    = getParam(8, 0.f);
    noise_    = getParam(9, 0.f);
    inTrim_   = (getParam(10, 0.5f) - 0.5f) * 48.f; // Map 0-1 to -24 to +24 dB
    outTrim_  = (getParam(11, 0.5f) - 0.5f) * 48.f; // Map 0-1 to -24 to +24 dB
    osMode_   = 0; // Auto mode

    tone_.update(voicing_, bass_, mid_, treble_, (float)fs_);
    ot_.setPresence(presence_);
}

juce::String VintageTubePreamp_Studio::getParameterName(int index) const {
    switch(index){
        case kBypass:        return "Bypass";
        case kVoicing:       return "Voicing";
        case kInputTrim_dB:  return "Input Trim (dB)";
        case kOutputTrim_dB: return "Output Trim (dB)";
        case kDrive:         return "Drive";
        case kBright:        return "Bright";
        case kBass:          return "Bass";
        case kMid:           return "Mid";
        case kTreble:        return "Treble";
        case kPresence:      return "Presence";
        case kMicMech:       return "Microphonics";
        case kGhost:         return "Ghost Notes";
        case kNoise:         return "Noise";
        case kOSMode:        return "Oversampling Mode";
        default:             return "Param " + juce::String(index);
    }
}

void VintageTubePreamp_Studio::controlTick(){
    // Map drive to bias & rail: more drive -> hotter bias (less negative), sag more likely
    const float drive = drive_;
    V1_.Vbias = -2.0f + 1.5f*drive;
    V2_.Vbias = -1.8f + 1.2f*drive;
    V3_.Vbias = -15.0f + 10.0f*drive; // 12AU7

    // Bright cap: simple HF shelf injection at V1 output (pre-tonestack)
    // Implement as small treble boost in tone update via 'treble' micro offset—already handled by user control, so do tiny extra lift:
    const float brightLift = 0.15f*bright_;
    tone_.update(voicing_, bass_, mid_, std::clamp(treble_ + brightLift, 0.f, 1.f), (float)fs_);
}

void VintageTubePreamp_Studio::process(juce::AudioBuffer<float>& buffer){
    DenormalGuard guard;

    const int nCh = std::min(buffer.getNumChannels(), 2);
    const int N   = buffer.getNumSamples();
    if (N<=0 || nCh<=0){ return; }

    if (bypass_){ scrubBuffer(buffer); return; }

    // decide OS
    const bool needOS = (osMode_==1) || (osMode_==0 && fs_<96000.0);

    const float inTrim = dbToLin(inTrim_);
    const float outTrim= dbToLin(outTrim_);

    // control ticks chunked
    int pos=0; if (ctrlPhase_<=0) ctrlPhase_=kCtrlInterval;
    while(pos<N){
        int run = std::min(ctrlPhase_, N-pos);
        ctrlPhase_ -= run;
        if (ctrlPhase_==0){ controlTick(); ctrlPhase_=kCtrlInterval; }

        // process per sample
        for (int n=0;n<run;++n){
            // Mono core; for stereo, process L/R through identical core (dual-mono)
            for (int ch=0; ch<nCh; ++ch){
                float* x = buffer.getWritePointer(ch) + pos;

                float s = x[n] * inTrim;
                // 4× OS if needed
                if (!needOS){
                    // ====== single-rate path ======
                    // Microphonic perturbation to V1 grid bias
                    const float mech = micro_.process(s, (float)fs_, micMech_);
                    V1_.Vbias += mech;

                    // Stage V1
                    const float a_inc1 = s;                // map audio sample to incident wave (scaled)
                    float v1 = V1_.process(a_inc1, (float)fs_);

                    // Coupling C1 (simple HP via WDF port R)
                    C1_.setInc(v1); float v1c = C1_.getRefl();

                    // Tone stack
                    float ts = tone_.process(v1c);

                    // Stage V2 (recovery)
                    const float a_inc2 = ts;
                    float v2 = V2_.process(a_inc2, (float)fs_);

                    // Coupling C2
                    C2_.setInc(v2); float v2c = C2_.getRefl();

                    // Stage V3 (driver/power)
                    const float a_inc3 = v2c;
                    float v3 = V3_.process(a_inc3, (float)fs_);

                    // OT + NFB
                    float y = ot_.process(v3, (float)fs_);
                    // Simple global NFB: subtract fraction of output from V2 input on next sample (one-sample delay implicit)
                    V2_.Vbias -= 0.02f * ot_.nfb * y;

                    // PSU sag from stage draw
                    const float I = V1_.plateCurrent() + V2_.plateCurrent() + 1.2f*V3_.plateCurrent();
                    rail_.step(I, (float)fs_);
                    V1_.Vplate = rail_.V; V2_.Vplate = rail_.V*0.95f; V3_.Vplate = rail_.V*0.9f;

                    // Ghost notes: weak comb around 60–120 Hz (speaker/room), modulated by ghost_
                    if (ghost_>0.f){
                        static float z=0.f; const float a=std::exp(-2.0f*float(M_PI)*8.0f/(float)fs_);
                        z = a*z + (1.f-a)*y;
                        y += 0.01f*ghost_*(y - z);
                    }

                    // Noise (hiss/hum)
                    if (noise_>0.f) y += white(0.00015f*noise_);

                    // DC + out trim
                    y = dc_[ch].process(y) * outTrim;

                    x[n] = y;
                } else {
                    // ====== 4× OS path ======
                    float upL[4], upR[4]; os4_.up4(s, s, upL, upR);

                    float acc=0.f;
                    for (int k=0;k<4;++k){
                        const float fs4 = (float)fs_ * 4.f;

                        const float mech = micro_.process(upL[k], fs4, micMech_);
                        V1_.Vbias += mech*0.25f; // distribute across substeps

                        float v1 = V1_.process(upL[k], fs4);
                        C1_.setInc(v1); float v1c = C1_.getRefl();
                        float ts = tone_.process(v1c);
                        float v2 = V2_.process(ts, fs4);
                        C2_.setInc(v2); float v2c = C2_.getRefl();
                        float v3 = V3_.process(v2c, fs4);
                        float y  = ot_.process(v3, fs4);
                        V2_.Vbias -= 0.02f*ot_.nfb*y*0.25f; // distribute

                        const float I = V1_.plateCurrent() + V2_.plateCurrent() + 1.2f*V3_.plateCurrent();
                        rail_.step(I*0.25f, fs4);
                        V1_.Vplate = rail_.V; V2_.Vplate = rail_.V*0.95f; V3_.Vplate = rail_.V*0.9f;

                        // ghost at OS rate
                        if (ghost_>0.f){
                            static float z4=0.f; const float a4=std::exp(-2.0f*float(M_PI)*32.0f/fs4);
                            z4 = a4*z4 + (1.f-a4)*y;
                            y += 0.008f*ghost_*(y - z4);
                        }
                        if (noise_>0.f) y += white(0.00004f*noise_);

                        acc += y;
                    }
                    float outL, outR; float ysum=acc*0.25f;
                    float packL[4]={ysum,ysum,ysum,ysum}, packR[4]={ysum,ysum,ysum,ysum};
                    os4_.down4(packL, packR, outL, outR);
                    float y = dc_[ch].process(outL) * outTrim;
                    x[n] = y;
                }
            }
        }

        pos += run;
    }

    scrubBuffer(buffer);
}