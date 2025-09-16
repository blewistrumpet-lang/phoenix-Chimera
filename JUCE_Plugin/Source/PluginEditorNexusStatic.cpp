#include "PluginEditorNexusStatic.h"
#include "NexusLookAndFeel.h"
#include "TrinityProtocol.h"

PluginEditorNexusStatic::PluginEditorNexusStatic(ChimeraAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    // DBG("PluginEditorNexusStatic constructor starting...");
    
    // Apply Tactile Futurism aesthetic
    nexusLookAndFeel = std::make_unique<NexusLookAndFeel>();
    setLookAndFeel(nexusLookAndFeel.get());
    
    // IMPORTANT: Don't set size yet - it triggers resized() before slots are created!
    // setSize(1200, 800);
    setResizable(false, false);
    
    // === TRINITY AI INITIALIZATION ===
    initializeTrinityAI();
    
    // Create title ONCE
    titleLabel.setText("CHIMERA PHOENIX - NEXUS ENGINE", juce::dontSendNotification);
    titleLabel.setFont(juce::Font("Roboto Condensed", 28.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setColour(juce::Label::textColourId, juce::Colour(0xff00ffcc));
    addAndMakeVisible(titleLabel);
    
    // Create preset name label
    presetNameLabel.setText(currentPresetName, juce::dontSendNotification);
    presetNameLabel.setJustificationType(juce::Justification::centred);
    presetNameLabel.setFont(juce::Font(18.0f, juce::Font::bold));
    presetNameLabel.setColour(juce::Label::textColourId, juce::Colour(0xff00ff88));
    addAndMakeVisible(presetNameLabel);
    
    // Create all 6 slots ONCE
    // DBG("Creating 6 slot components...");
    for (int i = 0; i < 6; ++i)
    {
        // DBG("Creating slot " + juce::String(i));
        slots[i] = std::make_unique<SlotComponent>(i);
        slots[i]->setLookAndFeel(nexusLookAndFeel.get());  // Apply Nexus aesthetic
        addAndMakeVisible(slots[i].get());
        
        // CRITICAL: Defer initialization until after construction
        // initializeSlot(i) will be called after constructor completes
    }
    
    // Re-enabled with safety - skip ComboBox attachments for now
    for (int i = 0; i < 6; ++i)
    {
        // Set up attachments (modified to skip ComboBox)
        initializeSlotSafe(i);
    }
    
    // Add parameter listeners for engine changes
    for (int i = 0; i < 6; ++i)
    {
        juce::String engineParam = "slot" + juce::String(i + 1) + "_engine";
        audioProcessor.getValueTreeState().addParameterListener(engineParam, this);
    }
    
    // Initial update for all slots
    for (int i = 0; i < 6; ++i)
    {
        updateSlotEngine(i);
    }
    
    // NOW safe to set size after all components are created - updated for Trinity
    // Use a more reasonable height that fits on most screens
    setSize(1200, 880);  // Adjusted to show Trinity text box clearly
    
    // Start a one-shot timer to create ComboBox attachments after UI is ready
    startTimer(50);  // 50ms delay
}

PluginEditorNexusStatic::~PluginEditorNexusStatic()
{
    stopTimer();
    
    // Clean up Trinity AI components
    if (trinityClient) {
        trinityClient->removeListener(this);
        trinityClient->disconnect();
    }
    
    // Remove parameter listeners
    for (int i = 0; i < 6; ++i)
    {
        juce::String engineParam = "slot" + juce::String(i + 1) + "_engine";
        audioProcessor.getValueTreeState().removeParameterListener(engineParam, this);
    }
    
    // Clean up look and feel
    if (trinityTextBox) {
        trinityTextBox->setLookAndFeel(nullptr);
    }
    for (int i = 0; i < 6; ++i)
    {
        if (slots[i])
            slots[i]->setLookAndFeel(nullptr);
    }
    setLookAndFeel(nullptr);
}

void PluginEditorNexusStatic::initializeSlotSafe(int slotIndex)
{
    if (slotIndex < 0 || slotIndex >= 6) return;
    
    auto& slot = slots[slotIndex];
    auto& attachments = slotAttachments[slotIndex];
    
    // Populate engine selector but DON'T create attachment yet
    populateEngineSelector(slotIndex);
    
    // Skip ComboBox attachment - that's what causes the hang!
    // We'll create it later after the message loop is running
    
    // Create bypass attachment - this is safe
    juce::String bypassParam = "slot" + juce::String(slotIndex + 1) + "_bypass";
    attachments.bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.getValueTreeState(), bypassParam, slot->getBypassButton()
    );
    
    // Mix attachment removed - engines manage their own Mix parameters
    
    // Create solo attachment - this is safe
    juce::String soloParam = "slot" + juce::String(slotIndex + 1) + "_solo";
    attachments.soloAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.getValueTreeState(), soloParam, slot->getSoloButton()
    );
    
    // Create parameter attachments for sliders - these are safe
    for (int i = 0; i < 15; ++i)
    {
        juce::String paramName = "slot" + juce::String(slotIndex + 1) + "_param" + juce::String(i + 1);
        
        auto* slider = slot->getSlider(i);
        if (slider)
        {
            attachments.paramAttachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                audioProcessor.getValueTreeState(), paramName, *slider
            );
        }
    }
}

