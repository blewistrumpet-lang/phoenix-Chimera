#include "PluginEditorNexus.h"

//==============================================================================
// Main Editor
//==============================================================================

ChimeraAudioProcessorEditorNexus::ChimeraAudioProcessorEditorNexus(ChimeraAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setLookAndFeel(&nexusLookAndFeel);
    addComponentListener(this);
    
    // Create header
    headerPanel = std::make_unique<HeaderPanel>();
    addAndMakeVisible(headerPanel.get());
    
    // Create AI control panel
    aiPanel = std::make_unique<AIControlPanel>();
    aiPanel->onPromptSubmit = [this](const juce::String& prompt) {
        sendAIPrompt(prompt);
    };
    addAndMakeVisible(aiPanel.get());
    
    // Create slot components
    for (int i = 0; i < 6; ++i)
    {
        auto slot = std::make_unique<NexusSlotComponent>(audioProcessor, i);
        addAndMakeVisible(slot.get());
        slotComponents.push_back(std::move(slot));
    }
    
    // Create master controls
    masterPanel = std::make_unique<MasterControlPanel>(audioProcessor.getValueTreeState());
    addAndMakeVisible(masterPanel.get());
    
    // Set initial size - larger and resizable
    setSize(1200, 800);
    setResizable(true, true);
    setResizeLimits(900, 600, 2400, 1600);
    
    // Start update timer
    startTimerHz(30);
    
    // Check AI server connection
    checkServerConnection();
}

ChimeraAudioProcessorEditorNexus::~ChimeraAudioProcessorEditorNexus()
{
    removeComponentListener(this);
    setLookAndFeel(nullptr);
    stopTimer();
}

void ChimeraAudioProcessorEditorNexus::paint(juce::Graphics& g)
{
    drawBackground(g);
    drawGridOverlay(g);
}

void ChimeraAudioProcessorEditorNexus::resized()
{
    updateLayout();
}

void ChimeraAudioProcessorEditorNexus::timerCallback()
{
    // Update activity meters
    for (int i = 0; i < slotComponents.size(); ++i)
    {
        float activity = audioProcessor.getSlotActivity(i);
        slotComponents[i]->setActivity(activity);
    }
    
    // Update master meters
    float inputLevel = audioProcessor.getCurrentInputLevel();
    float outputLevel = audioProcessor.getCurrentOutputLevel();
    masterPanel->updateMeters(inputLevel, outputLevel);
    
    // Update CPU usage
    currentCpuUsage = audioProcessor.getCpuUsage();
    headerPanel->setCpuUsage(currentCpuUsage);
}

void ChimeraAudioProcessorEditorNexus::componentMovedOrResized(Component& component, 
                                                               bool wasMoved, bool wasResized)
{
    if (wasResized && &component == this)
    {
        updateLayout();
    }
}

void ChimeraAudioProcessorEditorNexus::updateLayout()
{
    auto bounds = getLocalBounds();
    
    // Header section
    headerPanel->setBounds(bounds.removeFromTop(80));
    
    // AI control section
    aiPanel->setBounds(bounds.removeFromTop(100));
    
    // Master controls at bottom
    masterPanel->setBounds(bounds.removeFromBottom(120));
    
    // Calculate slot grid layout
    bounds = bounds.reduced(10);
    int cols = calculateOptimalSlotColumns();
    int rows = (6 + cols - 1) / cols;
    
    int slotWidth = bounds.getWidth() / cols;
    int slotHeight = bounds.getHeight() / rows;
    
    for (int i = 0; i < 6; ++i)
    {
        int col = i % cols;
        int row = i / cols;
        
        slotComponents[i]->setBounds(
            bounds.getX() + col * slotWidth,
            bounds.getY() + row * slotHeight,
            slotWidth - 10,
            slotHeight - 10
        );
    }
}

int ChimeraAudioProcessorEditorNexus::calculateOptimalSlotColumns() const
{
    int width = getWidth();
    
    if (width < 1000) return 2;
    if (width < 1400) return 3;
    return 3; // Max 3 columns for better visibility
}

