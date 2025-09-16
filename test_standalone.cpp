// Actual standalone test for the plugin UI
#include "JUCE_Plugin/Source/PluginProcessor.h"
#include "JUCE_Plugin/Source/PluginEditorNexusStatic.h"
#include <iostream>

class TestApp : public juce::JUCEApplication
{
public:
    TestApp() {}

    const juce::String getApplicationName() override { return "Plugin Test"; }
    const juce::String getApplicationVersion() override { return "1.0"; }
    bool moreThanOneInstanceAllowed() override { return true; }

    void initialise(const juce::String&) override
    {
        std::cout << "Creating ChimeraAudioProcessor..." << std::endl;
        processor = std::make_unique<ChimeraAudioProcessor>();
        
        std::cout << "Preparing processor..." << std::endl;
        processor->prepareToPlay(44100, 512);
        
        std::cout << "Creating editor..." << std::endl;
        try {
            auto* editor = processor->createEditor();
            if (!editor) {
                std::cerr << "ERROR: createEditor returned nullptr!" << std::endl;
                quit();
                return;
            }
            
            std::cout << "Editor created successfully!" << std::endl;
            
            // Create window to hold the editor
            mainWindow = std::make_unique<juce::DocumentWindow>("Test", juce::Colours::black, 
                                                                juce::DocumentWindow::allButtons);
            mainWindow->setContentOwned(editor, true);
            mainWindow->setResizable(false, false);
            mainWindow->centreWithSize(editor->getWidth(), editor->getHeight());
            mainWindow->setVisible(true);
            
            std::cout << "Window created and shown!" << std::endl;
        }
        catch (const std::exception& e) {
            std::cerr << "CRASH: " << e.what() << std::endl;
            quit();
        }
        catch (...) {
            std::cerr << "CRASH: Unknown exception!" << std::endl;
            quit();
        }
    }

    void shutdown() override
    {
        mainWindow = nullptr;
        processor = nullptr;
    }

private:
    std::unique_ptr<ChimeraAudioProcessor> processor;
    std::unique_ptr<juce::DocumentWindow> mainWindow;
};

START_JUCE_APPLICATION(TestApp)