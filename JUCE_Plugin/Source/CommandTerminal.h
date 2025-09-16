#pragma once

#include <JuceHeader.h>
#include "SkunkworksLookAndFeel.h"

class CommandTerminal : public juce::Component,
                       private juce::Timer,
                       private juce::TextEditor::Listener
{
public:
    CommandTerminal();
    ~CommandTerminal() override;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // Terminal interface
    void setPrompt(const juce::String& prompt);
    juce::String getCommand() const { return commandInput.getText(); }
    void clearCommand() { commandInput.clear(); }
    
    void addOutput(const juce::String& text, bool isError = false);
    void clearOutput();
    
    void setStatus(const juce::String& status, bool isWarning = false);
    void showTypingAnimation(bool show) { isTyping = show; }
    
    // Command execution callback
    std::function<void(const juce::String&)> onCommandExecute;
    
private:
    // Terminal display
    juce::TextEditor outputDisplay;
    juce::TextEditor commandInput;
    juce::Label promptLabel;
    juce::Label statusBar;
    
    // Control buttons
    juce::TextButton executeButton{"EXECUTE"};
    juce::TextButton clearButton{"CLEAR"};
    juce::TextButton historyButton{"HISTORY"};
    
    // Visual elements
    class ScanlineEffect : public juce::Component 
    {
    public:
        ScanlineEffect() { setInterceptsMouseClicks(false, false); }
        void paint(juce::Graphics& g) override;
        void setScanlinePosition(float pos) { scanlinePos = pos; repaint(); }
    private:
        float scanlinePos{0.0f};
    };
    
    ScanlineEffect scanlines;
    
    // Command history
    std::vector<juce::String> commandHistory;
    int historyIndex{-1};
    
    // Animation properties
    bool cursorVisible{true};
    bool isTyping{false};
    float scanlinePosition{0.0f};
    int typingDots{0};
    
    // Colors
    juce::Colour terminalGreen{0xff00ff44};
    juce::Colour terminalAmber{0xffffaa00};
    juce::Colour terminalRed{0xffff2222};
    
    // Timer for animations
    void timerCallback() override;
    
    // TextEditor listener
    void textEditorTextChanged(juce::TextEditor&) override;
    void textEditorReturnKeyPressed(juce::TextEditor&) override;
    void textEditorEscapeKeyPressed(juce::TextEditor&) override;
    
    // Helper methods
    void executeCommand();
    void navigateHistory(bool up);
    void drawTerminalFrame(juce::Graphics& g);
    void applyTerminalStyling();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CommandTerminal)
};