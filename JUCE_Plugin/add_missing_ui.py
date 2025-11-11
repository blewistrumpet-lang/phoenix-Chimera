#!/usr/bin/env python3
import xml.etree.ElementTree as ET

def add_missing_ui_files(input_file, output_file):
    # Parse the XML
    tree = ET.parse(input_file)
    root = tree.getroot()

    # Find the first GROUP element that contains UI files
    ui_group = None
    for group in root.iter('GROUP'):
        # Check if this group has UI files
        for file_elem in group.findall('FILE'):
            if 'PluginEditor' in file_elem.get('name', ''):
                ui_group = group
                break
        if ui_group is not None:
            break

    if ui_group is None:
        # Find the mainGroup
        for group in root.iter('GROUP'):
            if group.get('id') == '{E4C45340-7B10-17C2-E3D9-9A42DF0C7CC1}':
                ui_group = group
                break

    if ui_group is not None:
        # Check if PluginEditor_Original is already there
        has_original = False
        has_pi = False

        for file_elem in ui_group.findall('FILE'):
            name = file_elem.get('name', '')
            if name == 'PluginEditor_Original.cpp' or name == 'PluginEditor_Original.h':
                has_original = True
            if name == 'PluginEditor_Pi.cpp' or name == 'PluginEditor_Pi.h':
                has_pi = True

        # Add missing files
        if not has_original:
            # Add PluginEditor_Original.cpp
            elem = ET.SubElement(ui_group, 'FILE')
            elem.set('id', 'Original001')
            elem.set('name', 'PluginEditor_Original.cpp')
            elem.set('compile', '1')
            elem.set('resource', '0')
            elem.set('file', 'Source/PluginEditor_Original.cpp')

            # Add PluginEditor_Original.h
            elem = ET.SubElement(ui_group, 'FILE')
            elem.set('id', 'Original002')
            elem.set('name', 'PluginEditor_Original.h')
            elem.set('compile', '0')
            elem.set('resource', '0')
            elem.set('file', 'Source/PluginEditor_Original.h')

            print("Added PluginEditor_Original files")

        if not has_pi:
            # Add PluginEditor_Pi.cpp
            elem = ET.SubElement(ui_group, 'FILE')
            elem.set('id', 'Pi001')
            elem.set('name', 'PluginEditor_Pi.cpp')
            elem.set('compile', '1')
            elem.set('resource', '0')
            elem.set('file', 'Source/PluginEditor_Pi.cpp')

            # Add PluginEditor_Pi.h
            elem = ET.SubElement(ui_group, 'FILE')
            elem.set('id', 'Pi002')
            elem.set('name', 'PluginEditor_Pi.h')
            elem.set('compile', '0')
            elem.set('resource', '0')
            elem.set('file', 'Source/PluginEditor_Pi.h')

            print("Added PluginEditor_Pi files")

    # Write the updated XML
    tree.write(output_file, encoding='utf-8', xml_declaration=True)
    print(f"Updated file written to: {output_file}")

if __name__ == "__main__":
    input_file = "/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/ChimeraPhoenix.jucer.cleaned"
    output_file = "/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/ChimeraPhoenix.jucer"

    add_missing_ui_files(input_file, output_file)