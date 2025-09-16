#include "PluginEditorNexusDynamic.h"
#include "EngineTypes.h"
#include "EngineBase.h"
#include <cmath>

//==============================================================================
// Main Editor
//==============================================================================
PluginEditorNexusDynamic::PluginEditorNexusDynamic(ChimeraAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    // Set up custom look and feel
    nexusLnF = std::make_unique<NexusLookAndFeelDynamic>();
    setLookAndFeel(nexusLnF.get());
    
    // Set default size (1200x800) and make resizable
    setSize(1200, 800);
    setResizable(true, true);
    setResizeLimits(1000, 700, 1600, 1200);
    
    // Create AI Command Panel (left column)
    aiPanel = std::make_unique<AICommandPanel>(audioProcessor);
    addAndMakeVisible(aiPanel.get());
    
    // Create 6 engine slots (right column, 2x3 grid)
    for (int i = 0; i < 6; ++i)
    {
        engineSlots[i] = std::make_unique<DynamicEngineSlot>(audioProcessor, i);
        addAndMakeVisible(engineSlots[i].get());
    }
    
    // Create master panel (bottom)
    masterPanel = std::make_unique<MasterPanel>(audioProcessor);
    addAndMakeVisible(masterPanel.get());
    
    // Don't start timer in constructor - wait for component to be visible
    // startTimerHz(30);
}

PluginEditorNexusDynamic::~PluginEditorNexusDynamic()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void PluginEditorNexusDynamic::parentHierarchyChanged()
{
    // Start timer when we have a parent window
    if (getParentComponent() != nullptr && !isTimerRunning())
    {
        startTimerHz(30);
    }
}

void PluginEditorNexusDynamic::visibilityChanged()
{
    if (isVisible() && !isTimerRunning())
    {
        startTimerHz(30);
    }
    else if (!isVisible() && isTimerRunning())
    {
        stopTimer();
    }
}

void PluginEditorNexusDynamic::paint(juce::Graphics& g)
{
    // Draw carbon fiber background
    drawCarbonFiberBackground(g);
    
    // Draw holographic overlay with scanline
    drawHolographicOverlay(g);
}

void PluginEditorNexusDynamic::resized()
{
    auto bounds = getLocalBounds();
    
    // Reserve bottom 100px for master section
    auto masterArea = bounds.removeFromBottom(100);
    masterPanel->setBounds(masterArea.reduced(10));
    
    // Split remaining area: 40% left for AI, 60% right for engines
    auto aiArea = bounds.removeFromLeft(bounds.getWidth() * 0.4f);
    aiPanel->setBounds(aiArea.reduced(10));
    
    // Arrange engine slots in 2x3 grid
    auto slotArea = bounds.reduced(10);
    int slotWidth = slotArea.getWidth() / 2;
    int slotHeight = slotArea.getHeight() / 3;
    
    for (int i = 0; i < 6; ++i)
    {
        int col = i % 2;
        int row = i / 2;
        
        auto slotBounds = juce::Rectangle<int>(
            slotArea.getX() + col * slotWidth,
            slotArea.getY() + row * slotHeight,
            slotWidth - 5,
            slotHeight - 5
        );
        
        engineSlots[i]->setBounds(slotBounds);
    }
}

void PluginEditorNexusDynamic::timerCallback()
{
    // Update scanline position
    scanlineY += 2.0f;
    if (scanlineY > getHeight()) scanlineY = 0.0f;
    
    // Update glow pulse
    glowPulse = 0.5f + 0.5f * std::sin(juce::Time::getMillisecondCounter() * 0.001f);
    
    // Update meters if panel exists
    if (masterPanel)
        masterPanel->updateMeters();
    
    // Trigger repaint for animation
    repaint();
}

void PluginEditorNexusDynamic::drawCarbonFiberBackground(juce::Graphics& g)
{
    // Dark charcoal base
    g.fillAll(juce::Colour(0xff1F2937));
    
    // Carbon fiber texture pattern
    for (int y = 0; y < getHeight(); y += 4)
    {
        for (int x = 0; x < getWidth(); x += 4)
        {
            if ((x / 4 + y / 4) % 2 == 0)
            {
                g.setColour(juce::Colour(0xff111827));
                g.fillRect(x, y, 2, 2);
            }
        }
    }
}