void ChimeraAudioProcessorEditorNexus::drawBackground(juce::Graphics& g)
{
    // Deep space gradient background
    juce::ColourGradient bgGradient(
        juce::Colour(0xff0a0a0f),
        0, 0,
        juce::Colour(0xff15151f),
        getWidth(), getHeight(),
        false
    );
    
    bgGradient.addColour(0.5, juce::Colour(0xff0f0f18));
    g.setGradientFill(bgGradient);
    g.fillAll();
    
    // Add subtle noise texture
    juce::Random rng;
    for (int i = 0; i < 500; ++i)
    {
        float x = rng.nextFloat() * getWidth();
        float y = rng.nextFloat() * getHeight();
        float brightness = rng.nextFloat() * 0.3f;
        g.setColour(juce::Colour(0xffffffff).withAlpha(brightness));
        g.fillEllipse(x, y, 1, 1);
    }
}

void ChimeraAudioProcessorEditorNexus::drawGridOverlay(juce::Graphics& g)
{
    // Tactical grid overlay
    if (auto* lnf = dynamic_cast<NexusLookAndFeel*>(&getLookAndFeel()))
    {
        lnf->drawTacticalGrid(g, getLocalBounds().toFloat(), 50.0f, 0.02f);
    }
}

void ChimeraAudioProcessorEditorNexus::sendAIPrompt(const juce::String& prompt)
{
    // Implementation similar to refined version
    juce::Thread::launch([this, prompt]() {
        for (int port : {8001, 8000})
        {
            juce::URL url("http://localhost:" + juce::String(port) + "/generate");
            
            // Format headers as a single string
            juce::String headers = "Content-Type: application/json\r\n";
            
            // Create JSON body
            juce::var jsonVar;
            jsonVar = juce::JSON::fromString("{\"prompt\":\"" + prompt + "\"}");
            juce::String postData = juce::JSON::toString(jsonVar);
            
            // Create POST request with proper API
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
        
        juce::MessageManager::callAsync([this] {
            aiPanel->statusLabel.setText("Connection failed", juce::dontSendNotification);
        });
    });
}

void ChimeraAudioProcessorEditorNexus::handleAIResponse(const juce::String& response)
{
    // Implementation similar to refined version
    auto json = juce::JSON::parse(response);
    
    if (json.hasProperty("success") && json["success"])
    {
        aiPanel->statusLabel.setText("Preset loaded", juce::dontSendNotification);
        // Load preset parameters...
    }
    else
    {
        aiPanel->statusLabel.setText("Generation failed", juce::dontSendNotification);
    }
}

void ChimeraAudioProcessorEditorNexus::checkServerConnection()
{
    juce::Thread::launch([this]() {
        for (int port : {8001, 8000})
        {
            juce::URL url("http://localhost:" + juce::String(port) + "/health");
            
            if (auto stream = url.createInputStream(
                juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                    .withConnectionTimeoutMs(500)))
            {
                auto response = stream->readEntireStreamAsString();
                auto json = juce::JSON::parse(response);
                
                if (json.hasProperty("status") && json["status"].toString() == "healthy")
                {
                    juce::MessageManager::callAsync([this] {
                        isServerConnected = true;
                        headerPanel->setServerStatus(true);
                    });
                    return;
                }
            }
        }
        
        juce::MessageManager::callAsync([this] {
            isServerConnected = false;
            headerPanel->setServerStatus(false);
        });
    });
}

//==============================================================================
// Slot Component
//==============================================================================

ChimeraAudioProcessorEditorNexus::NexusSlotComponent::NexusSlotComponent(
    ChimeraAudioProcessor& p, int slot)
    : processor(p), slotIndex(slot)
{
    // Title
    slotTitle.setText("SLOT " + juce::String(slot + 1), juce::dontSendNotification);
    slotTitle.setFont(juce::Font(juce::FontOptions().withHeight(16.0f)).boldened());
    slotTitle.setColour(juce::Label::textColourId, juce::Colour(NexusLookAndFeel::Colors::accent));
    slotTitle.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(slotTitle);
    
    // Engine selector
    engineSelector.addItem("-- EMPTY --", 1);
    
    // Add all available engines with their actual names
    for (int i = 0; i < EngineLibrary::getEngineCount(); ++i)
    {
        juce::String engineName = EngineLibrary::getEngineName(i);
        if (engineName.isEmpty())
            engineName = "Engine " + juce::String(i + 1);
        engineSelector.addItem(engineName, i + 2);
    }
    
    engineSelector.onChange = [this] { updateParameters(); };
    addAndMakeVisible(engineSelector);
    
    // Control buttons
    bypassButton.setColour(juce::ToggleButton::textColourId, juce::Colour(0xffffaa00));
    addAndMakeVisible(bypassButton);
    
    soloButton.setColour(juce::ToggleButton::textColourId, juce::Colour(0xff00ff88));
    addAndMakeVisible(soloButton);
    
    muteButton.setColour(juce::ToggleButton::textColourId, juce::Colour(0xffff006e));
    addAndMakeVisible(muteButton);
    
    // Attach to value tree
    juce::String slotPrefix = "slot" + juce::String(slot + 1) + "_";
    
    engineAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processor.getValueTreeState(), slotPrefix + "engine", engineSelector);
    
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processor.getValueTreeState(), slotPrefix + "bypass", bypassButton);
    
    // Initialize parameters
    updateParameters();
}

