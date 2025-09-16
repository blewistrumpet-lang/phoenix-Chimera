#include "PluginEditorNexus_Final.h"
#include "EngineTypes.h"

//==============================================================================
// MAIN EDITOR IMPLEMENTATION
//==============================================================================

PluginEditorNexus_Final::PluginEditorNexus_Final(ChimeraAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    // Apply the Tactile Futurism aesthetic
    setLookAndFeel(&nexusLookAndFeel);
    
    // Create AI Command Center (left column)
    aiCenter = std::make_unique<AICommandCenter>();
    aiCenter->onPromptExecute = [this](const juce::String& prompt) {
        executeAIPrompt(prompt);
    };
    addAndMakeVisible(aiCenter.get());
    
    // Create 6 Engine Slots (right column)
    for (int i = 0; i < 6; ++i)
    {
        auto slot = std::make_unique<EngineSlot>(audioProcessor, i);
        addAndMakeVisible(slot.get());
        engineSlots.push_back(std::move(slot));
    }
    
    // Create Master Section (bottom bar)
    masterSection = std::make_unique<MasterSection>(audioProcessor.getValueTreeState());
    addAndMakeVisible(masterSection.get());
    
    // Set size as per mandate: 1200x800, resizable
    setSize(1200, 800);
    setResizable(true, true);
    setResizeLimits(1000, 700, 1600, 1200);
    
    // Start animation timer
    startTimerHz(30);
}

PluginEditorNexus_Final::~PluginEditorNexus_Final()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void PluginEditorNexus_Final::paint(juce::Graphics& g)
{
    drawBackground(g);
    drawTitleBar(g);
}

void PluginEditorNexus_Final::resized()
{
    auto bounds = getLocalBounds();
    
    // Title bar
    auto titleHeight = 50;
    bounds.removeFromTop(titleHeight);
    
    // Master section at bottom
    auto masterHeight = 100;
    masterSection->setBounds(bounds.removeFromBottom(masterHeight));
    
    // Main content area
    bounds = bounds.reduced(10);
    
    // Two-column layout
    auto leftColumnWidth = 380;
    auto leftColumn = bounds.removeFromLeft(leftColumnWidth);
    bounds.removeFromLeft(10); // Spacing
    
    // AI Command Center (left)
    aiCenter->setBounds(leftColumn);
    
    // 6-Slot Rack (right) - 2x3 grid
    auto slotWidth = bounds.getWidth() / 2;
    auto slotHeight = bounds.getHeight() / 3;
    
    for (int i = 0; i < 6; ++i)
    {
        int col = i % 2;
        int row = i / 2;
        
        engineSlots[i]->setBounds(
            bounds.getX() + col * slotWidth + (col * 5),
            bounds.getY() + row * slotHeight + (row * 5),
            slotWidth - 5,
            slotHeight - 5
        );
    }
}

void PluginEditorNexus_Final::timerCallback()
{
    // Update animation phase
    animationPhase += 0.02f;
    if (animationPhase > 1.0f)
        animationPhase = 0.0f;
    
    nexusLookAndFeel.updateAnimations();
    
    // Update slot activity levels
    for (int i = 0; i < engineSlots.size(); ++i)
    {
        float activity = audioProcessor.getSlotActivity(i);
        engineSlots[i]->setActivity(activity);
    }
    
    // Update master meters
    float inputLevel = audioProcessor.getCurrentInputLevel();
    float outputLevel = audioProcessor.getCurrentOutputLevel();
    masterSection->updateMeters(inputLevel, outputLevel);
}

void PluginEditorNexus_Final::drawBackground(juce::Graphics& g)
{
    // Deep space black base
    g.fillAll(juce::Colour(NexusLookAndFeel_Final::Colors::baseBlack));
    
    // Subtle gradient overlay
    juce::ColourGradient bgGrad(
        juce::Colour(NexusLookAndFeel_Final::Colors::baseDark).withAlpha(0.7f),
        0, 0,
        juce::Colour(NexusLookAndFeel_Final::Colors::baseBlack),
        getWidth(), getHeight(),
        false
    );
    g.setGradientFill(bgGrad);
    g.fillAll();
    
    // Scanline effect
    nexusLookAndFeel.drawScanlineEffect(g, getLocalBounds().toFloat(), animationPhase);
}

