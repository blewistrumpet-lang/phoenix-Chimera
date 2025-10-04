#include "SlotComponentFixed.h"

SlotComponentFixed::SlotComponentFixed(int slotIndex)
    : slotNumber(slotIndex), visibleParamCount(0), currentEngineId(-1), componentsInitialized(false)
{
    DBG("SlotComponentFixed constructor for slot " + juce::String(slotIndex));
    
    // Initialize control types to default
    for (int i = 0; i < 15; ++i) {
        controlTypes[i] = CONTROL_ROTARY;
    }
    
    // DO NOT create or configure GUI components here!
    // Wait for initializeComponents() to be called
    DBG("SlotComponentFixed constructor completed - deferred GUI init");
}

SlotComponentFixed::~SlotComponentFixed()
{
    // Components are automatically deleted by JUCE
}

void SlotComponentFixed::initializeComponents()
{
    if (componentsInitialized) return;
    
    DBG("SlotComponentFixed::initializeComponents() for slot " + juce::String(slotNumber));
    
    // NOW it's safe to configure GUI components
    
    // Create slot label
    slotLabel.setText("SLOT " + juce::String(slotNumber + 1), juce::dontSendNotification);
    slotLabel.setJustificationType(juce::Justification::centred);
    slotLabel.setColour(juce::Label::textColourId, juce::Colour(0xff00ffcc));
    slotLabel.setFont(juce::Font(14.0f));
    addAndMakeVisible(slotLabel);
    
    // Create engine selector
    engineSelector.setTextWhenNothingSelected("Select Engine");
    engineSelector.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff1F2937));
    engineSelector.setColour(juce::ComboBox::textColourId, juce::Colour(0xffE5E7EB));
    addAndMakeVisible(engineSelector);
    
    // Create bypass button
    bypassButton.setButtonText("BYPASS");
    bypassButton.setColour(juce::ToggleButton::textColourId, juce::Colour(0xffff006e));
    bypassButton.setColour(juce::ToggleButton::tickColourId, juce::Colour(0xffff006e));
    addAndMakeVisible(bypassButton);
    
    // Create ALL 15 parameter controls
    for (int i = 0; i < 15; ++i)
    {
        // Configure rotary/slider control
        sliders[i].setSliderStyle(juce::Slider::RotaryVerticalDrag);
        sliders[i].setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        sliders[i].setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff00ffcc));
        sliders[i].setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff374151));
        sliders[i].setColour(juce::Slider::thumbColourId, juce::Colour(0xff00ffcc));
        sliders[i].setVisible(false);
        addAndMakeVisible(sliders[i]);
        
        // Configure toggle button
        toggles[i].setColour(juce::ToggleButton::textColourId, juce::Colour(0xffE5E7EB));
        toggles[i].setColour(juce::ToggleButton::tickColourId, juce::Colour(0xff00ffcc));
        toggles[i].setColour(juce::ToggleButton::tickDisabledColourId, juce::Colour(0xff374151));
        toggles[i].setVisible(false);
        addAndMakeVisible(toggles[i]);
        
        // Configure label
        labels[i].setText("Param " + juce::String(i + 1), juce::dontSendNotification);
        labels[i].setJustificationType(juce::Justification::centred);
        labels[i].setColour(juce::Label::textColourId, juce::Colour(0xff9CA3AF));
        labels[i].setFont(juce::Font(10.0f));
        labels[i].setVisible(false);
        addAndMakeVisible(labels[i]);
        
        // Default to rotary control
        controlTypes[i] = CONTROL_ROTARY;
    }
    
    componentsInitialized = true;
    DBG("SlotComponentFixed::initializeComponents() completed");
}

