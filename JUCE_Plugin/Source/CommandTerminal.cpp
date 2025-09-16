#include "CommandTerminal.h"

CommandTerminal::CommandTerminal()
{
    // Configure output display
    outputDisplay.setMultiLine(true);
    outputDisplay.setReadOnly(true);
    outputDisplay.setScrollbarsShown(true);
    outputDisplay.setCaretVisible(false);
    outputDisplay.setColour(juce::TextEditor::backgroundColourId, juce::Colours::black);
    outputDisplay.setColour(juce::TextEditor::textColourId, terminalAmber);
    outputDisplay.setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    outputDisplay.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 12.0f, juce::Font::plain));
    addAndMakeVisible(outputDisplay);
    
    // Configure command input
    commandInput.setMultiLine(false);
    commandInput.setColour(juce::TextEditor::backgroundColourId, juce::Colours::black);
    commandInput.setColour(juce::TextEditor::textColourId, terminalGreen);
    commandInput.setColour(juce::TextEditor::outlineColourId, terminalGreen.withAlpha(0.3f));
    commandInput.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 14.0f, juce::Font::plain));
    commandInput.setTextToShowWhenEmpty("Enter command...", terminalGreen.withAlpha(0.3f));
    commandInput.addListener(this);
    addAndMakeVisible(commandInput);
    
    // Prompt label
    promptLabel.setText("> ", juce::dontSendNotification);
    promptLabel.setColour(juce::Label::textColourId, terminalGreen);
    promptLabel.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 14.0f, juce::Font::bold));
    addAndMakeVisible(promptLabel);
    
    // Status bar
    statusBar.setText("READY", juce::dontSendNotification);
    statusBar.setColour(juce::Label::textColourId, terminalAmber);
    statusBar.setColour(juce::Label::backgroundColourId, juce::Colour(0xff111111));
    statusBar.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 11.0f, juce::Font::plain));
    statusBar.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(statusBar);
    
    // Execute button
    executeButton.setColour(juce::TextButton::buttonColourId, terminalGreen.withAlpha(0.2f));
    executeButton.setColour(juce::TextButton::textColourOffId, terminalGreen);
    executeButton.onClick = [this] { executeCommand(); };
    addAndMakeVisible(executeButton);
    
    // Clear button
    clearButton.setColour(juce::TextButton::buttonColourId, terminalAmber.withAlpha(0.2f));
    clearButton.setColour(juce::TextButton::textColourOffId, terminalAmber);
    clearButton.onClick = [this] { clearOutput(); };
    addAndMakeVisible(clearButton);
    
    // History button
    historyButton.setColour(juce::TextButton::buttonColourId, juce::Colours::grey.withAlpha(0.2f));
    historyButton.setColour(juce::TextButton::textColourOffId, juce::Colours::grey);
    historyButton.onClick = [this] { 
        // Show history popup menu
        juce::PopupMenu historyMenu;
        for (int i = commandHistory.size() - 1; i >= 0; --i)
        {
            historyMenu.addItem(i + 1, commandHistory[i]);
        }
        
        historyMenu.showMenuAsync(juce::PopupMenu::Options(),
            [this](int result) {
                if (result > 0)
                {
                    commandInput.setText(commandHistory[result - 1]);
                }
            });
    };
    addAndMakeVisible(historyButton);
    
    // Scanline effect overlay
    addAndMakeVisible(scanlines);
    
    // Add initial boot message
    addOutput("=================================");
    addOutput("CHIMERA COMMAND TERMINAL v3.0");
    addOutput("Military Grade Audio Processing");
    addOutput("=================================");
    addOutput("");
    addOutput("Type 'help' for available commands");
    addOutput("");
    
    // Start animation timer
    startTimerHz(15);
}

CommandTerminal::~CommandTerminal()
{
    stopTimer();
}

void CommandTerminal::paint(juce::Graphics& g)
{
    drawTerminalFrame(g);
    
    // CRT phosphor glow effect
    if (isTyping)
    {
        g.setColour(terminalGreen.withAlpha(0.05f));
        g.fillAll();
    }
}