ChimeraAudioProcessorEditorNexus::NexusSlotComponent::~NexusSlotComponent() = default;

void ChimeraAudioProcessorEditorNexus::NexusSlotComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Draw slot panel
    if (auto* lnf = dynamic_cast<NexusLookAndFeel*>(&getLookAndFeel()))
    {
        lnf->drawNexusPanel(g, bounds, activityLevel > 0.01f, activityLevel);
        
        // Draw holographic frame when active
        if (activityLevel > 0.01f)
        {
            lnf->drawHolographicFrame(g, bounds.reduced(2),
                juce::Colour(NexusLookAndFeel::Colors::accent).withAlpha(activityLevel));
        }
    }
    
    // Activity indicator bar
    if (activityLevel > 0.01f)
    {
        auto barBounds = bounds.reduced(5).removeFromBottom(3);
        g.setColour(juce::Colour(NexusLookAndFeel::Colors::accent).withAlpha(0.3f));
        g.fillRoundedRectangle(barBounds, 1.0f);
        
        barBounds.setWidth(barBounds.getWidth() * activityLevel);
        g.setColour(juce::Colour(NexusLookAndFeel::Colors::accent));
        g.fillRoundedRectangle(barBounds, 1.0f);
    }
}

void ChimeraAudioProcessorEditorNexus::NexusSlotComponent::resized()
{
    auto bounds = getLocalBounds().reduced(10);
    
    // Header section
    auto headerBounds = bounds.removeFromTop(25);
    slotTitle.setBounds(headerBounds.removeFromLeft(80));
    
    // Control buttons
    auto buttonWidth = 50;
    muteButton.setBounds(headerBounds.removeFromRight(buttonWidth));
    soloButton.setBounds(headerBounds.removeFromRight(buttonWidth));
    bypassButton.setBounds(headerBounds.removeFromRight(buttonWidth));
    
    // Engine selector
    engineSelector.setBounds(headerBounds.reduced(5, 0));
    
    bounds.removeFromTop(10);
    
    // Dynamic parameter layout
    if (!parameterControls.empty())
    {
        int numParams = static_cast<int>(parameterControls.size());
        
        // Calculate grid dimensions
        int cols = (numParams <= 4) ? 2 :
                  (numParams <= 9) ? 3 :
                  (numParams <= 16) ? 4 : 5;
        int rows = (numParams + cols - 1) / cols;
        
        int controlWidth = bounds.getWidth() / cols;
        int controlHeight = juce::jmin(80, bounds.getHeight() / rows);
        
        for (int i = 0; i < numParams; ++i)
        {
            int col = i % cols;
            int row = i / cols;
            
            auto controlBounds = juce::Rectangle<int>(
                bounds.getX() + col * controlWidth,
                bounds.getY() + row * controlHeight,
                controlWidth - 5,
                controlHeight - 5
            );
            
            auto& control = parameterControls[i];
            
            if (control.isToggle && control.toggle)
            {
                control.toggle->setBounds(controlBounds.reduced(10));
            }
            else if (control.slider && control.label)
            {
                control.label->setBounds(controlBounds.removeFromTop(15));
                control.slider->setBounds(controlBounds);
            }
        }
    }
}

void ChimeraAudioProcessorEditorNexus::NexusSlotComponent::updateParameters()
{
    // Clear existing controls
    for (auto& control : parameterControls)
    {
        if (control.slider) removeChildComponent(control.slider.get());
        if (control.toggle) removeChildComponent(control.toggle.get());
        if (control.label) removeChildComponent(control.label.get());
    }
    parameterControls.clear();
    
    // Get selected engine
    int engineId = engineSelector.getSelectedId() - 2;
    
    if (engineId < 0)
    {
        repaint();
        return;
    }
    
    createParametersForEngine(engineId);
    resized();
    repaint();
}

