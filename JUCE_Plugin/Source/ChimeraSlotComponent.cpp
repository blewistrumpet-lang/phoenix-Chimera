#include "ChimeraSlotComponent.h"

//==============================================================================
// MilitaryKnob Implementation
//==============================================================================

MilitaryKnob::MilitaryKnob(const juce::String& paramName) 
    : parameterName(paramName)
{
    setSliderStyle(juce::Slider::RotaryVerticalDrag);
    setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    setPopupDisplayEnabled(false, false, nullptr);
}

void MilitaryKnob::paint(juce::Graphics& g)
{
    // Let the LookAndFeel draw the main knob
    getLookAndFeel().drawRotarySlider(g, 0, 0, getWidth(), getHeight(),
                                      static_cast<float>(valueToProportionOfLength(getValue())),
                                      static_cast<float>(juce::MathConstants<double>::pi * 1.2),
                                      static_cast<float>(juce::MathConstants<double>::pi * 2.8),
                                      *this);
    
    // Add hover glow effect
    if (isHovering)
    {
        g.setColour(ledColor.withAlpha(glowIntensity * 0.2f));
        g.fillEllipse(getLocalBounds().toFloat().expanded(5.0f));
    }
    
    // Draw parameter name below
    if (parameterName.isNotEmpty())
    {
        g.setFont(juce::Font("Arial Black", 10.0f, juce::Font::plain));
        g.setColour(juce::Colour(SkunkworksLookAndFeel::ColorScheme::textDimmed));
        g.drawText(parameterName, getLocalBounds().removeFromBottom(15), 
                  juce::Justification::centred);
    }
}

void MilitaryKnob::mouseEnter(const juce::MouseEvent& event)
{
    isHovering = true;
    glowIntensity = 1.0f;
    repaint();
}

void MilitaryKnob::mouseExit(const juce::MouseEvent& event)
{
    isHovering = false;
    glowIntensity = 0.0f;
    repaint();
}

//==============================================================================
// ChimeraSlotComponent Implementation
//==============================================================================

ChimeraSlotComponent::ChimeraSlotComponent(int slot, 
                                         juce::AudioProcessorValueTreeState& apvts,
                                         std::function<void(int)> onEngineChanged)
    : slotNumber(slot),
      valueTreeState(apvts),
      engineChangedCallback(onEngineChanged)
{
    // Slot label with military designation
    slotLabel.setText("SLOT-" + juce::String(slotNumber + 1).paddedLeft('0', 2), 
                     juce::dontSendNotification);
    slotLabel.setFont(juce::Font("Arial Black", 16.0f, juce::Font::bold));
    slotLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(slotLabel);
    
    // Engine name display
    engineNameLabel.setFont(juce::Font("Arial", 14.0f, juce::Font::plain));
    engineNameLabel.setJustificationType(juce::Justification::centred);
    engineNameLabel.setColour(juce::Label::textColourId, 
                             juce::Colour(SkunkworksLookAndFeel::ColorScheme::amberLED));
    addAndMakeVisible(engineNameLabel);
    
    // Engine selector
    juce::String slotStr = juce::String(slotNumber + 1);
    
    // Get engine choices from parameter
    if (auto* engineParam = dynamic_cast<juce::AudioParameterChoice*>(
        valueTreeState.getParameter("slot" + slotStr + "_engine")))
    {
        for (int i = 0; i < engineParam->choices.size(); ++i)
        {
            engineSelector.addItem(engineParam->choices[i], i + 1);
        }
    }
    
    engineSelector.onChange = [this]() {
        int newEngine = engineSelector.getSelectedId() - 1;
        setEngine(newEngine);
        if (engineChangedCallback)
            engineChangedCallback(newEngine);
    };
    addAndMakeVisible(engineSelector);
    
    // Bypass button with military styling
    bypassButton.setColour(juce::ToggleButton::textColourId, 
                          juce::Colour(SkunkworksLookAndFeel::ColorScheme::warningRed));
    addAndMakeVisible(bypassButton);
    
    // Menu button for additional options
    menuButton.setColour(juce::TextButton::textColourOffId,
                        juce::Colour(SkunkworksLookAndFeel::ColorScheme::textDimmed));
    addAndMakeVisible(menuButton);
    
    // Activity meter
    addAndMakeVisible(activityMeter);
    
    // Warning label (hidden by default)
    warningLabel.setColour(juce::Label::textColourId,
                          juce::Colour(SkunkworksLookAndFeel::ColorScheme::warningRed));
    warningLabel.setJustificationType(juce::Justification::centred);
    warningLabel.setVisible(false);
    addAndMakeVisible(warningLabel);
    
    // Create parameter knobs (15 max per slot)
    for (int i = 0; i < 15; ++i)
    {
        auto knob = std::make_unique<MilitaryKnob>();
        addAndMakeVisible(knob.get());
        paramKnobs.push_back(std::move(knob));
        
        auto label = std::make_unique<juce::Label>();
        label->setJustificationType(juce::Justification::centred);
        label->setFont(juce::Font(10.0f));
        addAndMakeVisible(label.get());
        paramLabels.push_back(std::move(label));
    }
    
    // Create APVTS attachments
    engineAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        valueTreeState, "slot" + slotStr + "_engine", engineSelector);
    
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        valueTreeState, "slot" + slotStr + "_bypass", bypassButton);
    
    for (int i = 0; i < 15; ++i)
    {
        auto attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            valueTreeState, "slot" + slotStr + "_param" + juce::String(i + 1), 
            *paramKnobs[i]);
        paramAttachments.push_back(std::move(attachment));
    }
    
    // Listen for engine changes
    valueTreeState.addParameterListener("slot" + slotStr + "_engine", this);
    
    // Start animation timer
    startTimerHz(30);
    
    // Initialize with current engine
    setEngine(engineSelector.getSelectedId() - 1);
}

