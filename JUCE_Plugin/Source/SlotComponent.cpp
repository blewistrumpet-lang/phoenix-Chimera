#include "SlotComponent.h"
#include "NexusLookAndFeel.h"
#include "ParameterFormatter.h"
#include "IntelligentHarmonizer.h"

SlotComponent::SlotComponent(int slotIndex)
    : slotNumber(slotIndex), visibleParamCount(0), currentEngineId(-1)
{
    // DBG("SlotComponent constructor for slot " + juce::String(slotIndex));
    // No need to load external JSON - using embedded ParameterControlMap
    
    // Initialize control types to default
    for (int i = 0; i < 15; ++i) {
        controlTypes[i] = CONTROL_ROTARY;
    }
    
    // Create slot label - readable version
    // DBG("Creating slot label...");
    slotLabel.setText("Slot " + juce::String(slotNumber + 1), juce::dontSendNotification);
    slotLabel.setJustificationType(juce::Justification::centred);
    slotLabel.setColour(juce::Label::textColourId, juce::Colour(0xffe0e4f0));  // Bright readable text
    slotLabel.setFont(juce::Font(14.0f));  // Readable size
    addAndMakeVisible(slotLabel);
    // DBG("Slot label created");
    
    // Create engine selector ONCE
    // TEMPORARILY SIMPLIFIED TO PREVENT HANG
    // engineSelector.setTextWhenNothingSelected("Select Engine");
    // engineSelector.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff1F2937));
    // engineSelector.setColour(juce::ComboBox::textColourId, juce::Colour(0xffE5E7EB));
    addAndMakeVisible(engineSelector);
    
    // Create bypass button - clear labeling
    bypassButton.setButtonText("Bypass");
    bypassButton.setColour(juce::ToggleButton::textColourId, juce::Colour(0xffff4466));
    bypassButton.setColour(juce::ToggleButton::tickColourId, juce::Colour(0xffff4466));
    addAndMakeVisible(bypassButton);
    
    // Mix slider removed - engines have their own Mix parameters
    
    // Create solo button - clear labeling
    soloButton.setButtonText("Solo");
    soloButton.setColour(juce::ToggleButton::textColourId, juce::Colour(0xffffbb00));
    soloButton.setColour(juce::ToggleButton::tickColourId, juce::Colour(0xffffbb00));
    addAndMakeVisible(soloButton);
    
    // Create ALL 15 parameter controls ONCE
    for (int i = 0; i < 15; ++i)
    {
        // Configure rotary/slider control
        sliders[i].setSliderStyle(juce::Slider::RotaryVerticalDrag);
        sliders[i].setTextBoxStyle(juce::Slider::NoTextBox, false, 60, 14);  // No text box, we use value labels
        sliders[i].setRange(0.0, 1.0, 0.001);  // IMPORTANT: Set default range!
        sliders[i].setValue(0.5);  // Default value
        sliders[i].setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff00ffcc));
        sliders[i].setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff374151));
        sliders[i].setColour(juce::Slider::thumbColourId, juce::Colour(0xff00ffcc));
        sliders[i].setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xff00ffcc));
        sliders[i].setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0x00000000));
        sliders[i].setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0x00000000));
        sliders[i].setVisible(false);
        sliders[i].addListener(this);  // Add listener for value changes
        addAndMakeVisible(sliders[i]);
        sliders[i].toFront(false);  // Bring to front
        
        // Configure toggle button
        toggles[i].setColour(juce::ToggleButton::textColourId, juce::Colour(0xffE5E7EB));
        toggles[i].setColour(juce::ToggleButton::tickColourId, juce::Colour(0xff00ffcc));
        toggles[i].setColour(juce::ToggleButton::tickDisabledColourId, juce::Colour(0xff374151));
        toggles[i].setVisible(false);
        addAndMakeVisible(toggles[i]);
        
        // Configure label
        labels[i].setText("Param " + juce::String(i + 1), juce::dontSendNotification);
        labels[i].setJustificationType(juce::Justification::centred);
        labels[i].setColour(juce::Label::textColourId, juce::Colour(0xffe8ecf4));  // White for readability
        labels[i].setFont(juce::Font(12.0f));  // Readable size
        labels[i].setVisible(false);
        addAndMakeVisible(labels[i]);
        
        // Configure value label
        valueLabels[i].setText("0.0", juce::dontSendNotification);
        valueLabels[i].setJustificationType(juce::Justification::centred);
        valueLabels[i].setColour(juce::Label::textColourId, juce::Colour(0xff60d4ff));  // Bright cyan
        valueLabels[i].setFont(juce::Font(8.0f));  // Smaller value text to save space
        valueLabels[i].setVisible(false);
        addAndMakeVisible(valueLabels[i]);
        
        // Default to rotary control
        controlTypes[i] = CONTROL_ROTARY;
    }
}