void PluginEditorNexusStatic::initializeSlot(int slotIndex)
{
    if (slotIndex < 0 || slotIndex >= 6) return;
    
    auto& slot = slots[slotIndex];
    auto& attachments = slotAttachments[slotIndex];
    
    // Populate engine selector
    populateEngineSelector(slotIndex);
    
    // Create engine selector attachment
    juce::String engineParam = "slot" + juce::String(slotIndex + 1) + "_engine";
    attachments.engineAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.getValueTreeState(), engineParam, slot->getEngineSelector()
    );
    
    // Create bypass attachment
    juce::String bypassParam = "slot" + juce::String(slotIndex + 1) + "_bypass";
    attachments.bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.getValueTreeState(), bypassParam, slot->getBypassButton()
    );
    
    // Create parameter attachments for ALL 15 possible parameters
    // For now, just attach to sliders - the SlotComponent will handle control type visibility
    for (int i = 0; i < 15; ++i)
    {
        juce::String paramName = "slot" + juce::String(slotIndex + 1) + "_param" + juce::String(i + 1);
        
        // Always use slider for now - SlotComponent will show/hide appropriate control
        auto* slider = slot->getSlider(i);
        if (slider)
        {
            attachments.paramAttachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                audioProcessor.getValueTreeState(), paramName, *slider
            );
        }
    }
}

void PluginEditorNexusStatic::populateEngineSelector(int slotIndex)
{
    if (slotIndex < 0 || slotIndex >= 6) return;
    
    auto& selector = slots[slotIndex]->getEngineSelector();
    selector.clear();
    
    // Get engine choices from parameter
    juce::String engineParam = "slot" + juce::String(slotIndex + 1) + "_engine";
    auto* param = audioProcessor.getValueTreeState().getParameter(engineParam);
    
    if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(param))
    {
        for (int i = 0; i < choiceParam->choices.size(); ++i)
        {
            selector.addItem(choiceParam->choices[i], i + 1);
        }
    }
}

void PluginEditorNexusStatic::updateSlotEngine(int slotIndex)
{
    if (slotIndex < 0 || slotIndex >= 6) return;
    if (!slots[slotIndex]) return;  // Safety check
    
    // Get the current engine for this slot
    try {
        auto& engine = audioProcessor.getEngine(slotIndex);
        
        // Get the engine ID from the parameter value
        juce::String engineParam = "slot" + juce::String(slotIndex + 1) + "_engine";
        auto* param = audioProcessor.getValueTreeState().getParameter(engineParam);
        int engineId = 0;  // Default to ENGINE_NONE
        
        if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(param))
        {
            engineId = choiceParam->getIndex();
        }
        
        DBG("UpdateSlotEngine: slot=" + juce::String(slotIndex) + 
            " engineId=" + juce::String(engineId) + 
            " engine=" + juce::String::toHexString((juce::int64)engine.get()));
        
        // Update the slot component's visibility and content
        // This does NOT create/destroy components, only changes visibility
        DBG("  About to update slot component with engine: " + 
            juce::String::toHexString((juce::int64)engine.get()));
        if (engine) {
            DBG("    Engine name: " + engine->getName());
            DBG("    Param count: " + juce::String(engine->getNumParameters()));
        }
        slots[slotIndex]->update(engine.get(), engineId);
    }
    catch (...) {
        DBG("UpdateSlotEngine: Exception - updating with nullptr");
        // If engine isn't ready yet, just update with no engine
        slots[slotIndex]->update(nullptr, 0);
    }
}