void ChimeraAudioProcessorEditorNexus::NexusSlotComponent::createParametersForEngine(int engineId)
{
    int paramCount = UnifiedDefaultParameters::getParameterCount(engineId);
    paramCount = juce::jmin(paramCount, 15);
    
    juce::String slotPrefix = "slot" + juce::String(slotIndex + 1) + "_";
    
    for (int i = 0; i < paramCount; ++i)
    {
        ParameterControl control;
        
        // Get actual parameter name
        juce::String paramName = getActualParameterName(engineId, i);
        control.isToggle = shouldBeToggle(paramName);
        
        juce::String paramId = slotPrefix + "param" + juce::String(i + 1);
        
        if (control.isToggle)
        {
            control.toggle = std::make_unique<juce::ToggleButton>(paramName);
            control.toggle->setColour(juce::ToggleButton::textColourId, 
                                     juce::Colour(NexusLookAndFeel::Colors::text));
            addAndMakeVisible(control.toggle.get());
            
            if (processor.getValueTreeState().getParameter(paramId))
            {
                control.buttonAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
                    processor.getValueTreeState(), paramId, *control.toggle);
            }
        }
        else
        {
            control.slider = std::make_unique<juce::Slider>();
            control.slider->setSliderStyle(juce::Slider::RotaryVerticalDrag);
            control.slider->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            control.slider->setPopupDisplayEnabled(true, true, this);
            addAndMakeVisible(control.slider.get());
            
            control.label = std::make_unique<juce::Label>();
            control.label->setText(paramName, juce::dontSendNotification);
            control.label->setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));
            control.label->setColour(juce::Label::textColourId, 
                                   juce::Colour(NexusLookAndFeel::Colors::textDim));
            control.label->setJustificationType(juce::Justification::centred);
            addAndMakeVisible(control.label.get());
            
            if (processor.getValueTreeState().getParameter(paramId))
            {
                control.sliderAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                    processor.getValueTreeState(), paramId, *control.slider);
            }
        }
        
        parameterControls.push_back(std::move(control));
    }
}

juce::String ChimeraAudioProcessorEditorNexus::NexusSlotComponent::getActualParameterName(
    int engineId, int paramIndex)
{
    // Try to get parameter name from EngineLibrary first
    juce::String paramName = EngineLibrary::getParameterName(engineId, paramIndex);
    
    if (paramName.isEmpty())
    {
        // Fallback to UnifiedDefaultParameters
        paramName = UnifiedDefaultParameters::getParameterName(engineId, paramIndex);
    }
    
    if (paramName.isEmpty())
    {
        // Last resort - generic name
        paramName = "Param " + juce::String(paramIndex + 1);
    }
    
    return paramName;
}

bool ChimeraAudioProcessorEditorNexus::NexusSlotComponent::shouldBeToggle(const juce::String& paramName)
{
    juce::String lowerName = paramName.toLowerCase();
    
    return lowerName.contains("enable") ||
           lowerName.contains("bypass") ||
           lowerName.contains("on") ||
           lowerName.contains("off") ||
           lowerName.contains("freeze") ||
           lowerName.contains("gate") ||
           lowerName.contains("sync") ||
           lowerName.contains("stereo") ||
           lowerName.contains("mono") ||
           lowerName.contains("active") ||
           lowerName.contains("mute") ||
           lowerName.contains("solo");
}

//==============================================================================
// Header Panel
//==============================================================================

ChimeraAudioProcessorEditorNexus::HeaderPanel::HeaderPanel()
{
    titleLabel.setFont(juce::Font(juce::FontOptions().withHeight(32.0f)).boldened());
    titleLabel.setColour(juce::Label::textColourId, juce::Colour(NexusLookAndFeel::Colors::accent));
    addAndMakeVisible(titleLabel);
    
    subtitleLabel.setFont(juce::Font(juce::FontOptions().withHeight(14.0f)));
    subtitleLabel.setColour(juce::Label::textColourId, juce::Colour(NexusLookAndFeel::Colors::textDim));
    addAndMakeVisible(subtitleLabel);
    
    versionLabel.setFont(juce::Font(juce::FontOptions().withHeight(12.0f)));
    versionLabel.setColour(juce::Label::textColourId, juce::Colour(NexusLookAndFeel::Colors::highlight));
    addAndMakeVisible(versionLabel);
    
    addAndMakeVisible(aiStatus);
    addAndMakeVisible(cpuStatus);
}

void ChimeraAudioProcessorEditorNexus::HeaderPanel::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Draw header background
    if (auto* lnf = dynamic_cast<NexusLookAndFeel*>(&getLookAndFeel()))
    {
        lnf->drawNexusPanel(g, bounds.reduced(5), false, 0.0f);
    }
}