void SlotComponentFixed::update(EngineBase* currentEngine, int engineId)
{
    // Ensure components are initialized
    if (!componentsInitialized) {
        initializeComponents();
    }
    
    currentEngineId = engineId;
    
    if (currentEngine == nullptr)
    {
        // Hide all parameters if no engine
        for (int i = 0; i < 15; ++i)
        {
            sliders[i].setVisible(false);
            toggles[i].setVisible(false);
            labels[i].setVisible(false);
        }
        visibleParamCount = 0;
        return;
    }
    
    // Get parameter count from live engine
    int numParams = currentEngine->getNumParameters();
    numParams = juce::jmin(numParams, 15);
    
    // Update each parameter control
    for (int i = 0; i < 15; ++i)
    {
        if (i < numParams)
        {
            // Get parameter name from engine
            juce::String paramName = currentEngine->getParameterName(i);
            labels[i].setText(paramName, juce::dontSendNotification);
            labels[i].setVisible(true);
            
            // Determine control type and configure
            ControlType controlType = getControlTypeForParameter(engineId, i);
            controlTypes[i] = controlType;
            configureControlForType(i, controlType);
        }
        else
        {
            // Hide all controls for this parameter
            sliders[i].setVisible(false);
            toggles[i].setVisible(false);
            labels[i].setVisible(false);
        }
    }
    
    visibleParamCount = numParams;
    resized();
}

SlotComponentFixed::ControlType SlotComponentFixed::getControlTypeForParameter(int engineId, int paramIndex)
{
    // Use the embedded ParameterControlMap
    auto controlType = ParameterControlMap::getControlType(engineId, paramIndex);
    
    switch (controlType)
    {
        case ParameterControlMap::CONTROL_TOGGLE:
            return CONTROL_TOGGLE;
        case ParameterControlMap::CONTROL_ROTARY:
            return CONTROL_ROTARY;
        case ParameterControlMap::CONTROL_STEPPED:
            return CONTROL_STEPPED;
        case ParameterControlMap::CONTROL_ROTARY:
        default:
            return CONTROL_ROTARY;
    }
}

void SlotComponentFixed::configureControlForType(int paramIndex, ControlType type)
{
    // Hide all control types first
    sliders[paramIndex].setVisible(false);
    toggles[paramIndex].setVisible(false);
    
    switch (type)
    {
        case CONTROL_ROTARY:
            sliders[paramIndex].setSliderStyle(juce::Slider::RotaryVerticalDrag);
            sliders[paramIndex].setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            sliders[paramIndex].setVisible(true);
            break;
            
        case CONTROL_ROTARY:
            sliders[paramIndex].setSliderStyle(juce::Slider::LinearVertical);
            sliders[paramIndex].setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            sliders[paramIndex].setColour(juce::Slider::trackColourId, juce::Colour(0xff00ffcc));
            sliders[paramIndex].setColour(juce::Slider::backgroundColourId, juce::Colour(0xff374151));
            sliders[paramIndex].setVisible(true);
            break;
            
        case CONTROL_STEPPED:
            sliders[paramIndex].setSliderStyle(juce::Slider::RotaryVerticalDrag);
            sliders[paramIndex].setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            // Visual indicator for stepped control
            sliders[paramIndex].setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xffFBBF24));
            sliders[paramIndex].setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff374151));
            sliders[paramIndex].setVisible(true);
            break;
            
        case CONTROL_TOGGLE:
            toggles[paramIndex].setVisible(true);
            break;
    }
}

juce::Component* SlotComponentFixed::getParameterControl(int index)
{
    if (index < 0 || index >= 15)
        return nullptr;
    
    switch (controlTypes[index])
    {
        case CONTROL_TOGGLE:
            return &toggles[index];
        default:
            return &sliders[index];
    }
}