void PluginEditorNexusDynamic::drawHolographicOverlay(juce::Graphics& g)
{
    // Animated scanline
    g.setColour(juce::Colour(0xff00ffcc).withAlpha(0.1f));
    g.fillRect(0.0f, scanlineY - 20.0f, (float)getWidth(), 40.0f);
    
    // Glowing edges
    juce::ColourGradient edgeGlow(
        juce::Colour(0xff00ffcc).withAlpha(0.2f * glowPulse),
        0, 0,
        juce::Colour(0x00000000),
        50, 0,
        false
    );
    g.setGradientFill(edgeGlow);
    g.fillRect(0, 0, 50, getHeight());
    
    edgeGlow.point1 = {(float)getWidth(), 0};
    edgeGlow.point2 = {(float)getWidth() - 50, 0};
    g.setGradientFill(edgeGlow);
    g.fillRect(getWidth() - 50, 0, 50, getHeight());
}

//==============================================================================
// Dynamic Engine Slot
//==============================================================================
PluginEditorNexusDynamic::DynamicEngineSlot::DynamicEngineSlot(ChimeraAudioProcessor& p, int slotIndex)
    : processor(p), slot(slotIndex)
{
    // Slot label
    slotLabel.setText("SLOT " + juce::String(slot + 1), juce::dontSendNotification);
    slotLabel.setJustificationType(juce::Justification::centred);
    slotLabel.setColour(juce::Label::textColourId, juce::Colour(0xff00ffcc));
    addAndMakeVisible(slotLabel);
    
    // Engine selector
    engineSelector.addItem("NONE", 1);
    
    // Add all available engines
    // Get engine names from the parameter choices
    auto* engineParam = processor.getValueTreeState().getParameter(
        "slot" + juce::String(slot + 1) + "_engine");
    if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(engineParam))
    {
        auto choices = choiceParam->choices;
        for (int i = 0; i < choices.size(); ++i)
        {
            engineSelector.addItem(choices[i], i + 1);
        }
    }
    
    engineSelector.onChange = [this] {
        updateParametersFromLiveEngine();
    };
    addAndMakeVisible(engineSelector);
    
    // Bypass button
    bypassButton.setButtonText("BYPASS");
    bypassButton.setColour(juce::ToggleButton::textColourId, juce::Colour(0xffff006e));
    addAndMakeVisible(bypassButton);
    
    // Attach to value tree
    juce::String slotPrefix = "slot" + juce::String(slot + 1) + "_";
    
    engineAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processor.getValueTreeState(), slotPrefix + "engine", engineSelector);
    
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processor.getValueTreeState(), slotPrefix + "bypass", bypassButton);
    
    // Initialize with current engine
    updateParametersFromLiveEngine();
}

PluginEditorNexusDynamic::DynamicEngineSlot::~DynamicEngineSlot() = default;

void PluginEditorNexusDynamic::DynamicEngineSlot::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    
    // Draw holographic panel background
    g.setColour(juce::Colour(0xff111827));
    g.fillRoundedRectangle(bounds.toFloat(), 5.0f);
    
    // Draw glowing border based on activity
    g.setColour(juce::Colour(0xff00ffcc).withAlpha(0.3f + activityLevel * 0.7f));
    g.drawRoundedRectangle(bounds.toFloat().reduced(1), 5.0f, 2.0f);
    
    // Draw corner brackets
    g.setColour(juce::Colour(0xff00ffcc));
    auto corner = 15.0f;
    auto thick = 2.0f;
    
    // Top-left
    g.fillRect((float)bounds.getX(), (float)bounds.getY(), corner, thick);
    g.fillRect((float)bounds.getX(), (float)bounds.getY(), thick, corner);
    
    // Top-right
    g.fillRect((float)bounds.getRight() - corner, (float)bounds.getY(), corner, thick);
    g.fillRect((float)bounds.getRight() - thick, (float)bounds.getY(), thick, corner);
    
    // Bottom-left
    g.fillRect((float)bounds.getX(), (float)bounds.getBottom() - thick, corner, thick);
    g.fillRect((float)bounds.getX(), (float)bounds.getBottom() - corner, thick, corner);
    
    // Bottom-right
    g.fillRect((float)bounds.getRight() - corner, (float)bounds.getBottom() - thick, corner, thick);
    g.fillRect((float)bounds.getRight() - thick, (float)bounds.getBottom() - corner, thick, corner);
}