SlotComponent::~SlotComponent()
{
    // Remove slider listeners
    for (int i = 0; i < 15; ++i)
    {
        sliders[i].removeListener(this);
    }
    // Components are automatically deleted by JUCE
}

// Removed loadParameterMapping() - no longer needed with embedded ParameterControlMap

void SlotComponent::update(EngineBase* engine, int engineId)
{
    currentEngineId = engineId;
    currentEngine = engine;  // Store for semantic rules
    currentEngineName = engine ? engine->getName() : "";
    
    DBG("SlotComponent::update slot " + juce::String(slotNumber) + 
        ": engine=" + juce::String::toHexString((juce::int64)engine) + 
        " engineId=" + juce::String(engineId));
    DBG("  Component bounds: " + getBounds().toString());
    
    if (engine == nullptr)
    {
        DBG("  No engine - hiding all controls");
        currentEngineName = "";
        // Hide all parameters if no engine
        for (int i = 0; i < 15; ++i)
        {
            sliders[i].setVisible(false);
            toggles[i].setVisible(false);
            labels[i].setVisible(false);
            valueLabels[i].setVisible(false);
        }
        visibleParamCount = 0;
        return;
    }
    
    // Engine name already set above
    
    // Get parameter count from live engine
    int numParams = engine->getNumParameters();
    numParams = juce::jmin(numParams, 15);
    DBG("  Engine has " + juce::String(numParams) + " parameters");
    
    // Update each parameter control
    for (int i = 0; i < 15; ++i)
    {
        if (i < numParams)
        {
            // Get parameter name from engine
            juce::String paramName = engine->getParameterName(i);
            labels[i].setText(paramName, juce::dontSendNotification);
            labels[i].setVisible(true);
            
            // Determine control type and configure
            ControlType controlType = getControlTypeForParameter(engineId, i);
            controlTypes[i] = controlType;
            configureControlForType(i, controlType);
            
            // Ensure slider is child of this component
            if (sliders[i].getParentComponent() != this) {
                DBG("    WARNING: Slider " + juce::String(i) + " is not a child!");
                addAndMakeVisible(sliders[i]);
            }
            
            // Debug output
            DBG("    Param " + juce::String(i) + ": " + paramName + 
                " visible=" + (sliders[i].isVisible() ? "true" : "false") +
                " bounds=" + sliders[i].getBounds().toString() +
                " parent=" + juce::String::toHexString((juce::int64)sliders[i].getParentComponent()));
            
            // Show value label for non-toggle controls and update initial value
            if (controlType != CONTROL_TOGGLE) {
                valueLabels[i].setVisible(true);
                // Update initial value display
                float normalizedValue = sliders[i].getValue();
                
                // Try to use engine's getParameterDisplayString if available
                juce::String formattedValue;
                if (auto* harmonizer = dynamic_cast<IntelligentHarmonizer*>(engine)) {
                    formattedValue = harmonizer->getParameterDisplayString(i, normalizedValue);
                } else {
                    // Fallback to ParameterFormatter for other engines
                    formattedValue = ParameterFormatter::formatValue(
                        currentEngineName, paramName, normalizedValue);
                }
                valueLabels[i].setText(formattedValue, juce::dontSendNotification);
            } else {
                valueLabels[i].setVisible(false);
            }
        }
        else
        {
            // Hide all controls for this parameter
            sliders[i].setVisible(false);
            toggles[i].setVisible(false);
            labels[i].setVisible(false);
            valueLabels[i].setVisible(false);
        }
    }
    
    visibleParamCount = numParams;
    
    // Force a layout update to position the controls
    resized();
    
    DBG("  After update: visibleParamCount=" + juce::String(visibleParamCount));
    DBG("  Forcing repaint...");
    repaint();
}