void PluginEditorNexus_Final::drawTitleBar(juce::Graphics& g)
{
    auto titleBounds = getLocalBounds().removeFromTop(50).toFloat();
    
    // Title background panel
    nexusLookAndFeel.drawHolographicPanel(g, titleBounds.reduced(5, 5), true);
    
    // Title text
    g.setColour(juce::Colour(NexusLookAndFeel_Final::Colors::primaryCyan));
    g.setFont(nexusLookAndFeel.getTacticalFont(24.0f, true));
    g.drawText("CHIMERA PHOENIX NEXUS", titleBounds.reduced(20, 0), 
               juce::Justification::centredLeft);
    
    // Version text
    g.setColour(juce::Colour(NexusLookAndFeel_Final::Colors::textSecondary));
    g.setFont(nexusLookAndFeel.getTacticalFont(14.0f));
    g.drawText("v3.0.2030 | TACTILE FUTURISM", titleBounds.reduced(20, 0), 
               juce::Justification::centredRight);
}

void PluginEditorNexus_Final::executeAIPrompt(const juce::String& prompt)
{
    // Send to Trinity Pipeline
    juce::Thread::launch([this, prompt]() {
        // Try both ports
        for (int port : {8001, 8000})
        {
            juce::URL url("http://localhost:" + juce::String(port) + "/generate");
            
            juce::String headers = "Content-Type: application/json\r\n";
            juce::String postData = "{\"prompt\":\"" + prompt + "\"}";
            
            url = url.withPOSTData(postData);
            
            if (auto stream = url.createInputStream(
                juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                    .withConnectionTimeoutMs(5000)
                    .withExtraHeaders(headers)))
            {
                auto response = stream->readEntireStreamAsString();
                handleAIResponse(response);
                return;
            }
        }
    });
}

void PluginEditorNexus_Final::handleAIResponse(const juce::String& response)
{
    // Parse and apply AI-generated parameters
    auto json = juce::JSON::parse(response);
    
    if (json.hasProperty("success") && json["success"])
    {
        // Update UI with response
        juce::MessageManager::callAsync([this] {
            aiCenter->statusLabel.setText("PRESET LOADED", juce::dontSendNotification);
        });
    }
}

//==============================================================================
// AI COMMAND CENTER IMPLEMENTATION
//==============================================================================

PluginEditorNexus_Final::AICommandCenter::AICommandCenter()
{
    // Title
    titleLabel.setFont(juce::Font(juce::FontOptions().withHeight(18.0f)));
    titleLabel.setColour(juce::Label::textColourId, 
                        juce::Colour(NexusLookAndFeel_Final::Colors::primaryCyan));
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);
    
    // Status
    statusLabel.setFont(juce::Font(juce::FontOptions().withHeight(12.0f)));
    statusLabel.setColour(juce::Label::textColourId, 
                         juce::Colour(NexusLookAndFeel_Final::Colors::textSecondary));
    statusLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(statusLabel);
    
    // Prompt input
    promptInput.setMultiLine(false);
    promptInput.setReturnKeyStartsNewLine(false);
    promptInput.setTextToShowWhenEmpty("Enter sound design prompt...", 
                                       juce::Colour(NexusLookAndFeel_Final::Colors::textSecondary));
    promptInput.setFont(juce::Font(juce::FontOptions().withHeight(14.0f)));
    addAndMakeVisible(promptInput);
    
    // Buttons
    executeButton.setColour(juce::TextButton::buttonColourId, 
                           juce::Colour(NexusLookAndFeel_Final::Colors::primaryCyan));
    executeButton.onClick = [this] {
        if (onPromptExecute)
            onPromptExecute(promptInput.getText());
    };
    addAndMakeVisible(executeButton);
    
    enhanceButton.setColour(juce::TextButton::buttonColourId, 
                           juce::Colour(NexusLookAndFeel_Final::Colors::baseDark));
    addAndMakeVisible(enhanceButton);
    
    randomizeButton.setColour(juce::TextButton::buttonColourId, 
                             juce::Colour(NexusLookAndFeel_Final::Colors::secondaryMagenta));
    addAndMakeVisible(randomizeButton);
    
    // Pipeline status
    addAndMakeVisible(pipelineStatus);
}