ChimeraSlotComponent::~ChimeraSlotComponent()
{
    stopTimer();
    juce::String slotStr = juce::String(slotNumber + 1);
    valueTreeState.removeParameterListener("slot" + slotStr + "_engine", this);
}

void ChimeraSlotComponent::paint(juce::Graphics& g)
{
    drawSlotBackground(g);
    
    // Draw style-specific visuals
    switch (currentStyle)
    {
        case StyleDynamics:
            drawDynamicsStyle(g);
            break;
        case StyleDistortion:
            drawDistortionStyle(g);
            break;
        case StyleModulation:
            drawModulationStyle(g);
            break;
        case StyleTimeBased:
            drawTimeBasedStyle(g);
            break;
        case StyleSpectral:
            drawSpectralStyle(g);
            break;
        default:
            break;
    }
    
    drawStatusLEDs(g);
    
    // Warning flash overlay
    if (isWarning && warningFlash > 0.0f)
    {
        g.setColour(juce::Colour(SkunkworksLookAndFeel::ColorScheme::warningRed)
                   .withAlpha(warningFlash * 0.3f));
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 5.0f);
    }
    
    // Pulse animation overlay
    if (pulseAnimation > 0.0f)
    {
        g.setColour(getStyleColor().withAlpha(pulseAnimation * 0.2f));
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 5.0f);
    }
}

void ChimeraSlotComponent::resized()
{
    layoutControls();
}

void ChimeraSlotComponent::setEngine(int engineIndex)
{
    currentEngineIndex = engineIndex;
    
    // Determine style based on engine category
    juce::String category = getEngineCategory(engineIndex);
    
    if (category == "Dynamics") setSlotStyle(StyleDynamics);
    else if (category == "Distortion") setSlotStyle(StyleDistortion);
    else if (category == "Modulation") setSlotStyle(StyleModulation);
    else if (category == "TimeBased") setSlotStyle(StyleTimeBased);
    else if (category == "Spectral") setSlotStyle(StyleSpectral);
    else if (category == "Utility") setSlotStyle(StyleUtility);
    else setSlotStyle(StyleEmpty);
    
    // Update engine name display
    if (engineIndex > 0)
    {
        engineNameLabel.setText(engineSelector.getItemText(engineIndex), 
                               juce::dontSendNotification);
    }
    else
    {
        engineNameLabel.setText("[ EMPTY ]", juce::dontSendNotification);
    }
    
    updateParameterVisibility();
    repaint();
}

