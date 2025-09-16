// Simple manual test - no auto-close timer
#include "JUCE_Plugin/Source/PluginProcessor.h"
#include "JUCE_Plugin/Source/TestEditorIncremental.h"
#include <iostream>

class TestApp : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override { return "Manual Test"; }
    const juce::String getApplicationVersion() override { return "1.0"; }
    bool moreThanOneInstanceAllowed() override { return true; }

    void initialise(const juce::String&) override
    {
        std::cout << "\n=== MANUAL UI TEST (no auto-close) ===" << std::endl;
        std::cout << "Close the window manually to exit..." << std::endl;
        
        try {
            std::cout << "1. Creating processor..." << std::endl;
            processor = std::make_unique<ChimeraAudioProcessor>();
            
            std::cout << "2. Creating editor..." << std::endl;
            auto* editor = new TestEditorIncremental(*processor);
            
            std::cout << "3. Creating window..." << std::endl;
            mainWindow = std::make_unique<MainWindow>(
                "Manual Test - Close Me", 
                juce::Colours::darkgrey,
                juce::DocumentWindow::allButtons
            );
            
            mainWindow->setContentOwned(editor, true);
            mainWindow->setResizable(false, false);
            mainWindow->centreWithSize(editor->getWidth(), editor->getHeight());
            mainWindow->setVisible(true);
            
            std::cout << "\n✓ Window created - is it responsive?" << std::endl;
            std::cout << "Try clicking buttons, resizing, closing..." << std::endl;
            
            // NO TIMER - wait for manual close
        }
        catch (const std::exception& e) {
            std::cerr << "✗ Failed: " << e.what() << std::endl;
            quit();
        }
    }

    void shutdown() override
    {
        std::cout << "Shutting down..." << std::endl;
        mainWindow = nullptr;
        processor = nullptr;
    }

    void systemRequestedQuit() override
    {
        std::cout << "User closed window - exiting..." << std::endl;
        quit();
    }

private:
    class MainWindow : public juce::DocumentWindow
    {
    public:
        MainWindow(const juce::String& name, juce::Colour colour, int buttons)
            : DocumentWindow(name, colour, buttons)
        {
            setUsingNativeTitleBar(true);
        }
        
        void closeButtonPressed() override
        {
            JUCEApplication::getInstance()->systemRequestedQuit();
        }
    };
    
    std::unique_ptr<ChimeraAudioProcessor> processor;
    std::unique_ptr<MainWindow> mainWindow;
};

START_JUCE_APPLICATION(TestApp)