void PluginEditorNexusStatic::timerCallback()
{
    // One-shot timer to create ComboBox attachments after UI is ready
    stopTimer();
    
    if (!comboBoxAttachmentsCreated)
    {
        createComboBoxAttachments();
        comboBoxAttachmentsCreated = true;
    }
}

void PluginEditorNexusStatic::createComboBoxAttachments()
{
    // Now safe to create ComboBox attachments
    for (int i = 0; i < 6; ++i)
    {
        if (!slots[i]) continue;
        
        auto& attachments = slotAttachments[i];
        juce::String engineParam = "slot" + juce::String(i + 1) + "_engine";
        
        // Create the ComboBox attachment now that the UI is ready
        attachments.engineAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            audioProcessor.getValueTreeState(), engineParam, slots[i]->getEngineSelector()
        );
    }
}

void PluginEditorNexusStatic::parameterChanged(const juce::String& parameterID, float newValue)
{
    DBG("PluginEditorNexusStatic::parameterChanged: " + parameterID + " = " + juce::String(newValue));
    
    // Check if this is an engine change
    for (int i = 0; i < 6; ++i)
    {
        juce::String engineParam = "slot" + juce::String(i + 1) + "_engine";
        if (parameterID == engineParam)
        {
            // Engine changed - schedule an update after a brief delay
            // to allow the processor to create the engine first
            const int slotToUpdate = i;
            juce::Timer::callAfterDelay(100, [this, slotToUpdate]() {
                DBG("Delayed update for slot " + juce::String(slotToUpdate));
                updateSlotEngine(slotToUpdate);
            });
            
            // === TRINITY AI INTEGRATION ===
            // Don't send engine changes to Trinity automatically
            // This was causing a feedback loop where Trinity generates presets in response
            
            break;
        }
    }
    
    // Send any parameter change to Trinity for context (if connected)
    if (trinityClient && trinityClient->isConnected()) {
        // Determine slot index from parameter ID
        if (parameterID.startsWith("slot") && parameterID.contains("_param")) {
            int slotIndex = parameterID.getCharPointer()[4] - '1'; // slot1_param1 -> 0
            if (slotIndex >= 0 && slotIndex < 6) {
                // Extract parameter name (e.g., "param1", "param2", etc.)
                juce::String paramName = parameterID.substring(parameterID.lastIndexOf("_") + 1);
                
                juce::var message = TrinityProtocol::createParameterChangeMessage(
                    trinityClient->getCurrentSessionId(),
                    slotIndex,
                    paramName,
                    newValue,
                    "parameter_adjustment"
                );
                
                TrinityNetworkClient::TrinityMessage trinityMsg;
                trinityMsg.type = "parameter_change";
                trinityMsg.content = parameterID + " changed to " + juce::String(newValue);
                trinityMsg.data = message;
                trinityMsg.sessionId = trinityClient->getCurrentSessionId();
                trinityMsg.timestamp = juce::Time::currentTimeMillis();
                
                trinityClient->sendMessage(trinityMsg);
            }
        }
    }
}

void PluginEditorNexusStatic::paint(juce::Graphics& g)
{
    // Warmer gradient background
    juce::ColourGradient bgGradient(juce::Colour(0xff14141c), 0, 0,
                                    juce::Colour(0xff1a1a24), getWidth(), getHeight(), false);
    g.setGradientFill(bgGradient);
    g.fillAll();
    
    // Subtle grid dividers
    g.setColour(juce::Colour(0xff252530));
    
    // Vertical divider
    float midX = getWidth() / 2.0f;
    g.drawLine(midX, 60, midX, getHeight(), 2);
    
    // Horizontal dividers
    float slotHeight = (getHeight() - 60) / 3.0f;
    for (int i = 1; i < 3; ++i)
    {
        float y = 60 + i * slotHeight;
        g.drawLine(0, y, getWidth(), y, 2);
    }
}