SlotComponent::ControlType SlotComponent::getControlTypeForParameter(int engineId, int paramIndex)
{
    // First check the ParameterControlMap
    auto controlType = ParameterControlMap::getControlType(engineId, paramIndex);
    
    // Apply semantic rules based on parameter name
    // This ensures consistency across ALL engines
    if (engineId > 0 && currentEngine) {
        juce::String paramName = currentEngine->getParameterName(paramIndex).toLowerCase();
        
        // SEMANTIC RULE 1: Time parameters now use rotary encoders (no more sliders)
        // All continuous parameters use CONTROL_ROTARY for consistency
        
        // SEMANTIC RULE 2: Discrete choices use stepped encoders
        if (paramName.contains("type") || paramName.contains("mode") ||
            paramName.contains("scale") || paramName.contains("key") ||
            paramName.contains("console") || paramName.contains("voicing") ||
            paramName.contains("oversample") || paramName.contains("oversampling")) {
            return CONTROL_STEPPED;
        }
        
        // SEMANTIC RULE 3: Boolean parameters use toggles
        if (paramName.contains("bypass") || paramName.contains("enable") ||
            paramName.contains("on/off") || paramName.contains("auto")) {
            return CONTROL_TOGGLE;
        }
    }
    
    // Fall back to map specification
    switch (controlType)
    {
        case ParameterControlMap::CONTROL_TOGGLE:
            return CONTROL_TOGGLE;
        case ParameterControlMap::CONTROL_STEPPED:
            return CONTROL_STEPPED;
        case ParameterControlMap::CONTROL_ROTARY:
        default:
            return CONTROL_ROTARY;
    }
}

void SlotComponent::configureControlForType(int paramIndex, ControlType type)
{
    // Hide all control types first
    sliders[paramIndex].setVisible(false);
    toggles[paramIndex].setVisible(false);
    
    switch (type)
    {
        case CONTROL_ROTARY:
            sliders[paramIndex].setSliderStyle(juce::Slider::RotaryVerticalDrag);
            sliders[paramIndex].setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 14);  // TRY WITH TEXT BOX
            sliders[paramIndex].setRange(0.0, 1.0, 0.001);  // Ensure range is set
            sliders[paramIndex].setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff00ffcc));
            sliders[paramIndex].setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xffffffff));  // White outline
            sliders[paramIndex].setColour(juce::Slider::thumbColourId, juce::Colour(0xffff0000));  // Red thumb
            sliders[paramIndex].setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xff00ffcc));
            sliders[paramIndex].setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff000000));
            sliders[paramIndex].setVisible(true);
            sliders[paramIndex].toFront(false);
            DBG("      Configured ROTARY for param " + juce::String(paramIndex) + " - visible: true");
            break;
            
        // CONTROL_SLIDER case removed - all parameters now use rotary encoders or toggles
            
        case CONTROL_STEPPED:
        {
            sliders[paramIndex].setSliderStyle(juce::Slider::RotaryVerticalDrag);
            sliders[paramIndex].setTextBoxStyle(juce::Slider::NoTextBox, false, 60, 14);
            // Visual indicator for stepped control - yellow color
            sliders[paramIndex].setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xffFBBF24));
            sliders[paramIndex].setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff374151));
            
            // Configure discrete steps and text display based on parameter
            if (currentEngine && currentEngineId > 0) {
                juce::String paramName = currentEngine->getParameterName(paramIndex).toLowerCase();
                
                // Scale selector - show text values
                if (paramName.contains("scale")) {
                    sliders[paramIndex].setRange(0, 6, 1);  // 7 scale types
                    
                    // Update value label to show scale name
                    valueLabels[paramIndex].setText("Major", juce::dontSendNotification);
                    valueLabels[paramIndex].setVisible(true);
                } 
                // Key/Root note selector
                else if (paramName.contains("key") || paramName.contains("root")) {
                    sliders[paramIndex].setRange(0, 11, 1);  // 12 notes
                    valueLabels[paramIndex].setText("C", juce::dontSendNotification);
                    valueLabels[paramIndex].setVisible(true);
                }
                // Type selectors
                else if (paramName.contains("type") || paramName.contains("mode") || 
                         paramName.contains("console") || paramName.contains("voicing")) {
                    sliders[paramIndex].setRange(0, 4, 1);  // 5 types
                    valueLabels[paramIndex].setText("Type 1", juce::dontSendNotification);
                    valueLabels[paramIndex].setVisible(true);
                }
                // Oversampling
                else if (paramName.contains("oversamp")) {
                    sliders[paramIndex].setRange(0, 3, 1);  // 4 options
                    valueLabels[paramIndex].setText("Off", juce::dontSendNotification);
                    valueLabels[paramIndex].setVisible(true);
                }
                else {
                    sliders[paramIndex].setRange(0, 9, 1);  // Default 10 steps
                }
            }
            
            sliders[paramIndex].setVisible(true);
            break;
        }
            
        case CONTROL_TOGGLE:
            toggles[paramIndex].setVisible(true);
            break;
    }
}

juce::Component* SlotComponent::getParameterControl(int index)
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

void SlotComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Warmer, more musical background
    g.setColour(juce::Colour(0xff1a1a22));  // Warm dark blue-gray
    g.fillRoundedRectangle(bounds, 6);
    
    // Visible but subtle border
    g.setColour(juce::Colour(0xff2a2a38));  // Visible border
    g.drawRoundedRectangle(bounds, 6, 1.5f);
    
    // Different appearance based on state
    bool hasEngine = (currentEngineId > 0);
    
    if (!hasEngine && getHeight() > 100) {
        // Empty slot indicator
        g.setColour(juce::Colour(0xff2a2a35).withAlpha(0.5f));
        g.setFont(13.0f);
        g.drawText("Empty Slot", bounds, juce::Justification::centred);
    } else if (hasEngine && visibleParamCount > 0) {
        // Active slot with subtle glow
        g.setColour(juce::Colour(0xff4090ff).withAlpha(0.08f));
        g.fillRoundedRectangle(bounds.reduced(2), 5);
    }
}

void SlotComponent::resized()
{
    auto bounds = getLocalBounds().reduced(4);  // Tighter margins for more space
    
    // HEADER: Compact header (28px)
    auto headerBounds = bounds.removeFromTop(28);
    
    // Slot label on left (45px wide)
    slotLabel.setBounds(headerBounds.removeFromLeft(45));
    
    // Solo button on right (45px)
    soloButton.setBounds(headerBounds.removeFromRight(45).reduced(1));
    headerBounds.removeFromRight(2);  // Tiny gap
    
    // Bypass button on right (55px)
    bypassButton.setBounds(headerBounds.removeFromRight(55).reduced(1));
    headerBounds.removeFromRight(4);  // Small gap before selector
    
    // Engine selector takes remaining center space
    engineSelector.setBounds(headerBounds.reduced(1, 2));
    
    bounds.removeFromTop(6);  // Smaller gap before parameters
    
    // Parameter grid with dynamic layout
    DBG("resized() - visibleParamCount=" + juce::String(visibleParamCount) + 
        " bounds height=" + juce::String(bounds.getHeight()));
    if (visibleParamCount > 0)
    {
        // Ensure bounds are valid even if height is initially 0
        if (bounds.getHeight() <= 0) {
            // Calculate needed height based on parameter count using unified sizing
            int rows = (visibleParamCount + 4) / 5;  // 5 columns max
            int totalControlHeight = LABEL_HEIGHT + KNOB_SIZE + VALUE_HEIGHT;
            int neededHeight = rows * (totalControlHeight + 8) + 20;  // Spacing + padding
            bounds.setHeight(juce::jmax(200, neededHeight));
        }
        // Use grid layout for now - semantic layout needs debugging
        layoutParametersGrid(bounds);
    }
    else
    {
        DBG("  NOT laying out parameters - count=" + juce::String(visibleParamCount));
    }
}