void SlotComponentFixed::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    
    // Modern gradient background
    juce::ColourGradient gradient(
        juce::Colour(0xff1F2937).withAlpha(0.95f),
        bounds.getTopLeft().toFloat(),
        juce::Colour(0xff111827).withAlpha(0.95f),
        bounds.getBottomRight().toFloat(),
        false
    );
    g.setGradientFill(gradient);
    g.fillRoundedRectangle(bounds.toFloat(), 8);
    
    // Glowing border for active slot
    if (visibleParamCount > 0)
    {
        g.setColour(juce::Colour(0xff00ffcc).withAlpha(0.4f));
        g.drawRoundedRectangle(bounds.toFloat().reduced(0.5f), 8, 2);
    }
    else
    {
        g.setColour(juce::Colour(0xff374151).withAlpha(0.5f));
        g.drawRoundedRectangle(bounds.toFloat().reduced(0.5f), 8, 1);
    }
}

void SlotComponentFixed::resized()
{
    if (!componentsInitialized) return;
    
    auto bounds = getLocalBounds().reduced(12);
    
    // Header section
    auto headerBounds = bounds.removeFromTop(25);
    slotLabel.setBounds(headerBounds);
    bounds.removeFromTop(8);
    
    // Control row
    auto controlRow = bounds.removeFromTop(32);
    auto selectorWidth = controlRow.getWidth() * 0.65f;
    engineSelector.setBounds(controlRow.removeFromLeft(selectorWidth).reduced(2));
    bypassButton.setBounds(controlRow.reduced(2));
    bounds.removeFromTop(12);
    
    // Parameter grid with dynamic layout
    if (visibleParamCount > 0 && bounds.getHeight() > 0)
    {
        layoutParametersGrid(bounds);
    }
}

void SlotComponentFixed::layoutParametersGrid(juce::Rectangle<int> bounds)
{
    int cols = calculateOptimalColumns(visibleParamCount);
    int rows = (visibleParamCount + cols - 1) / cols;
    
    // Calculate cell dimensions with proper spacing
    int cellWidth = bounds.getWidth() / cols;
    int cellHeight = juce::jmin(100, bounds.getHeight() / rows);
    
    for (int i = 0; i < visibleParamCount; ++i)
    {
        int col = i % cols;
        int row = i / cols;
        
        auto cellBounds = juce::Rectangle<int>(
            bounds.getX() + col * cellWidth,
            bounds.getY() + row * cellHeight,
            cellWidth - 6,
            cellHeight - 4
        );
        
        // Position label
        labels[i].setBounds(cellBounds.removeFromTop(16));
        
        // Position control based on type
        auto controlBounds = cellBounds.reduced(4);
        
        switch (controlTypes[i])
        {
            case CONTROL_ROTARY:
            case CONTROL_STEPPED:
            {
                int knobSize = juce::jmin(50, controlBounds.getWidth(), controlBounds.getHeight());
                sliders[i].setBounds(controlBounds.withSizeKeepingCentre(knobSize, knobSize));
                break;
            }
            
            case CONTROL_ROTARY:
            {
                int sliderWidth = juce::jmin(40, controlBounds.getWidth());
                sliders[i].setBounds(controlBounds.withWidth(sliderWidth).withX(
                    cellBounds.getCentreX() - sliderWidth/2
                ));
                break;
            }
            
            case CONTROL_TOGGLE:
            {
                int toggleHeight = 24;
                toggles[i].setBounds(controlBounds.withHeight(toggleHeight)
                                                  .withY(controlBounds.getCentreY() - toggleHeight/2));
                break;
            }
        }
    }
    
    // Hide remaining controls
    for (int i = visibleParamCount; i < 15; ++i)
    {
        sliders[i].setVisible(false);
        toggles[i].setVisible(false);
        labels[i].setVisible(false);
    }
}

int SlotComponentFixed::calculateOptimalColumns(int paramCount)
{
    // Smart column calculation based on parameter count
    if (paramCount <= 3)
        return paramCount;  // Single row
    else if (paramCount <= 6)
        return 3;  // 2 rows of 3
    else if (paramCount <= 8)
        return 4;  // 2 rows of 4
    else if (paramCount <= 10)
        return 5;  // 2 rows of 5
    else if (paramCount <= 12)
        return 4;  // 3 rows of 4
    else
        return 5;  // 3 rows of 5 for 13-15 params
}