void PluginEditorNexus_Final::AICommandCenter::paint(juce::Graphics& g)
{
    auto lnf = dynamic_cast<NexusLookAndFeel_Final*>(&getLookAndFeel());
    if (lnf)
    {
        lnf->drawHolographicPanel(g, getLocalBounds().toFloat(), false);
    }
}

void PluginEditorNexus_Final::AICommandCenter::resized()
{
    auto bounds = getLocalBounds().reduced(15);
    
    titleLabel.setBounds(bounds.removeFromTop(30));
    bounds.removeFromTop(5);
    
    statusLabel.setBounds(bounds.removeFromTop(20));
    bounds.removeFromTop(10);
    
    promptInput.setBounds(bounds.removeFromTop(40));
    bounds.removeFromTop(10);
    
    auto buttonRow = bounds.removeFromTop(35);
    int buttonWidth = buttonRow.getWidth() / 3 - 5;
    
    executeButton.setBounds(buttonRow.removeFromLeft(buttonWidth));
    buttonRow.removeFromLeft(5);
    enhanceButton.setBounds(buttonRow.removeFromLeft(buttonWidth));
    buttonRow.removeFromLeft(5);
    randomizeButton.setBounds(buttonRow);
    
    bounds.removeFromTop(20);
    pipelineStatus.setBounds(bounds.removeFromTop(100));
}

void PluginEditorNexus_Final::AICommandCenter::PipelineStatus::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    int stageHeight = bounds.getHeight() / 4;
    
    for (int i = 0; i < 4; ++i)
    {
        auto stageBounds = bounds.removeFromTop(stageHeight).toFloat();
        
        // Stage indicator
        g.setColour(stages[i] ? 
                   juce::Colour(NexusLookAndFeel_Final::Colors::primaryCyan) :
                   juce::Colour(NexusLookAndFeel_Final::Colors::baseDark));
        g.fillRoundedRectangle(stageBounds.reduced(5, 2), 4.0f);
        
        // Stage name
        g.setColour(juce::Colour(NexusLookAndFeel_Final::Colors::textPrimary));
        g.setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));
        g.drawText(stageNames[i], stageBounds, juce::Justification::centred);
    }
}

void PluginEditorNexus_Final::AICommandCenter::PipelineStatus::setStage(int stage, bool active)
{
    if (stage >= 0 && stage < 4)
    {
        stages[stage] = active;
        repaint();
    }
}

//==============================================================================
// ENGINE SLOT IMPLEMENTATION - DYNAMIC PARAMETER SYSTEM
//==============================================================================

PluginEditorNexus_Final::EngineSlot::EngineSlot(ChimeraAudioProcessor& proc, int slotIndex)
    : processor(proc), slot(slotIndex)
{
    // Slot label
    slotLabel.setText("SLOT " + juce::String(slot + 1), juce::dontSendNotification);
    slotLabel.setFont(juce::Font(juce::FontOptions().withHeight(16.0f)));
    slotLabel.setColour(juce::Label::textColourId, 
                        juce::Colour(NexusLookAndFeel_Final::Colors::primaryCyan));
    slotLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(slotLabel);
    
    // Engine selector - populate from database
    engineSelector.addItem("-- EMPTY --", 1);
    
    // Add all engines from the database
    for (const auto& engine : ChimeraParameters::engineDatabase)
    {
        engineSelector.addItem(engine.displayName, engine.legacyId + 2);
    }
    
    engineSelector.onChange = [this] {
        updateParametersFromDatabase();
    };
    addAndMakeVisible(engineSelector);
    
    // Bypass button
    bypassButton.setColour(juce::ToggleButton::textColourId, 
                          juce::Colour(NexusLookAndFeel_Final::Colors::secondaryMagenta));
    addAndMakeVisible(bypassButton);
    
    // Attach to value tree
    juce::String slotPrefix = "slot" + juce::String(slot + 1) + "_";
    
    engineAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processor.getValueTreeState(), slotPrefix + "engine", engineSelector);
    
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processor.getValueTreeState(), slotPrefix + "bypass", bypassButton);
    
    // Initialize parameters
    updateParametersFromDatabase();
}

