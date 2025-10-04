#!/usr/bin/env python3
"""
Merge all engine knowledge into a SINGLE, COMPLETE source of truth
This will be THE definitive engine knowledge base for AI components
"""

import json
from engine_knowledge_base import ENGINE_KNOWLEDGE
from engine_mapping_authoritative import ENGINE_NAMES

def create_complete_engine_knowledge():
    """Create the definitive, complete engine knowledge base"""
    
    # Load existing trinity knowledge
    with open("trinity_engine_knowledge.json", "r") as f:
        trinity_knowledge = json.load(f)
    
    # Enhance each engine with complete descriptions
    for engine_id_str, engine_data in trinity_knowledge["engines"].items():
        engine_id = int(engine_id_str)
        
        # Get additional knowledge from engine_knowledge_base
        if engine_id in ENGINE_KNOWLEDGE:
            kb_data = ENGINE_KNOWLEDGE[engine_id]
            
            # Add detailed descriptions
            engine_data["function"] = kb_data.get("function", f"Engine {engine_id} processing")
            engine_data["character"] = kb_data.get("character", "Standard processing")
            engine_data["use_cases"] = kb_data.get("use_cases", [])
            engine_data["detailed_description"] = kb_data.get("function", "") + ". " + kb_data.get("character", "")
            
            # Add sound design notes
            if "warm" in kb_data.get("character", "").lower():
                engine_data["sound_design_notes"] = "Adds warmth and analog character"
            elif "aggressive" in kb_data.get("character", "").lower():
                engine_data["sound_design_notes"] = "Adds aggression and intensity"
            elif "clean" in kb_data.get("character", "").lower():
                engine_data["sound_design_notes"] = "Transparent processing without coloration"
            elif "vintage" in kb_data.get("character", "").lower():
                engine_data["sound_design_notes"] = "Vintage analog character and coloration"
            else:
                engine_data["sound_design_notes"] = "Specialized audio processing"
        
        # Ensure we have the name from authoritative source
        if engine_id in ENGINE_NAMES:
            engine_data["name"] = ENGINE_NAMES[engine_id]
            engine_data["id"] = engine_id  # Ensure ID is integer
    
    # Add engine selection guidance
    trinity_knowledge["engine_selection_rules"] = {
        "shimmer_reverb": {
            "keywords": ["shimmer", "ethereal", "angelic", "celestial"],
            "engine_id": 42,
            "engine_name": "Shimmer Reverb"
        },
        "spring_reverb": {
            "keywords": ["spring", "surf", "vintage reverb", "twangy"],
            "engine_id": 40,
            "engine_name": "Spring Reverb"
        },
        "plate_reverb": {
            "keywords": ["plate", "studio reverb", "classic reverb"],
            "engine_id": 39,
            "engine_name": "Plate Reverb"
        },
        "chorus": {
            "keywords": ["chorus", "ensemble", "thicken"],
            "engine_id": 23,
            "engine_name": "Digital Chorus"
        },
        "noise_gate": {
            "keywords": ["gate", "noise gate", "tight", "clean up"],
            "engine_id": 4,
            "engine_name": "Noise Gate"
        },
        "parallel_compression": {
            "keywords": ["parallel compression", "new york compression"],
            "engine_id": 2,
            "engine_name": "Classic Compressor",
            "special_setup": "Set mix to 40-50% for parallel processing"
        }
    }
    
    # Add strict instructions for AI
    trinity_knowledge["ai_instructions"] = {
        "critical": [
            "ONLY use engine IDs from this knowledge base",
            "NEVER invent engine names or IDs",
            "ALWAYS use minimum 4 engines per preset",
            "ALWAYS fulfill specific requests (if user asks for spring reverb, use Spring Reverb ID 40)",
            "NEVER describe engines that don't exist in the knowledge base"
        ],
        "engine_mapping": {
            "When user says 'shimmer'": "Use Engine 42 (Shimmer Reverb)",
            "When user says 'spring reverb'": "Use Engine 40 (Spring Reverb)",
            "When user says 'chorus'": "Use Engine 23 (Digital Chorus)",
            "When user says 'phaser'": "Use Engine 24 (Analog Phaser)",
            "When user says 'noise gate'": "Use Engine 4 (Noise Gate)",
            "When user says 'tape'": "Use Engine 34 (Tape Echo) or 15 (Vintage Tube Preamp)"
        }
    }
    
    # Save the complete knowledge base
    output_path = "trinity_engine_knowledge_COMPLETE.json"
    with open(output_path, "w") as f:
        json.dump(trinity_knowledge, f, indent=2)
    
    print(f"✅ Created complete engine knowledge base: {output_path}")
    print(f"   Total engines: {len(trinity_knowledge['engines'])}")
    print(f"   Selection rules: {len(trinity_knowledge['engine_selection_rules'])}")
    
    # Verify completeness
    missing_descriptions = []
    for eid, engine in trinity_knowledge["engines"].items():
        if "function" not in engine or "character" not in engine:
            missing_descriptions.append(f"{eid}: {engine.get('name', 'Unknown')}")
    
    if missing_descriptions:
        print(f"⚠️  Missing descriptions for: {', '.join(missing_descriptions[:5])}")
    else:
        print("✅ All engines have complete descriptions!")
    
    return output_path

if __name__ == "__main__":
    create_complete_engine_knowledge()