void PluginEditorNexusDynamic::DynamicEngineSlot::resized()
{
    auto bounds = getLocalBounds().reduced(10);
    
    // Top section: label, selector, bypass
    auto topSection = bounds.removeFromTop(60);
    slotLabel.setBounds(topSection.removeFromTop(25));
    
    auto controlRow = topSection;
    engineSelector.setBounds(controlRow.removeFromLeft(controlRow.getWidth() * 0.7f).reduced(2));
    bypassButton.setBounds(controlRow.reduced(2));
    
    // Remaining area for parameters
    auto paramArea = bounds.reduced(5);
    
    if (!dynamicParams.empty())
    {
        // Calculate optimal grid layout
        int numParams = (int)dynamicParams.size();
        int cols = (numParams <= 4) ? 2 : 3;
        int rows = (numParams + cols - 1) / cols;
        
        int paramWidth = paramArea.getWidth() / cols;
        int paramHeight = std::min(80, paramArea.getHeight() / rows);
        
        for (int i = 0; i < numParams; ++i)
        {
            int col = i % cols;
            int row = i / cols;
            
            auto paramBounds = juce::Rectangle<int>(
                paramArea.getX() + col * paramWidth,
                paramArea.getY() + row * paramHeight,
                paramWidth - 5,
                paramHeight - 5
            );
            
            auto& param = dynamicParams[i];
            
            // Label at top
            auto labelBounds = paramBounds.removeFromTop(20);
            if (param.label) param.label->setBounds(labelBounds);
            
            // Control below
            if (param.control) param.control->setBounds(paramBounds);
        }
    }
}

void PluginEditorNexusDynamic::DynamicEngineSlot::updateParametersFromLiveEngine()
{
    // Clear existing parameters
    for (auto& param : dynamicParams)
    {
        if (param.control) removeChildComponent(param.control.get());
        if (param.label) removeChildComponent(param.label.get());
    }
    dynamicParams.clear();
    
    // Get selected engine ID
    int selectedIndex = engineSelector.getSelectedId();
    if (selectedIndex <= 1) return; // NONE selected
    
    int engineId = selectedIndex - 1;
    
    // **CRITICAL: Query the LIVE engine instance**
    auto& engine = processor.getEngine(slot);
    if (!engine) return;
    
    // Get parameter count from LIVE engine
    int numParams = engine->getNumParameters();
    
    juce::String slotPrefix = "slot" + juce::String(slot + 1) + "_param";
    
    // Create controls for each parameter
    for (int i = 0; i < numParams && i < 15; ++i)
    {
        DynamicParam param;
        
        // Get parameter name from LIVE engine
        param.name = engine->getParameterName(i);
        param.isToggle = shouldBeToggle(param.name);
        
        // Create label
        param.label = std::make_unique<juce::Label>();
        param.label->setText(param.name, juce::dontSendNotification);
        param.label->setJustificationType(juce::Justification::centred);
        param.label->setColour(juce::Label::textColourId, juce::Colour(0xffE5E7EB));
        addAndMakeVisible(param.label.get());
        
        juce::String paramId = slotPrefix + juce::String(i + 1);
        
        if (param.isToggle)
        {
            // Create toggle button
            auto toggle = std::make_unique<juce::ToggleButton>();
            toggle->setColour(juce::ToggleButton::textColourId, juce::Colour(0xff00ffcc));
            param.buttonAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
                processor.getValueTreeState(), paramId, *toggle);
            param.control = std::move(toggle);
        }
        else
        {
            // Create rotary slider
            auto slider = std::make_unique<juce::Slider>();
            slider->setSliderStyle(juce::Slider::RotaryVerticalDrag);
            slider->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            slider->setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff00ffcc));
            slider->setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff1F2937));
            param.sliderAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                processor.getValueTreeState(), paramId, *slider);
            param.control = std::move(slider);
        }
        
        addAndMakeVisible(param.control.get());
        dynamicParams.push_back(std::move(param));
    }
    
    resized();
}