void PluginEditorNexusStatic::resized()
{
    auto bounds = getLocalBounds();
    DBG("Window resized to: " + juce::String(bounds.getWidth()) + "x" + juce::String(bounds.getHeight()));
    
    // Title at top - smaller to save space
    auto titleArea = bounds.removeFromTop(40);
    titleLabel.setBounds(titleArea.removeFromLeft(titleArea.getWidth() * 0.7f).reduced(10));
    presetNameLabel.setBounds(titleArea.reduced(10));
    
    // Trinity AI text box at bottom - larger for better usability
    if (trinityTextBox && trinityTextBox->isVisible()) {
        auto trinityArea = bounds.removeFromBottom(100).reduced(10, 5);  // Increased from 60 to 100
        trinityTextBox->setBounds(trinityArea);
        bounds.removeFromBottom(5); // Spacing
    }
    
    // Dynamic slot heights based on parameter count
    float slotWidth = bounds.getWidth() / 2.0f;
    
    // Calculate required heights for each row
    int rowHeights[3] = {0, 0, 0};
    int totalRequiredHeight = 0;
    
    // Calculate optimal heights based on available space
    // With 900px total - 40px title - 62px trinity = 798px for slots
    int availableHeight = bounds.getHeight();
    int targetHeightPerRow = availableHeight / 3;  // Equal distribution
    
    for (int row = 0; row < 3; ++row)
    {
        int maxRequired = 100;  // Minimum for empty slots
        for (int col = 0; col < 2; ++col)
        {
            int i = row * 2 + col;
            if (i < 6 && slots[i])
            {
                int required = slots[i]->getRequiredHeight();
                maxRequired = juce::jmax(maxRequired, required);
            }
        }
        // Use the smaller of required or target to ensure all fit
        rowHeights[row] = juce::jmin(maxRequired, targetHeightPerRow);
        totalRequiredHeight += rowHeights[row];
    }
    
    // Ensure we use all available space efficiently
    // availableHeight already declared above
    
    // If we have extra space, distribute it
    if (totalRequiredHeight < availableHeight)
    {
        int extraSpace = availableHeight - totalRequiredHeight;
        int extraPerRow = extraSpace / 3;
        for (int row = 0; row < 3; ++row)
        {
            rowHeights[row] += extraPerRow;
        }
    }
    // If we need to compress, do so evenly
    else if (totalRequiredHeight > availableHeight)
    {
        // Force equal heights when space is tight
        int equalHeight = availableHeight / 3;
        for (int row = 0; row < 3; ++row)
        {
            rowHeights[row] = equalHeight;
        }
    }
    
    // Position slots with dynamic heights
    int yPos = bounds.getY();
    for (int row = 0; row < 3; ++row)
    {
        for (int col = 0; col < 2; ++col)
        {
            int i = row * 2 + col;
            if (i < 6 && slots[i])
            {
                slots[i]->setBounds(
                    col * slotWidth + 5,
                    yPos + 5,
                    slotWidth - 10,
                    rowHeights[row] - 10
                );
            }
        }
        yPos += rowHeights[row];
    }
}

// === TRINITY AI INTEGRATION IMPLEMENTATION ===