void ChimeraSlotComponent::setSlotStyle(SlotStyle style)
{
    currentStyle = style;
    
    // Update knob LED colors based on style
    auto styleColor = getStyleColor();
    for (auto& knob : paramKnobs)
    {
        knob->setLEDColor(styleColor);
    }
    
    repaint();
}

void ChimeraSlotComponent::updateParameterVisibility()
{
    // This would connect to the actual engine to get parameter count
    // For now, show different numbers based on engine type
    int numVisibleParams = (currentEngineIndex == 0) ? 0 : 8; // Example
    
    for (int i = 0; i < paramKnobs.size(); ++i)
    {
        bool visible = i < numVisibleParams;
        paramKnobs[i]->setVisible(visible);
        paramLabels[i]->setVisible(visible);
    }
}

void ChimeraSlotComponent::setProcessingLevel(float level)
{
    activityMeter.setLevel(level);
}

void ChimeraSlotComponent::setWarningState(bool hasWarning, const juce::String& message)
{
    isWarning = hasWarning;
    warningLabel.setText(message, juce::dontSendNotification);
    warningLabel.setVisible(hasWarning);
    
    if (hasWarning)
    {
        warningFlash = 1.0f;
    }
}

void ChimeraSlotComponent::pulseActivity()
{
    pulseAnimation = 1.0f;
}

void ChimeraSlotComponent::timerCallback()
{
    // Animate warning flash
    if (warningFlash > 0.0f)
    {
        warningFlash *= 0.95f;
        if (warningFlash < 0.01f) warningFlash = 0.0f;
    }
    
    // Animate activity pulse
    if (pulseAnimation > 0.0f)
    {
        pulseAnimation *= 0.9f;
        if (pulseAnimation < 0.01f) pulseAnimation = 0.0f;
    }
    
    repaint();
}

void ChimeraSlotComponent::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID.endsWith("_engine"))
    {
        juce::MessageManager::callAsync([this, newValue]() {
            setEngine(static_cast<int>(newValue));
        });
    }
}

void ChimeraSlotComponent::drawSlotBackground(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Main panel with metal texture
    if (auto* lnf = dynamic_cast<SkunkworksLookAndFeel*>(&getLookAndFeel()))
    {
        lnf->drawMetalPanel(g, bounds, false);
    }
    
    // Style-specific panel tinting
    if (currentStyle != StyleEmpty)
    {
        g.setColour(getStyleColor().withAlpha(0.05f));
        g.fillRoundedRectangle(bounds.reduced(2.0f), 3.0f);
    }
    
    // Corner screws
    if (auto* lnf = dynamic_cast<SkunkworksLookAndFeel*>(&getLookAndFeel()))
    {
        lnf->drawScrew(g, 8, 8, 8);
        lnf->drawScrew(g, bounds.getWidth() - 16, 8, 8);
        lnf->drawScrew(g, 8, bounds.getHeight() - 16, 8);
        lnf->drawScrew(g, bounds.getWidth() - 16, bounds.getHeight() - 16, 8);
    }
}

void ChimeraSlotComponent::drawStatusLEDs(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    auto ledArea = bounds.removeFromTop(30).removeFromRight(60).reduced(5);
    
    if (auto* lnf = dynamic_cast<SkunkworksLookAndFeel*>(&getLookAndFeel()))
    {
        // Power LED
        auto powerLED = ledArea.removeFromLeft(20);
        lnf->drawLEDIndicator(g, powerLED, !bypassButton.getToggleState(),
                            juce::Colour(SkunkworksLookAndFeel::ColorScheme::greenLED));
        
        // Warning LED
        ledArea.removeFromLeft(5);
        auto warningLED = ledArea.removeFromLeft(20);
        lnf->drawLEDIndicator(g, warningLED, isWarning,
                            juce::Colour(SkunkworksLookAndFeel::ColorScheme::warningRed));
    }
}

