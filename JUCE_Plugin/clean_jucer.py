#!/usr/bin/env python3
import xml.etree.ElementTree as ET
import sys

# Files we want to keep
KEEP_FILES = {
    'PluginEditor_Original.cpp',
    'PluginEditor_Original.h',
    'PluginEditor_Pi.cpp',
    'PluginEditor_Pi.h',
    'PluginEditorFull.cpp',
    'PluginEditorFull.h',
    'PluginEditorNexusStatic.cpp',
    'PluginEditorNexusStatic.h'
}

def clean_jucer_file(input_file, output_file):
    # Parse the XML
    tree = ET.parse(input_file)
    root = tree.getroot()

    # Find all FILE elements
    for parent in root.iter():
        files_to_remove = []
        for child in parent:
            if child.tag == 'FILE':
                name = child.get('name', '')
                # Check if this is a PluginEditor file
                if 'PluginEditor' in name:
                    # Only keep if it's in our keep list
                    if name not in KEEP_FILES:
                        files_to_remove.append(child)
                        print(f"Removing: {name}")
                    else:
                        print(f"Keeping: {name}")

        # Remove the files we don't want
        for file_elem in files_to_remove:
            parent.remove(file_elem)

    # Write the cleaned XML
    tree.write(output_file, encoding='utf-8', xml_declaration=True)
    print(f"\nCleaned file written to: {output_file}")

if __name__ == "__main__":
    input_file = "/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/ChimeraPhoenix.jucer"
    output_file = "/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/ChimeraPhoenix.jucer.cleaned"

    clean_jucer_file(input_file, output_file)