void PluginEditorNexusStatic::initializeTrinityAI() {
    // Create Trinity network client
    trinityClient = std::make_unique<TrinityNetworkClient>();
    trinityClient->addListener(this);
    
    // Create Trinity text box with glow effects
    trinityTextBox = std::make_unique<TrinityTextBox>();
    trinityTextBox->setTrinityClient(trinityClient.get());
    trinityTextBox->setLookAndFeel(nexusLookAndFeel.get());
    addAndMakeVisible(trinityTextBox.get());
    
    // Set callback for when a preset is modified via the Alter button
    trinityTextBox->onPresetModified = [this](const juce::var& modifiedPreset) {
        // Apply the modified preset
        applyTrinityPresetFromParameters(modifiedPreset);
        
        // Update the preset name label if it has one
        if (modifiedPreset.hasProperty("name")) {
            currentPresetName = modifiedPreset["name"].toString();
            presetNameLabel.setText(currentPresetName, juce::dontSendNotification);
        }
    };
    
    // Configure Trinity client for local server
    TrinityNetworkClient::TrinityConfig config;
    config.cloudEndpoint = "ws://localhost:8000/ws";  // Local WebSocket
    config.httpEndpoint = "http://localhost:8000";     // Local HTTP server
    config.enableAutoReconnect = true;
    config.connectionTimeoutMs = 5000;  // Faster timeout for local
    config.heartbeatIntervalMs = 30000;
    
    trinityClient->setConfig(config);
    
    // Auto-connect to Trinity
    juce::Timer::callAfterDelay(1000, [this]() {
        if (trinityClient) {
            trinityClient->connectToTrinity();
        }
    });
}

void PluginEditorNexusStatic::sendPluginStateToTrinity() {
    if (!trinityClient || !trinityClient->isConnected()) {
        return;
    }
    
    // Collect current plugin state
    juce::Array<juce::var> slotStates;
    
    for (int i = 0; i < 6; ++i) {
        // Get current engine ID for this slot
        int engineId = audioProcessor.getEngineIDForSlot(i);
        juce::String engineName = "Unknown Engine"; // Could be enhanced with engine name lookup
        
        // Get bypass and solo states
        auto* bypassParam = audioProcessor.getValueTreeState().getRawParameterValue("slot" + juce::String(i + 1) + "_bypass");
        auto* soloParam = audioProcessor.getValueTreeState().getRawParameterValue("slot" + juce::String(i + 1) + "_solo");
        
        bool bypassed = bypassParam ? bypassParam->load() > 0.5f : false;
        bool soloed = soloParam ? soloParam->load() > 0.5f : false;
        
        // Collect parameter values
        juce::Array<juce::var> parameters;
        for (int p = 0; p < 15; ++p) {
            juce::String paramName = "slot" + juce::String(i + 1) + "_param" + juce::String(p + 1);
            auto* param = audioProcessor.getValueTreeState().getRawParameterValue(paramName);
            
            if (param) {
                float value = param->load();
                juce::var paramData = TrinityProtocol::createParameter(
                    "param" + juce::String(p + 1),
                    value,
                    0.5f, // Default value - could be enhanced
                    "",   // Category - could be enhanced
                    ""    // Unit - could be enhanced
                );
                parameters.add(paramData);
            }
        }
        
        juce::var slotState = TrinityProtocol::createSlotState(
            i, engineId, engineName, bypassed, soloed, parameters
        );
        slotStates.add(slotState);
    }
    
    // Send plugin state to Trinity
    trinityClient->sendPluginState(TrinityProtocol::createPluginStateMessage(
        trinityClient->getCurrentSessionId(),
        slotStates
    ));
}

void PluginEditorNexusStatic::applyTrinityPresetFromParameters(const juce::var& presetData) {
    if (!presetData.isObject()) return;
    
    DBG("Applying Trinity preset from parameters...");
    
    // Get the parameters object
    juce::var params = presetData.getProperty("parameters", juce::var());
    if (!params.isObject()) {
        DBG("No parameters object found");
        return;
    }
    
    auto& valueTree = audioProcessor.getValueTreeState();
    
    // Apply each parameter
    for (int slot = 0; slot < 6; ++slot) {
        // Apply engine
        juce::String engineParam = "slot" + juce::String(slot + 1) + "_engine";
        if (params.hasProperty(engineParam)) {
            float engineId = params.getProperty(engineParam, 0.0f);
            if (auto* param = valueTree.getParameter(engineParam)) {
                // Convert engine ID to normalized value (0-1 range)
                // Engine IDs are 0-56, and AudioParameterChoice expects normalized values
                float normalizedValue = param->convertTo0to1(engineId);
                param->setValueNotifyingHost(normalizedValue);
                DBG("Set " << engineParam << " engineId=" << engineId << " normalized=" << normalizedValue);
            }
        }
        
        // Apply bypass
        juce::String bypassParam = "slot" + juce::String(slot + 1) + "_bypass";
        if (params.hasProperty(bypassParam)) {
            float bypass = params.getProperty(bypassParam, 0.0f);
            if (auto* param = valueTree.getRawParameterValue(bypassParam)) {
                param->store(bypass);
            }
        }
        
        // Apply mix
        juce::String mixParam = "slot" + juce::String(slot + 1) + "_mix";
        if (params.hasProperty(mixParam)) {
            float mix = params.getProperty(mixParam, 0.5f);
            if (auto* param = valueTree.getRawParameterValue(mixParam)) {
                param->store(mix);
            }
        }
        
        // Apply parameters 1-15
        for (int p = 1; p <= 15; ++p) {
            juce::String paramName = "slot" + juce::String(slot + 1) + "_param" + juce::String(p);
            if (params.hasProperty(paramName)) {
                float value = params.getProperty(paramName, 0.5f);
                if (auto* param = valueTree.getRawParameterValue(paramName)) {
                    param->store(value);
                }
            }
        }
    }
    
    // Update all slot UIs
    for (int i = 0; i < 6; ++i) {
        updateSlotEngine(i);
    }
    
    DBG("Trinity preset applied successfully");
}