PluginEditorNexus_Final::EngineSlot::~EngineSlot() = default;

void PluginEditorNexus_Final::EngineSlot::paint(juce::Graphics& g)
{
    auto lnf = dynamic_cast<NexusLookAndFeel_Final*>(&getLookAndFeel());
    if (lnf)
    {
        // Draw panel with activity glow
        lnf->drawHolographicPanel(g, getLocalBounds().toFloat(), activityLevel > 0.01f);
        
        // Activity indicator
        if (activityLevel > 0.01f)
        {
            auto bounds = getLocalBounds().toFloat();
            lnf->drawNeonGlow(g, bounds, 
                            juce::Colour(NexusLookAndFeel_Final::Colors::primaryCyan), 
                            activityLevel * 0.5f);
        }
    }
}

void PluginEditorNexus_Final::EngineSlot::resized()
{
    auto bounds = getLocalBounds().reduced(10);
    
    // Header
    auto headerBounds = bounds.removeFromTop(25);
    slotLabel.setBounds(headerBounds.removeFromLeft(80));
    bypassButton.setBounds(headerBounds.removeFromRight(70));
    headerBounds.removeFromRight(5);
    engineSelector.setBounds(headerBounds);
    
    bounds.removeFromTop(10);
    
    // Dynamic parameter layout
    if (!parameters.empty())
    {
        int numParams = static_cast<int>(parameters.size());
        
        // Calculate optimal grid
        int cols = (numParams <= 3) ? numParams :
                  (numParams <= 6) ? 3 :
                  (numParams <= 9) ? 3 :
                  (numParams <= 12) ? 4 : 5;
        
        int rows = (numParams + cols - 1) / cols;
        
        int controlWidth = bounds.getWidth() / cols;
        int controlHeight = bounds.getHeight() / rows;
        
        for (int i = 0; i < numParams; ++i)
        {
            int col = i % cols;
            int row = i / cols;
            
            auto paramBounds = juce::Rectangle<int>(
                bounds.getX() + col * controlWidth,
                bounds.getY() + row * controlHeight,
                controlWidth - 5,
                controlHeight - 5
            );
            
            auto& param = parameters[i];
            
            if (param.isToggle)
            {
                // Toggle button takes full bounds
                if (param.control)
                    param.control->setBounds(paramBounds.reduced(10));
            }
            else
            {
                // Label above slider
                if (param.label)
                {
                    param.label->setBounds(paramBounds.removeFromTop(15));
                }
                if (param.control)
                {
                    param.control->setBounds(paramBounds);
                }
            }
        }
    }
}

void PluginEditorNexus_Final::EngineSlot::updateParametersFromDatabase()
{
    // Clear existing parameters
    for (auto& param : parameters)
    {
        if (param.control) removeChildComponent(param.control.get());
        if (param.label) removeChildComponent(param.label.get());
    }
    parameters.clear();
    
    // Get selected engine ID
    int engineId = engineSelector.getSelectedId() - 2;
    
    if (engineId < 0) // Empty slot
    {
        repaint();
        return;
    }
    
    // Create parameters from database
    createParametersForEngine(engineId);
    resized();
    repaint();
}