bool PluginEditorNexusDynamic::DynamicEngineSlot::shouldBeToggle(const juce::String& paramName) const
{
    juce::String lower = paramName.toLowerCase();
    return lower.contains("bypass") || lower.contains("enable") || 
           lower.contains("on/off") || lower.contains("freeze") ||
           lower.contains("reverse") || lower.contains("sync") ||
           lower.contains("phase") || lower.contains("swap") ||
           lower.contains("auto") || lower.contains("dc filter");
}

//==============================================================================
// AI Command Panel
//==============================================================================
PluginEditorNexusDynamic::AICommandPanel::AICommandPanel(ChimeraAudioProcessor& p)
    : processor(p)
{
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setColour(juce::Label::textColourId, juce::Colour(0xff00ffcc));
    titleLabel.setFont(juce::Font(24.0f).boldened());
    addAndMakeVisible(titleLabel);
    
    promptInput.setMultiLine(true);
    promptInput.setReturnKeyStartsNewLine(true);
    promptInput.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff111827));
    promptInput.setColour(juce::TextEditor::textColourId, juce::Colour(0xffE5E7EB));
    promptInput.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff00ffcc));
    addAndMakeVisible(promptInput);
    
    executeBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff00ffcc));
    executeBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff111827));
    addAndMakeVisible(executeBtn);
    
    enhanceBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffff006e));
    enhanceBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffE5E7EB));
    addAndMakeVisible(enhanceBtn);
    
    randomBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1F2937));
    randomBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff00ffcc));
    addAndMakeVisible(randomBtn);
    
    // Pipeline status lights
    for (auto& light : statusLights)
    {
        addAndMakeVisible(light);
    }
}

void PluginEditorNexusDynamic::AICommandPanel::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    
    // Holographic panel background
    g.setColour(juce::Colour(0xff111827).withAlpha(0.9f));
    g.fillRoundedRectangle(bounds.toFloat(), 10.0f);
    
    // Glowing border
    g.setColour(juce::Colour(0xff00ffcc).withAlpha(0.5f));
    g.drawRoundedRectangle(bounds.toFloat().reduced(1), 10.0f, 2.0f);
    
    // Draw pipeline status
    auto statusArea = bounds.removeFromBottom(40).reduced(10);
    for (int i = 0; i < 4; ++i)
    {
        auto lightBounds = statusArea.removeFromLeft(statusArea.getWidth() / 4).reduced(5).toFloat();
        
        // Pipeline stage active?
        bool active = (i == 0); // For demo, first stage always active
        g.setColour(active ? juce::Colour(0xff00ffcc) : juce::Colour(0xff1F2937));
        g.fillEllipse(lightBounds);
        
        if (active)
        {
            // Add glow effect
            g.setColour(juce::Colour(0xff00ffcc).withAlpha(0.3f));
            g.fillEllipse(lightBounds.expanded(3));
        }
    }
}

void PluginEditorNexusDynamic::AICommandPanel::resized()
{
    auto bounds = getLocalBounds().reduced(15);
    
    titleLabel.setBounds(bounds.removeFromTop(40));
    bounds.removeFromTop(10);
    
    promptInput.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.6f));
    bounds.removeFromTop(10);
    
    auto buttonArea = bounds.removeFromTop(40);
    int buttonWidth = buttonArea.getWidth() / 3 - 5;
    
    executeBtn.setBounds(buttonArea.removeFromLeft(buttonWidth));
    buttonArea.removeFromLeft(5);
    enhanceBtn.setBounds(buttonArea.removeFromLeft(buttonWidth));
    buttonArea.removeFromLeft(5);
    randomBtn.setBounds(buttonArea);
}

//==============================================================================
// Master Panel
//==============================================================================
PluginEditorNexusDynamic::MasterPanel::MasterPanel(ChimeraAudioProcessor& p)
    : processor(p)
{
    addAndMakeVisible(inputMeter);
    addAndMakeVisible(outputMeter);
    
    inputGain.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    inputGain.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    inputGain.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff00ffcc));
    addAndMakeVisible(inputGain);
    
    outputGain.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    outputGain.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    outputGain.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff00ffcc));
    addAndMakeVisible(outputGain);
    
    mixControl.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    mixControl.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    mixControl.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xffff006e));
    addAndMakeVisible(mixControl);
    
    // Attach to parameters - check if they exist first
    if (processor.getValueTreeState().getParameter("inputGain"))
        inputAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.getValueTreeState(), "inputGain", inputGain);
    
    if (processor.getValueTreeState().getParameter("outputGain"))
        outputAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.getValueTreeState(), "outputGain", outputGain);
    
    if (processor.getValueTreeState().getParameter("globalMix"))
        mixAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.getValueTreeState(), "globalMix", mixControl);
}