void ChimeraSlotComponent::layoutControls()
{
    auto bounds = getLocalBounds();
    
    // Top control strip
    auto topStrip = bounds.removeFromTop(35).reduced(10, 5);
    
    slotLabel.setBounds(topStrip.removeFromLeft(80));
    topStrip.removeFromLeft(10);
    
    engineSelector.setBounds(topStrip.removeFromLeft(150));
    topStrip.removeFromLeft(10);
    
    menuButton.setBounds(topStrip.removeFromRight(30));
    bypassButton.setBounds(topStrip.removeFromRight(80));
    
    // Engine name label
    bounds.removeFromTop(5);
    engineNameLabel.setBounds(bounds.removeFromTop(20).reduced(20, 0));
    
    // Activity meter on side
    activityMeter.setBounds(bounds.removeFromRight(15).reduced(2));
    
    // Parameter knobs in grid
    bounds.reduce(10, 10);
    int knobSize = 50;
    int knobSpacing = 5;
    int cols = 5;
    int rows = 3;
    
    for (int i = 0; i < paramKnobs.size(); ++i)
    {
        int row = i / cols;
        int col = i % cols;
        
        auto knobBounds = juce::Rectangle<int>(
            bounds.getX() + col * (knobSize + knobSpacing),
            bounds.getY() + row * (knobSize + knobSpacing + 15),
            knobSize, knobSize + 15
        );
        
        paramKnobs[i]->setBounds(knobBounds.removeFromTop(knobSize));
        paramLabels[i]->setBounds(knobBounds);
    }
    
    // Warning label at bottom if visible
    if (warningLabel.isVisible())
    {
        warningLabel.setBounds(getLocalBounds().removeFromBottom(20).reduced(10, 2));
    }
}

juce::Colour ChimeraSlotComponent::getStyleColor() const
{
    switch (currentStyle)
    {
        case StyleDynamics: return juce::Colour(0xff0088ff); // Blue
        case StyleDistortion: return juce::Colour(0xffff4400); // Orange
        case StyleModulation: return juce::Colour(0xff00ff88); // Green
        case StyleTimeBased: return juce::Colour(0xff8844ff); // Purple
        case StyleSpectral: return juce::Colour(0xff00ffff); // Cyan
        case StyleUtility: return juce::Colour(0xff888888); // Grey
        default: return juce::Colour(0xff444444); // Dark grey
    }
}

juce::String ChimeraSlotComponent::getEngineCategory(int engineIndex) const
{
    // Map engine indices to categories
    // This would ideally come from a central engine registry
    if (engineIndex <= 0) return "Empty";
    if (engineIndex <= 10) return "Dynamics";
    if (engineIndex <= 20) return "Distortion";
    if (engineIndex <= 30) return "Modulation";
    if (engineIndex <= 40) return "TimeBased";
    if (engineIndex <= 50) return "Spectral";
    return "Utility";
}

void ChimeraSlotComponent::drawDynamicsStyle(juce::Graphics& g)
{
    // VU meter visualization for dynamics processors
    auto bounds = getLocalBounds().reduced(15, 60).removeFromBottom(10);
    
    g.setColour(juce::Colours::black);
    g.fillRoundedRectangle(bounds.toFloat(), 2.0f);
    
    // Meter segments
    float level = activityMeter.level;
    int numSegments = 20;
    float segmentWidth = bounds.getWidth() / float(numSegments);
    
    for (int i = 0; i < numSegments * level; ++i)
    {
        auto segBounds = bounds.removeFromLeft(segmentWidth - 1);
        
        if (i < numSegments * 0.6f)
            g.setColour(juce::Colour(SkunkworksLookAndFeel::ColorScheme::greenLED));
        else if (i < numSegments * 0.85f)
            g.setColour(juce::Colour(SkunkworksLookAndFeel::ColorScheme::amberLED));
        else
            g.setColour(juce::Colour(SkunkworksLookAndFeel::ColorScheme::warningRed));
        
        g.fillRect(segBounds);
        bounds.removeFromLeft(1);
    }
}