void CommandTerminal::resized()
{
    auto bounds = getLocalBounds();
    
    // Terminal frame inset
    bounds.reduce(10, 10);
    
    // Status bar at top
    statusBar.setBounds(bounds.removeFromTop(25));
    bounds.removeFromTop(5);
    
    // Control buttons at bottom
    auto buttonRow = bounds.removeFromBottom(30);
    buttonRow.removeFromLeft(20); // Prompt space
    
    executeButton.setBounds(buttonRow.removeFromLeft(80));
    buttonRow.removeFromLeft(5);
    clearButton.setBounds(buttonRow.removeFromLeft(60));
    buttonRow.removeFromLeft(5);
    historyButton.setBounds(buttonRow.removeFromLeft(70));
    
    // Command input at bottom
    bounds.removeFromBottom(5);
    auto inputRow = bounds.removeFromBottom(25);
    promptLabel.setBounds(inputRow.removeFromLeft(20));
    commandInput.setBounds(inputRow);
    
    bounds.removeFromBottom(5);
    
    // Output display fills remaining space
    outputDisplay.setBounds(bounds);
    
    // Scanline effect overlay
    scanlines.setBounds(getLocalBounds());
}

void CommandTerminal::setPrompt(const juce::String& prompt)
{
    promptLabel.setText(prompt + " ", juce::dontSendNotification);
}

void CommandTerminal::addOutput(const juce::String& text, bool isError)
{
    auto currentText = outputDisplay.getText();
    
    // Add timestamp if it's a new command
    if (text.startsWith(">"))
    {
        auto time = juce::Time::getCurrentTime();
        auto timestamp = time.toString(false, true).substring(0, 8);
        currentText += "[" + timestamp + "] ";
    }
    
    currentText += text + "\n";
    outputDisplay.setText(currentText);
    
    // Auto-scroll to bottom
    outputDisplay.moveCaretToEnd();
    
    // Flash effect for errors
    if (isError)
    {
        outputDisplay.setColour(juce::TextEditor::textColourId, terminalRed);
        juce::Timer::callAfterDelay(100, [this] {
            outputDisplay.setColour(juce::TextEditor::textColourId, terminalAmber);
        });
    }
}

void CommandTerminal::clearOutput()
{
    outputDisplay.clear();
    addOutput("Terminal cleared.");
}

void CommandTerminal::setStatus(const juce::String& status, bool isWarning)
{
    statusBar.setText(status, juce::dontSendNotification);
    statusBar.setColour(juce::Label::textColourId, 
                        isWarning ? terminalRed : terminalAmber);
}

void CommandTerminal::timerCallback()
{
    // Cursor blink animation
    static int blinkCounter = 0;
    if (++blinkCounter > 15)
    {
        blinkCounter = 0;
        cursorVisible = !cursorVisible;
        
        if (commandInput.hasKeyboardFocus(true))
        {
            commandInput.setCaretVisible(cursorVisible);
        }
    }
    
    // Scanline animation
    scanlinePosition += 0.02f;
    if (scanlinePosition > 1.0f)
        scanlinePosition = 0.0f;
    scanlines.setScanlinePosition(scanlinePosition);
    
    // Typing indicator animation
    if (isTyping)
    {
        static int dotCounter = 0;
        if (++dotCounter > 10)
        {
            dotCounter = 0;
            typingDots = (typingDots + 1) % 4;
            
            juce::String dots;
            for (int i = 0; i < typingDots; ++i)
                dots += ".";
            
            setStatus("PROCESSING" + dots, false);
        }
    }
}

void CommandTerminal::textEditorTextChanged(juce::TextEditor&)
{
    // Could add real-time command validation here
}

void CommandTerminal::textEditorReturnKeyPressed(juce::TextEditor&)
{
    executeCommand();
}

void CommandTerminal::textEditorEscapeKeyPressed(juce::TextEditor&)
{
    commandInput.clear();
}