void ChimeraAudioProcessorEditorNexus::HeaderPanel::resized()
{
    auto bounds = getLocalBounds().reduced(15);
    
    auto leftSection = bounds.removeFromLeft(bounds.getWidth() / 2);
    titleLabel.setBounds(leftSection.removeFromTop(35));
    subtitleLabel.setBounds(leftSection.removeFromTop(20));
    versionLabel.setBounds(leftSection);
    
    auto rightSection = bounds;
    auto statusBounds = rightSection.removeFromRight(200);
    aiStatus.setBounds(statusBounds.removeFromTop(statusBounds.getHeight() / 2));
    cpuStatus.setBounds(statusBounds);
}

void ChimeraAudioProcessorEditorNexus::HeaderPanel::setServerStatus(bool connected)
{
    aiStatus.setStatus(connected ? "AI ONLINE" : "AI OFFLINE",
                      connected ? juce::Colour(NexusLookAndFeel::Colors::success) :
                                 juce::Colour(NexusLookAndFeel::Colors::accentAlt));
}

void ChimeraAudioProcessorEditorNexus::HeaderPanel::setCpuUsage(float cpu)
{
    juce::String cpuText = "CPU: " + juce::String(cpu, 1) + "%";
    juce::Colour cpuColor = (cpu < 50) ? juce::Colour(NexusLookAndFeel::Colors::success) :
                           (cpu < 75) ? juce::Colour(NexusLookAndFeel::Colors::warning) :
                                       juce::Colour(NexusLookAndFeel::Colors::accentAlt);
    cpuStatus.setStatus(cpuText, cpuColor);
}

void ChimeraAudioProcessorEditorNexus::HeaderPanel::StatusIndicator::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // LED indicator
    auto ledBounds = bounds.removeFromLeft(20).reduced(5);
    if (auto* lnf = dynamic_cast<NexusLookAndFeel*>(&getLookAndFeel()))
    {
        lnf->drawStatusLED(g, ledBounds, statusColor, true, false);
    }
    
    // Status text
    g.setColour(statusColor);
    g.setFont(juce::Font(juce::FontOptions().withHeight(12.0f)).boldened());
    g.drawText(statusText, bounds, juce::Justification::centredLeft);
}

void ChimeraAudioProcessorEditorNexus::HeaderPanel::StatusIndicator::setStatus(
    const juce::String& text, juce::Colour color)
{
    statusText = text;
    statusColor = color;
    repaint();
}

//==============================================================================
// AI Control Panel
//==============================================================================

ChimeraAudioProcessorEditorNexus::AIControlPanel::AIControlPanel()
{
    promptInput.setMultiLine(false);
    promptInput.setReturnKeyStartsNewLine(false);
    promptInput.setTextToShowWhenEmpty("Enter sound design prompt...", 
                                       juce::Colour(NexusLookAndFeel::Colors::textDim));
    promptInput.setFont(juce::Font(juce::FontOptions().withHeight(14.0f)));
    addAndMakeVisible(promptInput);
    
    generateButton.setColour(juce::TextButton::buttonColourId, 
                            juce::Colour(NexusLookAndFeel::Colors::accent));
    generateButton.onClick = [this] {
        if (onPromptSubmit)
            onPromptSubmit(promptInput.getText());
    };
    addAndMakeVisible(generateButton);
    
    enhanceButton.setColour(juce::TextButton::buttonColourId, 
                           juce::Colour(NexusLookAndFeel::Colors::highlight));
    addAndMakeVisible(enhanceButton);
    
    randomizeButton.setColour(juce::TextButton::buttonColourId, 
                             juce::Colour(NexusLookAndFeel::Colors::warning));
    addAndMakeVisible(randomizeButton);
    
    statusLabel.setFont(juce::Font(juce::FontOptions().withHeight(12.0f)));
    statusLabel.setColour(juce::Label::textColourId, 
                         juce::Colour(NexusLookAndFeel::Colors::textDim));
    addAndMakeVisible(statusLabel);
}

void ChimeraAudioProcessorEditorNexus::AIControlPanel::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    if (auto* lnf = dynamic_cast<NexusLookAndFeel*>(&getLookAndFeel()))
    {
        lnf->drawNexusPanel(g, bounds.reduced(5), false, 0.2f);
    }
}