void ChimeraSlotComponent::drawDistortionStyle(juce::Graphics& g)
{
    // Heat indicator for distortion
    if (activityMeter.level > 0.7f)
    {
        auto bounds = getLocalBounds().toFloat();
        g.setColour(juce::Colour(SkunkworksLookAndFeel::ColorScheme::warningRed)
                   .withAlpha(0.1f * (activityMeter.level - 0.7f)));
        g.fillRoundedRectangle(bounds, 5.0f);
    }
}

void ChimeraSlotComponent::drawModulationStyle(juce::Graphics& g)
{
    // Oscilloscope visualization
    auto bounds = getLocalBounds().reduced(15, 60).removeFromBottom(20);
    
    g.setColour(juce::Colours::black);
    g.fillRoundedRectangle(bounds.toFloat(), 2.0f);
    
    // Draw modulation waveform
    juce::Path wave;
    float phase = juce::Time::getMillisecondCounterHiRes() * 0.001f;
    
    for (int x = 0; x < bounds.getWidth(); ++x)
    {
        float y = bounds.getCentreY() + 
                 std::sin(phase + x * 0.05f) * bounds.getHeight() * 0.4f;
        
        if (x == 0)
            wave.startNewSubPath(bounds.getX() + x, y);
        else
            wave.lineTo(bounds.getX() + x, y);
    }
    
    g.setColour(getStyleColor());
    g.strokePath(wave, juce::PathStrokeType(1.5f));
}

void ChimeraSlotComponent::drawTimeBasedStyle(juce::Graphics& g)
{
    // Delay tap visualization
    auto bounds = getLocalBounds().reduced(15, 60).removeFromBottom(15);
    
    for (int i = 0; i < 4; ++i)
    {
        auto tapBounds = bounds.removeFromLeft(bounds.getWidth() / 4).reduced(2).toFloat();
        
        float alpha = 1.0f - (i * 0.25f);
        g.setColour(getStyleColor().withAlpha(alpha * 0.5f));
        g.fillRoundedRectangle(tapBounds, 2.0f);
    }
}

void ChimeraSlotComponent::drawSpectralStyle(juce::Graphics& g)
{
    // Spectrum analyzer bars
    auto bounds = getLocalBounds().reduced(15, 60).removeFromBottom(20);
    
    g.setColour(juce::Colours::black);
    g.fillRoundedRectangle(bounds.toFloat(), 2.0f);
    
    juce::Random rand;
    int numBars = 16;
    float barWidth = bounds.getWidth() / float(numBars);
    
    for (int i = 0; i < numBars; ++i)
    {
        auto barBounds = bounds.removeFromLeft(barWidth - 1).toFloat();
        float height = rand.nextFloat() * 0.8f + 0.2f;
        barBounds = barBounds.removeFromBottom(barBounds.getHeight() * height);
        
        g.setColour(getStyleColor());
        g.fillRect(barBounds);
        bounds.removeFromLeft(1);
    }
}

//==============================================================================
// ActivityMeter Implementation
//==============================================================================

void ChimeraSlotComponent::ActivityMeter::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Background
    g.setColour(juce::Colours::black);
    g.fillRoundedRectangle(bounds, 2.0f);
    
    // Level bar
    if (level > 0.01f)
    {
        auto levelBounds = bounds.reduced(2.0f);
        levelBounds = levelBounds.removeFromBottom(levelBounds.getHeight() * level);
        
        if (level < 0.6f)
            g.setColour(juce::Colour(SkunkworksLookAndFeel::ColorScheme::greenLED));
        else if (level < 0.85f)
            g.setColour(juce::Colour(SkunkworksLookAndFeel::ColorScheme::amberLED));
        else
            g.setColour(juce::Colour(SkunkworksLookAndFeel::ColorScheme::warningRed));
        
        g.fillRoundedRectangle(levelBounds, 1.0f);
    }
}