void PluginEditorNexusStatic::applyTrinityParameterSuggestions(const juce::Array<juce::var>& suggestions) {
    for (const auto& suggestion : suggestions) {
        if (!suggestion.isObject()) continue;
        
        int slotIndex = suggestion.getProperty("slot_index", -1);
        juce::String parameterName = suggestion.getProperty("parameter_name", "").toString();
        float newValue = suggestion.getProperty("value", 0.0f);
        
        if (slotIndex >= 0 && slotIndex < 6 && !parameterName.isEmpty()) {
            // Apply the suggested parameter change
            juce::String fullParamName = "slot" + juce::String(slotIndex + 1) + "_" + parameterName;
            auto* param = audioProcessor.getValueTreeState().getParameter(fullParamName);
            
            if (param) {
                // Animate the parameter change for visual feedback
                param->setValueNotifyingHost(newValue);
                
                // Flash the corresponding slot to show it was modified
                if (slots[slotIndex]) {
                    // Could add visual feedback here
                }
                
                DBG("Trinity applied suggestion: " << fullParamName << " = " << newValue);
            }
        }
    }
    
    // Send updated state back to Trinity for context
    juce::Timer::callAfterDelay(500, [this]() {
        sendPluginStateToTrinity();
    });
}

void PluginEditorNexusStatic::applyTrinityPreset(const juce::var& presetData) {
    if (!presetData.isObject()) return;
    
    DBG("Applying Trinity preset...");
    
    // Parse preset data and apply to plugin
    if (presetData.hasProperty("slots")) {
        juce::var slotsData = presetData.getProperty("slots", juce::var());
        
        if (slotsData.isArray()) {
            for (int i = 0; i < slotsData.size() && i < 6; ++i) {
                juce::var slotData = slotsData[i];
                
                if (slotData.isObject()) {
                    // Apply engine selection
                    if (slotData.hasProperty("engine_id")) {
                        int engineId = slotData.getProperty("engine_id", 0);
                        audioProcessor.setSlotEngine(i, engineId);
                    }
                    
                    // Apply parameters
                    if (slotData.hasProperty("parameters")) {
                        juce::var paramsData = slotData.getProperty("parameters", juce::var());
                        
                        if (paramsData.isArray()) {
                            for (int p = 0; p < paramsData.size(); ++p) {
                                juce::var paramData = paramsData[p];
                                
                                if (paramData.isObject()) {
                                    juce::String paramName = paramData.getProperty("name", "").toString();
                                    float value = paramData.getProperty("value", 0.5f);
                                    
                                    juce::String fullParamName = "slot" + juce::String(i + 1) + "_" + paramName;
                                    auto* param = audioProcessor.getValueTreeState().getParameter(fullParamName);
                                    
                                    if (param) {
                                        param->setValueNotifyingHost(value);
                                    }
                                }
                            }
                        }
                    }
                    
                    // Apply bypass/solo states
                    if (slotData.hasProperty("bypassed")) {
                        bool bypassed = slotData.getProperty("bypassed", false);
                        juce::String bypassParam = "slot" + juce::String(i + 1) + "_bypass";
                        auto* param = audioProcessor.getValueTreeState().getParameter(bypassParam);
                        if (param) {
                            param->setValueNotifyingHost(bypassed ? 1.0f : 0.0f);
                        }
                    }
                }
            }
        }
    }
    
    // Update UI and send new state to Trinity
    repaint();
    juce::Timer::callAfterDelay(1000, [this]() {
        sendPluginStateToTrinity();
    });
}