void ChimeraAudioProcessorEditorNexus::AIControlPanel::resized()
{
    auto bounds = getLocalBounds().reduced(15);
    
    auto topRow = bounds.removeFromTop(35);
    promptInput.setBounds(topRow.removeFromLeft(topRow.getWidth() - 350));
    topRow.removeFromLeft(10);
    
    auto buttonWidth = 110;
    generateButton.setBounds(topRow.removeFromLeft(buttonWidth));
    topRow.removeFromLeft(5);
    enhanceButton.setBounds(topRow.removeFromLeft(buttonWidth));
    topRow.removeFromLeft(5);
    randomizeButton.setBounds(topRow);
    
    bounds.removeFromTop(10);
    statusLabel.setBounds(bounds);
}

//==============================================================================
// Master Control Panel
//==============================================================================

ChimeraAudioProcessorEditorNexus::MasterControlPanel::MasterControlPanel(
    juce::AudioProcessorValueTreeState& apvts)
{
    // Setup controls
    inputGain.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    inputGain.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible(inputGain);
    addAndMakeVisible(inputLabel);
    
    outputGain.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    outputGain.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible(outputGain);
    addAndMakeVisible(outputLabel);
    
    mixControl.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    mixControl.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible(mixControl);
    addAndMakeVisible(mixLabel);
    
    addAndMakeVisible(inputMeter);
    addAndMakeVisible(outputMeter);
    
    // Create attachments
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

void ChimeraAudioProcessorEditorNexus::MasterControlPanel::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    if (auto* lnf = dynamic_cast<NexusLookAndFeel*>(&getLookAndFeel()))
    {
        lnf->drawNexusPanel(g, bounds.reduced(5), false, 0.1f);
    }
}

void ChimeraAudioProcessorEditorNexus::MasterControlPanel::resized()
{
    auto bounds = getLocalBounds().reduced(15);
    
    auto meterWidth = 60;
    auto controlWidth = 80;
    
    inputMeter.setBounds(bounds.removeFromLeft(meterWidth));
    bounds.removeFromLeft(20);
    
    auto controlBounds = bounds.removeFromLeft(controlWidth);
    inputLabel.setBounds(controlBounds.removeFromTop(20));
    inputGain.setBounds(controlBounds);
    
    bounds.removeFromLeft(20);
    
    controlBounds = bounds.removeFromLeft(controlWidth);
    mixLabel.setBounds(controlBounds.removeFromTop(20));
    mixControl.setBounds(controlBounds);
    
    bounds.removeFromLeft(20);
    
    controlBounds = bounds.removeFromLeft(controlWidth);
    outputLabel.setBounds(controlBounds.removeFromTop(20));
    outputGain.setBounds(controlBounds);
    
    bounds.removeFromLeft(20);
    outputMeter.setBounds(bounds.removeFromLeft(meterWidth));
}

void ChimeraAudioProcessorEditorNexus::MasterControlPanel::updateMeters(float inputLevel, 
                                                                       float outputLevel)
{
    inputMeter.setLevel(inputLevel);
    outputMeter.setLevel(outputLevel);
}

void ChimeraAudioProcessorEditorNexus::MasterControlPanel::VUMeter::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Background
    g.setColour(juce::Colour(0xff0a0a0f));
    g.fillRoundedRectangle(bounds, 4.0f);
    
    // Meter fill
    smoothedLevel = smoothedLevel * 0.9f + level * 0.1f;
    
    auto fillHeight = bounds.getHeight() * smoothedLevel;
    auto fillBounds = bounds.removeFromBottom(fillHeight);
    
    // Gradient based on level
    juce::Colour topColor = (smoothedLevel > 0.9f) ? juce::Colour(0xffff006e) :
                           (smoothedLevel > 0.7f) ? juce::Colour(0xffffaa00) :
                                                    juce::Colour(0xff00ff88);
    
    juce::ColourGradient meterGrad(
        topColor, fillBounds.getTopLeft(),
        topColor.darker(0.5f), fillBounds.getBottomLeft(),
        false
    );
    g.setGradientFill(meterGrad);
    g.fillRoundedRectangle(fillBounds, 4.0f);
    
    // Peak indicator
    if (smoothedLevel > 0.95f)
    {
        g.setColour(juce::Colour(0xffff006e));
        g.fillEllipse(bounds.getCentreX() - 3, bounds.getY() + 2, 6, 6);
    }
}

void ChimeraAudioProcessorEditorNexus::MasterControlPanel::VUMeter::setLevel(float newLevel)
{
    level = juce::jlimit(0.0f, 1.0f, newLevel);
    repaint();
}