void PluginEditorNexusDynamic::MasterPanel::paint(juce::Graphics& g)
{
    // Background panel
    g.setColour(juce::Colour(0xff111827));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 5.0f);
    
    // Labels
    g.setColour(juce::Colour(0xffE5E7EB));
    g.setFont(12.0f);
    
    auto bounds = getLocalBounds();
    auto meterArea = bounds.removeFromLeft(200);
    g.drawText("INPUT", meterArea.removeFromLeft(100), juce::Justification::centredTop);
    g.drawText("OUTPUT", meterArea, juce::Justification::centredTop);
    
    auto knobArea = bounds;
    int knobWidth = knobArea.getWidth() / 3;
    g.drawText("INPUT", knobArea.removeFromLeft(knobWidth), juce::Justification::centredTop);
    g.drawText("OUTPUT", knobArea.removeFromLeft(knobWidth), juce::Justification::centredTop);
    g.drawText("MIX", knobArea, juce::Justification::centredTop);
}

void PluginEditorNexusDynamic::MasterPanel::resized()
{
    auto bounds = getLocalBounds().reduced(5);
    
    // Meters on left
    auto meterArea = bounds.removeFromLeft(200);
    inputMeter.setBounds(meterArea.removeFromLeft(100).reduced(10, 20));
    outputMeter.setBounds(meterArea.reduced(10, 20));
    
    // Knobs on right
    int knobSize = 70;
    int knobSpacing = bounds.getWidth() / 3;
    
    inputGain.setBounds(bounds.getX(), bounds.getCentreY() - knobSize/2, knobSize, knobSize);
    outputGain.setBounds(bounds.getX() + knobSpacing, bounds.getCentreY() - knobSize/2, knobSize, knobSize);
    mixControl.setBounds(bounds.getX() + knobSpacing * 2, bounds.getCentreY() - knobSize/2, knobSize, knobSize);
}

void PluginEditorNexusDynamic::MasterPanel::updateMeters()
{
    // Safe meter update with bounds checking
    float inputLevel = processor.getCurrentInputLevel();
    float outputLevel = processor.getCurrentOutputLevel();
    
    inputMeter.setLevel(juce::jlimit(0.0f, 1.0f, inputLevel));
    outputMeter.setLevel(juce::jlimit(0.0f, 1.0f, outputLevel));
}

void PluginEditorNexusDynamic::MasterPanel::LevelMeter::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Background
    g.setColour(juce::Colour(0xff111827));
    g.fillRoundedRectangle(bounds, 2.0f);
    
    // Level bar
    float displayLevel = juce::jmin(1.0f, level);
    auto levelBounds = bounds.reduced(2);
    levelBounds.removeFromTop(levelBounds.getHeight() * (1.0f - displayLevel));
    
    // Color based on level
    if (displayLevel > 0.9f)
        g.setColour(juce::Colour(0xffff006e)); // Red for clipping
    else if (displayLevel > 0.7f)
        g.setColour(juce::Colour(0xffffcc00)); // Yellow for hot
    else
        g.setColour(juce::Colour(0xff00ffcc)); // Cyan for normal
    
    g.fillRoundedRectangle(levelBounds, 2.0f);
    
    // Peak indicator
    if (peakLevel > 0.0f)
    {
        float peakY = bounds.getY() + bounds.getHeight() * (1.0f - peakLevel);
        g.setColour(juce::Colour(0xffE5E7EB));
        g.drawHorizontalLine((int)peakY, bounds.getX(), bounds.getRight());
    }
}

void PluginEditorNexusDynamic::MasterPanel::LevelMeter::setLevel(float newLevel)
{
    level = newLevel * 0.2f + level * 0.8f; // Smooth
    
    if (newLevel > peakLevel)
    {
        peakLevel = newLevel;
    }
    else
    {
        peakLevel *= 0.99f; // Slow decay
    }
    
    repaint();
}