#pragma once
#include <JuceHeader.h>
#include "EngineBase.h"
#include "TestSignalGenerator.h"
#include "AudioMeasurements.h"

/**
 * Visual Test Component for Engine Analysis
 * Provides real-time visualization of engine behavior
 */
class EngineTestVisualizer : public juce::Component, 
                             public juce::Timer {
public:
    EngineTestVisualizer();
    ~EngineTestVisualizer() override;
    
    // Set the engine to test
    void setEngine(EngineBase* engine);
    
    // Test controls
    void startTest();
    void stopTest();
    bool isTestRunning() const { return m_isRunning; }
    
    // Test signal selection
    enum TestSignalType {
        Sine1k,
        WhiteNoise,
        PinkNoise,
        Impulse,
        Sweep,
        Square,
        Silence,
        DrumHit,
        Chord
    };
    
    void setTestSignal(TestSignalType type) { m_signalType = type; }
    
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;
    
private:
    // Sub-components
    class WaveformDisplay : public juce::Component {
    public:
        void setData(const juce::AudioBuffer<float>& buffer);
        void paint(juce::Graphics& g) override;
    private:
        std::vector<float> m_samples;
    };
    
    class SpectrumDisplay : public juce::Component {
    public:
        void setData(const std::vector<float>& magnitudes, float sampleRate);
        void paint(juce::Graphics& g) override;
    private:
        std::vector<float> m_magnitudes;
        float m_sampleRate = 48000.0f;
    };
    
    class MeasurementPanel : public juce::Component {
    public:
        MeasurementPanel();
        void updateMeasurements(float rms, float peak, float thd, float snr, float cpu);
        void paint(juce::Graphics& g) override;
    private:
        juce::Label m_rmsLabel, m_peakLabel, m_thdLabel, m_snrLabel, m_cpuLabel;
        float m_rms = 0, m_peak = 0, m_thd = 0, m_snr = 0, m_cpu = 0;
    };
    
    class SpectrogramDisplay : public juce::Component {
    public:
        SpectrogramDisplay();
        void addSpectrum(const std::vector<float>& magnitudes);
        void paint(juce::Graphics& g) override;
        void clear();
    private:
        static constexpr int MAX_HISTORY = 200;
        std::vector<std::vector<float>> m_history;
        juce::Image m_spectrogramImage;
        void updateImage();
    };
    
    // Visual components
    std::unique_ptr<WaveformDisplay> m_inputWaveform;
    std::unique_ptr<WaveformDisplay> m_outputWaveform;
    std::unique_ptr<SpectrumDisplay> m_inputSpectrum;
    std::unique_ptr<SpectrumDisplay> m_outputSpectrum;
    std::unique_ptr<MeasurementPanel> m_measurements;
    std::unique_ptr<SpectrogramDisplay> m_spectrogram;
    
    // Controls
    juce::ComboBox m_signalSelector;
    juce::TextButton m_startStopButton;
    juce::TextButton m_clearButton;
    juce::Label m_engineNameLabel;
    
    // Test state
    EngineBase* m_currentEngine = nullptr;
    TestSignalType m_signalType = Sine1k;
    bool m_isRunning = false;
    
    // Audio buffers
    juce::AudioBuffer<float> m_testSignal;
    juce::AudioBuffer<float> m_processedSignal;
    
    // Processing
    void generateTestSignal();
    void processAndAnalyze();
    void updateVisualizations();
    
    // Measurement tracking
    float m_lastCPU = 0.0f;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EngineTestVisualizer)
};