// === TRINITY CLIENT LISTENER IMPLEMENTATION ===

void PluginEditorNexusStatic::trinityConnectionStateChanged(TrinityNetworkClient::ConnectionState newState) {
    juce::MessageManager::callAsync([this, newState]() {
        DBG("Trinity connection state changed: " << (int)newState);
        
        // Update UI elements based on connection state
        if (newState == TrinityNetworkClient::ConnectionState::Connected) {
            // Send initial plugin state when connected
            sendPluginStateToTrinity();
        }
    });
}

void PluginEditorNexusStatic::trinityMessageReceived(const TrinityNetworkClient::TrinityResponse& response) {
    juce::MessageManager::callAsync([this, response]() {
        // Re-enable text box when we receive any response
        trinityTextBox->setEnabled(true);
        
        if (!response.success) {
            // Show error message
            trinityTextBox->showResponse("❌ Error: " + response.message, true);
            juce::Timer::callAfterDelay(3000, [this]() {
                trinityTextBox->clearResponse();
            });
            return;
        }
        
        // Handle different types of Trinity responses
        if (response.type == "suggestion") {
            // Apply parameter suggestions
            if (TrinityProtocol::hasParameterSuggestions(response.data)) {
                auto suggestions = TrinityProtocol::getParameterSuggestions(response.data);
                applyTrinityParameterSuggestions(suggestions);
            }
        }
        else if (response.type == "preset") {
            // Apply preset data
            if (TrinityProtocol::hasPresetData(response.data)) {
                auto presetData = TrinityProtocol::getPresetData(response.data);
                applyTrinityPreset(presetData);
            }
        }
        else if (response.type == "response") {
            // Handle general AI responses
            DBG("Trinity response: " << response.message);
            
            // Show the creative preset name to the user in the Trinity text box
            if (response.message.isNotEmpty()) {
                // Display success with the creative preset name
                juce::String displayMessage = "✅ Generated: '" + response.message + "'";
                trinityTextBox->showResponse(displayMessage);
                
                // Clear the text after 5 seconds
                juce::Timer::callAfterDelay(5000, [this]() {
                    trinityTextBox->clearResponse();
                });
            }
            
            // Check if response contains preset data
            // Only apply if this was an explicit preset request, not a parameter change notification
            if (response.type != "parameter_change" && 
                response.data.isObject() && 
                response.data.hasProperty("parameters")) {
                DBG("Response contains preset parameters - applying...");
                applyTrinityPresetFromParameters(response.data);
                
                // Update the Trinity text box with the current preset for alter functionality
                trinityTextBox->setCurrentPreset(response.data);
                
                // Update the preset name label
                if (response.data.hasProperty("name")) {
                    currentPresetName = response.data.getProperty("name", "Untitled").toString();
                    presetNameLabel.setText(currentPresetName, juce::sendNotification);
                }
            }
        }
    });
}

void PluginEditorNexusStatic::trinitySessionStarted(const juce::String& sessionId) {
    juce::MessageManager::callAsync([this, sessionId]() {
        DBG("Trinity session started: " << sessionId);
        
        // Send initial plugin state for context
        sendPluginStateToTrinity();
    });
}

void PluginEditorNexusStatic::trinitySessionEnded(const juce::String& sessionId) {
    juce::MessageManager::callAsync([this, sessionId]() {
        DBG("Trinity session ended: " << sessionId);
    });
}

void PluginEditorNexusStatic::trinityError(const juce::String& error) {
    juce::MessageManager::callAsync([this, error]() {
        DBG("Trinity error: " << error);
        
        // Could show error notification in UI
    });
}