void PluginEditorNexus_Final::EngineSlot::createParametersForEngine(int engineId)
{
    // Find engine in database
    const ChimeraParameters::EngineInfo* engineInfo = nullptr;
    
    for (const auto& engine : ChimeraParameters::engineDatabase)
    {
        if (engine.legacyId == engineId)
        {
            engineInfo = &engine;
            break;
        }
    }
    
    if (!engineInfo)
        return;
    
    // Create controls for each parameter
    juce::String slotPrefix = "slot" + juce::String(slot + 1) + "_";
    
    for (int i = 0; i < engineInfo->parameterCount && i < 15; ++i)
    {
        DynamicParameter param;
        
        // Get parameter info from database
        const auto& paramInfo = engineInfo->parameters[i];
        param.name = paramInfo.name;
        
        // Determine if this is a toggle
        param.isToggle = isParameterToggle(param.name);
        
        juce::String paramId = slotPrefix + "param" + juce::String(i + 1);
        
        if (param.isToggle)
        {
            // Create toggle button
            auto toggle = std::make_unique<juce::ToggleButton>(param.name);
            toggle->setColour(juce::ToggleButton::textColourId, 
                            juce::Colour(NexusLookAndFeel_Final::Colors::textPrimary));
            
            param.control = std::move(toggle);
            addAndMakeVisible(param.control.get());
            
            // Attach to parameter
            if (processor.getValueTreeState().getParameter(paramId))
            {
                param.buttonAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
                    processor.getValueTreeState(), paramId, 
                    *static_cast<juce::ToggleButton*>(param.control.get()));
            }
        }
        else
        {
            // Create slider
            auto slider = std::make_unique<juce::Slider>();
            slider->setSliderStyle(juce::Slider::RotaryVerticalDrag);
            slider->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            slider->setPopupDisplayEnabled(true, true, this);
            
            param.control = std::move(slider);
            addAndMakeVisible(param.control.get());
            
            // Create label
            param.label = std::make_unique<juce::Label>();
            param.label->setText(param.name, juce::dontSendNotification);
            param.label->setFont(juce::Font(juce::FontOptions().withHeight(10.0f)));
            param.label->setColour(juce::Label::textColourId, 
                                 juce::Colour(NexusLookAndFeel_Final::Colors::textSecondary));
            param.label->setJustificationType(juce::Justification::centred);
            addAndMakeVisible(param.label.get());
            
            // Attach to parameter
            if (processor.getValueTreeState().getParameter(paramId))
            {
                param.sliderAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                    processor.getValueTreeState(), paramId,
                    *static_cast<juce::Slider*>(param.control.get()));
            }
        }
        
        parameters.push_back(std::move(param));
    }
}

bool PluginEditorNexus_Final::EngineSlot::isParameterToggle(const juce::String& name)
{
    juce::String lowerName = name.toLowerCase();
    
    return lowerName.contains("enable") ||
           lowerName.contains("bypass") ||
           lowerName.contains("on") ||
           lowerName.contains("off") ||
           lowerName.contains("freeze") ||
           lowerName.contains("gate") ||
           lowerName.contains("sync") ||
           lowerName.contains("mono") ||
           lowerName.contains("stereo");
}

//==============================================================================
// MASTER SECTION IMPLEMENTATION
//==============================================================================

PluginEditorNexus_Final::MasterSection::MasterSection(juce::AudioProcessorValueTreeState& apvts)
{
    // Input controls
    inputGain.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    inputGain.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 18);
    addAndMakeVisible(inputGain);
    
    inputLabel.setJustificationType(juce::Justification::centred);
    inputLabel.setColour(juce::Label::textColourId, 
                        juce::Colour(NexusLookAndFeel_Final::Colors::textPrimary));
    addAndMakeVisible(inputLabel);
    
    // Output controls
    outputGain.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    outputGain.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 18);
    addAndMakeVisible(outputGain);
    
    outputLabel.setJustificationType(juce::Justification::centred);
    outputLabel.setColour(juce::Label::textColourId, 
                         juce::Colour(NexusLookAndFeel_Final::Colors::textPrimary));
    addAndMakeVisible(outputLabel);
    
    // Mix control
    mixControl.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    mixControl.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 18);
    addAndMakeVisible(mixControl);
    
    mixLabel.setJustificationType(juce::Justification::centred);
    mixLabel.setColour(juce::Label::textColourId, 
                       juce::Colour(NexusLookAndFeel_Final::Colors::textPrimary));
    addAndMakeVisible(mixLabel);
    
    // Meters
    addAndMakeVisible(inputMeter);
    addAndMakeVisible(outputMeter);
    
    // Attachments
    if (apvts.getParameter("input_gain"))
    {
        attachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, "input_gain", inputGain));
    }
    
    if (apvts.getParameter("output_gain"))
    {
        attachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, "output_gain", outputGain));
    }
    
    if (apvts.getParameter("mix"))
    {
        attachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, "mix", mixControl));
    }
}

