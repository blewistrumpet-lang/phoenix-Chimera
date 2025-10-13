#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

/**
 * TrinityWebEditor - HTML5-based UI for Raspberry Pi
 *
 * Uses JUCE WebBrowserComponent to display trinity_ui.html
 * Communicates with plugin via JavaScript bridge
 */
class TrinityWebEditor : public juce::AudioProcessorEditor
{
public:
    TrinityWebEditor(ChimeraAudioProcessor& processor);
    ~TrinityWebEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    ChimeraAudioProcessor& audioProcessor;

    std::unique_ptr<juce::WebBrowserComponent> webView;

    // JavaScript integration object
    class JuceIntegration : public juce::DynamicObject
    {
    public:
        JuceIntegration(ChimeraAudioProcessor& proc) : processor(proc)
        {
            // Register JavaScript callable methods
            setMethod("setParameter", [this](const juce::var::NativeFunctionArgs& args) -> juce::var {
                if (args.numArguments >= 2)
                {
                    juce::String param = args.arguments[0].toString();
                    float value = (float)args.arguments[1];
                    handleParameterChange(param, value);
                }
                return juce::var();
            });

            setMethod("triggerVoice", [this](const juce::var::NativeFunctionArgs& args) -> juce::var {
                if (args.numArguments >= 1)
                {
                    juce::String gesture = args.arguments[0].toString();
                    handleVoiceGesture(gesture);
                }
                return juce::var();
            });

            setMethod("updateChain", [this](const juce::var::NativeFunctionArgs& args) -> juce::var {
                if (args.numArguments >= 2)
                {
                    int slot = (int)args.arguments[0];
                    bool active = (bool)args.arguments[1];
                    handleChainUpdate(slot, active);
                }
                return juce::var();
            });
        }

    private:
        ChimeraAudioProcessor& processor;

        void handleParameterChange(const juce::String& param, float value)
        {
            DBG("Parameter changed: " << param << " = " << value);

            if (param == "filter")
            {
                // Map to actual filter parameter
                if (auto* filterParam = processor.getValueTreeState().getParameter("FilterFreq"))
                    filterParam->setValueNotifyingHost(value);
            }
            else if (param == "mix")
            {
                // Map to mix parameter
                if (auto* mixParam = processor.getValueTreeState().getParameter("Mix"))
                    mixParam->setValueNotifyingHost(value);
            }
            else if (param == "preset")
            {
                // Handle preset change
                int presetNum = (int)(value * 500);
                DBG("Loading preset: " << presetNum);
            }
            else if (param == "voice_mode")
            {
                DBG("Voice mode: " << (int)value);
            }
            else if (param == "engine_mode")
            {
                DBG("Engine mode: " << (int)value);
            }
            else if (param == "ab_state")
            {
                DBG("A/B state: " << (int)value);
            }
        }

        void handleVoiceGesture(const juce::String& gesture)
        {
            DBG("Voice gesture: " << gesture);

            if (gesture == "tap")
            {
                // Start voice recording
            }
            else if (gesture == "hold")
            {
                // Tap tempo mode
            }
            else if (gesture == "double_tap")
            {
                // Panic/reset all
            }
        }

        void handleChainUpdate(int slot, bool active)
        {
            DBG("Chain slot " << slot << " active: " << (active ? "true" : "false"));
            // Update signal chain
        }
    };

    std::unique_ptr<JuceIntegration> juceIntegration;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrinityWebEditor)
};