void SlotComponent::layoutParametersGrid(juce::Rectangle<int> bounds)
{
    if (visibleParamCount == 0) {
        // Empty slot - hide all controls
        for (int i = 0; i < 15; ++i) {
            sliders[i].setVisible(false);
            toggles[i].setVisible(false);
            labels[i].setVisible(false);
            valueLabels[i].setVisible(false);
        }
        return;
    }
    
    int cols = calculateOptimalColumns(visibleParamCount);
    if (cols == 0) return;  // Safety check
    
    int rows = (visibleParamCount + cols - 1) / cols;
    
    // Dynamic sizing - fit parameters within available space
    int availableWidth = bounds.getWidth();
    int availableHeight = bounds.getHeight();
    
    // Calculate control sizes based on available space
    // Leave proper padding for visual breathing room
    int padding = 12;  // Increased from 8 for better spacing
    int usableWidth = availableWidth - (2 * padding);
    int usableHeight = availableHeight - (2 * padding);
    
    // Calculate cell size based on available space
    int cellWidth = usableWidth / cols;
    int cellHeight = usableHeight / rows;
    
    // Calculate optimal encoder size based on available space
    int labelAndValueHeight = LABEL_HEIGHT + VALUE_HEIGHT + 8;  // Include spacing
    int availableEncoderHeight = cellHeight - labelAndValueHeight;
    int availableEncoderWidth = cellWidth - 20;  // Leave margins
    
    int knobSize = juce::jmin(availableEncoderWidth, availableEncoderHeight);
    knobSize = juce::jmin(knobSize, 55);  // Cap at reasonable maximum
    knobSize = juce::jmax(knobSize, 38);  // Minimum for usability
    
    // Calculate actual control dimensions
    int totalControlWidth = juce::jmin(cellWidth - 4, 80);  // Prevent label truncation
    int totalControlHeight = LABEL_HEIGHT + knobSize + VALUE_HEIGHT;
    
    for (int i = 0; i < visibleParamCount; ++i)
    {
        int col = i % cols;
        int row = i / cols;
        
        // Calculate position using dynamic cell sizes
        int x = bounds.getX() + padding + col * cellWidth;
        int y = bounds.getY() + padding + row * cellHeight;
        
        // Center the control within the cell
        int controlX = x + (cellWidth - totalControlWidth) / 2;
        int controlY = y + (cellHeight - totalControlHeight) / 2;
        
        // Create bounds for the entire control stack
        auto controlStackBounds = juce::Rectangle<int>(controlX, controlY, totalControlWidth, totalControlHeight);
        
        // Position label at top
        auto labelBounds = controlStackBounds.removeFromTop(LABEL_HEIGHT);
        labels[i].setBounds(labelBounds);
        labels[i].setVisible(true);
        
        // Add spacing before value label
        controlStackBounds.removeFromBottom(2);  // Small gap
        
        // Reserve space for value label at bottom
        auto valueLabelBounds = controlStackBounds.removeFromBottom(VALUE_HEIGHT);
        valueLabels[i].setBounds(valueLabelBounds);
        valueLabels[i].setVisible(true);
        
        // The remaining space is for the control itself
        auto controlBounds = controlStackBounds;
        
        switch (controlTypes[i])
        {
            case CONTROL_ROTARY:
            case CONTROL_STEPPED:
            {
                // Dynamic knob size based on available space
                auto knobBounds = controlBounds.withSizeKeepingCentre(knobSize, knobSize);
                sliders[i].setBounds(knobBounds);
                sliders[i].setVisible(true);
                toggles[i].setVisible(false);
                break;
            }
            
            case CONTROL_TOGGLE:
            {
                toggles[i].setBounds(controlBounds.withHeight(TOGGLE_HEIGHT)
                                                  .withY(controlBounds.getCentreY() - TOGGLE_HEIGHT/2));
                sliders[i].setVisible(false);
                toggles[i].setVisible(true);
                // No value label for toggles
                valueLabels[i].setVisible(false);
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
        valueLabels[i].setVisible(false);
    }
}

int SlotComponent::calculateOptimalColumns(int paramCount)
{
    // Professional-grade column calculation for flawless layout
    // Optimized for visual balance and usability
    
    if (paramCount == 0)
        return 0;  // Empty slot
    else if (paramCount <= 5)
        return paramCount;  // Single row: 1-5 params in one row
    else if (paramCount <= 10)
        return 5;  // Two rows: 6-10 params as 2 rows of 5 max
    else if (paramCount <= 15)
        return 5;  // Three rows: 11-15 params as 3 rows of 5 max
    else
        return 5;  // Fallback for any future expansion
}

int SlotComponent::getRequiredHeight() const
{
    // Request heights that fit within 798px / 3 = 266px per row max
    if (visibleParamCount == 0) return 120;  // Compact for empty slot
    
    int rows = (visibleParamCount + 4) / 5;  // 5 columns max
    
    // Conservative heights to ensure everything fits
    if (rows == 3) {
        return 250;  // Max height for 3 rows (leaves room for all slots)
    } else if (rows == 2) {
        return 200;  // Good height for 2 rows
    } else {
        return 150;  // Standard height for 1 row
    }
}

void SlotComponent::sliderValueChanged(juce::Slider* slider)
{
    // Find which parameter this slider corresponds to
    for (int i = 0; i < 15; ++i)
    {
        if (&sliders[i] == slider)
        {
            // Update the value label
            if (valueLabels[i].isVisible() && currentEngine)
            {
                float normalizedValue = slider->getValue();
                juce::String formattedValue;
                
                // Try to use engine's getParameterDisplayString if available
                if (auto* harmonizer = dynamic_cast<IntelligentHarmonizer*>(currentEngine)) {
                    formattedValue = harmonizer->getParameterDisplayString(i, normalizedValue);
                }
                else {
                    // Fallback to ParameterFormatter for other engines
                    juce::String paramName = currentEngine->getParameterName(i);
                    formattedValue = ParameterFormatter::formatValue(
                        currentEngineName, paramName, normalizedValue);
                }
                
                valueLabels[i].setText(formattedValue, juce::dontSendNotification);
            }
            break;
        }
    }
}

// Visual hierarchy functions removed - all encoders now use unified KNOB_SIZE


// Semantic layout function removed - using unified grid layout for consistency