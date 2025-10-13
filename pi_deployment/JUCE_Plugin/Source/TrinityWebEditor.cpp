#include "TrinityWebEditor.h"

TrinityWebEditor::TrinityWebEditor(ChimeraAudioProcessor& processor)
    : AudioProcessorEditor(&processor), audioProcessor(processor)
{
    setSize(480, 320);

    // Create WebBrowserComponent
    webView = std::make_unique<juce::WebBrowserComponent>();
    addAndMakeVisible(webView.get());

    // Find the HTML file
    juce::File htmlFile = juce::File::getSpecialLocation(juce::File::currentExecutableFile)
                             .getParentDirectory()
                             .getChildFile("trinity_ui.html");

    // Fallback: check in Source directory for development
    if (!htmlFile.existsAsFile())
    {
        htmlFile = juce::File(__FILE__).getParentDirectory().getChildFile("trinity_ui.html");
    }

    // Create JavaScript integration
    juceIntegration = std::make_unique<JuceIntegration>(audioProcessor);

    if (htmlFile.existsAsFile())
    {
        DBG("Loading Trinity UI from: " << htmlFile.getFullPathName());
        webView->goToURL(htmlFile.getFullPathName());

        // Inject JavaScript bridge after page loads
        juce::Timer::callAfterDelay(500, [this]() {
            if (webView != nullptr)
            {
                // Inject the juce object into the web page
                webView->evaluateJavascript(
                    "window.juce = {}; "
                    "window.juce.setParameter = function(param, value) { "
                    "  console.log('setParameter:', param, value); "
                    "}; "
                    "window.juce.triggerVoice = function(gesture) { "
                    "  console.log('triggerVoice:', gesture); "
                    "}; "
                    "window.juce.updateChain = function(slot, active) { "
                    "  console.log('updateChain:', slot, active); "
                    "};"
                );
            }
        });
    }
    else
    {
        DBG("ERROR: trinity_ui.html not found at: " << htmlFile.getFullPathName());
    }
}

TrinityWebEditor::~TrinityWebEditor()
{
}

void TrinityWebEditor::paint(juce::Graphics& g)
{
    // Fallback if HTML doesn't load
    if (webView == nullptr || !webView->isVisible())
    {
        g.fillAll(juce::Colours::black);
        g.setColour(juce::Colours::red);
        g.setFont(14.0f);
        g.drawText("Trinity UI - HTML not loaded", getLocalBounds(), juce::Justification::centred);
    }
}

void TrinityWebEditor::resized()
{
    if (webView != nullptr)
    {
        webView->setBounds(getLocalBounds());
    }
}