void CommandTerminal::executeCommand()
{
    auto command = commandInput.getText();
    if (command.isEmpty()) return;
    
    // Add to history
    commandHistory.push_back(command);
    if (commandHistory.size() > 50) // Limit history size
        commandHistory.erase(commandHistory.begin());
    
    // Display command in output
    addOutput("> " + command);
    
    // Clear input
    commandInput.clear();
    
    // Process built-in commands
    if (command.toLowerCase() == "help")
    {
        addOutput("Available commands:");
        addOutput("  generate <prompt> - Generate AI preset");
        addOutput("  clear            - Clear terminal");
        addOutput("  status           - Show system status");
        addOutput("  version          - Show version info");
    }
    else if (command.toLowerCase() == "version")
    {
        addOutput("Chimera Phoenix v3.0.0");
        addOutput("Build: " + juce::String(__DATE__));
    }
    else if (command.toLowerCase() == "status")
    {
        addOutput("System Status: OPERATIONAL");
        addOutput("AI Server: CONNECTED");
        addOutput("Audio Engine: ACTIVE");
    }
    else if (command.toLowerCase().startsWith("generate "))
    {
        // Extract prompt and trigger callback
        auto prompt = command.substring(9);
        if (onCommandExecute)
        {
            isTyping = true;
            onCommandExecute(prompt);
        }
    }
    else
    {
        addOutput("Unknown command. Type 'help' for available commands.", true);
    }
}

void CommandTerminal::navigateHistory(bool up)
{
    if (commandHistory.empty()) return;
    
    if (up)
    {
        if (historyIndex < 0)
            historyIndex = commandHistory.size() - 1;
        else if (historyIndex > 0)
            historyIndex--;
    }
    else
    {
        if (historyIndex >= 0 && historyIndex < commandHistory.size() - 1)
            historyIndex++;
        else
            historyIndex = -1;
    }
    
    if (historyIndex >= 0 && historyIndex < commandHistory.size())
        commandInput.setText(commandHistory[historyIndex]);
    else
        commandInput.clear();
}

void CommandTerminal::drawTerminalFrame(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Outer bezel
    g.setColour(juce::Colour(0xff2a2a2a));
    g.fillRoundedRectangle(bounds, 5.0f);
    
    // Inner screen area
    bounds.reduce(5, 5);
    g.setColour(juce::Colour(0xff0a0a0a));
    g.fillRoundedRectangle(bounds, 3.0f);
    
    // CRT glass reflection effect
    juce::ColourGradient glassGradient(
        juce::Colours::white.withAlpha(0.05f), 
        bounds.getX(), bounds.getY(),
        juce::Colours::transparentBlack,
        bounds.getX(), bounds.getCentreY(),
        false
    );
    g.setGradientFill(glassGradient);
    g.fillRoundedRectangle(bounds, 3.0f);
    
    // Corner screws
    if (auto* lnf = dynamic_cast<SkunkworksLookAndFeel*>(&getLookAndFeel()))
    {
        lnf->drawScrew(g, 5, 5, 6);
        lnf->drawScrew(g, getWidth() - 11, 5, 6);
        lnf->drawScrew(g, 5, getHeight() - 11, 6);
        lnf->drawScrew(g, getWidth() - 11, getHeight() - 11, 6);
    }
}

//==============================================================================
// ScanlineEffect Implementation
//==============================================================================

void CommandTerminal::ScanlineEffect::paint(juce::Graphics& g)
{
    // Horizontal scanlines
    g.setColour(juce::Colours::black.withAlpha(0.1f));
    for (int y = 0; y < getHeight(); y += 3)
    {
        g.drawHorizontalLine(y, 0, getWidth());
    }
    
    // Moving scanline
    float scanY = getHeight() * scanlinePos;
    juce::ColourGradient scanGradient(
        juce::Colours::white.withAlpha(0.02f),
        0, scanY - 20,
        juce::Colours::white.withAlpha(0.0f),
        0, scanY + 20,
        false
    );
    g.setGradientFill(scanGradient);
    g.fillRect(juce::Rectangle<int>(0, static_cast<int>(scanY - 20), getWidth(), 40));
}