void PluginEditorNexus_Final::MasterSection::paint(juce::Graphics& g)
{
    auto lnf = dynamic_cast<NexusLookAndFeel_Final*>(&getLookAndFeel());
    if (lnf)
    {
        lnf->drawHolographicPanel(g, getLocalBounds().toFloat(), false);
    }
}

void PluginEditorNexus_Final::MasterSection::resized()
{
    auto bounds = getLocalBounds().reduced(10);
    
    int sectionWidth = bounds.getWidth() / 5;
    
    // Input meter
    auto meterBounds = bounds.removeFromLeft(sectionWidth - 20);
    inputMeter.setBounds(meterBounds);
    
    bounds.removeFromLeft(10);
    
    // Input gain
    auto controlBounds = bounds.removeFromLeft(sectionWidth);
    inputLabel.setBounds(controlBounds.removeFromTop(20));
    inputGain.setBounds(controlBounds);
    
    bounds.removeFromLeft(10);
    
    // Mix
    controlBounds = bounds.removeFromLeft(sectionWidth);
    mixLabel.setBounds(controlBounds.removeFromTop(20));
    mixControl.setBounds(controlBounds);
    
    bounds.removeFromLeft(10);
    
    // Output gain
    controlBounds = bounds.removeFromLeft(sectionWidth);
    outputLabel.setBounds(controlBounds.removeFromTop(20));
    outputGain.setBounds(controlBounds);
    
    bounds.removeFromLeft(10);
    
    // Output meter
    meterBounds = bounds.removeFromLeft(sectionWidth - 20);
    outputMeter.setBounds(meterBounds);
}

void PluginEditorNexus_Final::MasterSection::updateMeters(float input, float output)
{
    inputMeter.setLevel(input);
    outputMeter.setLevel(output);
}

void PluginEditorNexus_Final::MasterSection::VUMeter::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Background
    g.setColour(juce::Colour(NexusLookAndFeel_Final::Colors::baseBlack));
    g.fillRoundedRectangle(bounds, 4.0f);
    
    // Meter fill
    float fillHeight = bounds.getHeight() * level;
    auto fillBounds = bounds.removeFromBottom(fillHeight);
    
    // Color based on level
    juce::Colour meterColor;
    if (level > 0.9f)
        meterColor = juce::Colour(NexusLookAndFeel_Final::Colors::secondaryMagenta);
    else if (level > 0.7f)
        meterColor = juce::Colour(0xffffaa00); // Warning yellow
    else
        meterColor = juce::Colour(NexusLookAndFeel_Final::Colors::primaryCyan);
    
    g.setColour(meterColor);
    g.fillRoundedRectangle(fillBounds, 4.0f);
    
    // Peak indicator
    if (peakLevel > 0.0f)
    {
        float peakY = bounds.getBottom() - (bounds.getHeight() * peakLevel);
        g.setColour(juce::Colour(NexusLookAndFeel_Final::Colors::textPrimary));
        g.drawHorizontalLine(int(peakY), bounds.getX(), bounds.getRight());
    }
}

void PluginEditorNexus_Final::MasterSection::VUMeter::setLevel(float newLevel)
{
    level = juce::jlimit(0.0f, 1.0f, newLevel);
    
    if (level > peakLevel)
        peakLevel = level;
    else
        peakLevel *= 0.99f; // Slow decay
    
    repaint();
}