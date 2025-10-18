#!/usr/bin/env python3
"""
Intelligent Preset Naming System
Generates varied, contextually appropriate preset names
"""

import random
import re
import json
from typing import Dict, List, Any, Optional
from collections import deque
import hashlib

class IntelligentPresetNamer:
    def __init__(self):
        # Track recent names to avoid duplicates
        self.recent_names = deque(maxlen=100)
        self.pattern_history = deque(maxlen=20)

        # Load vocabulary
        self.vocabulary = self._init_vocabulary()

    def _init_vocabulary(self):
        """Initialize naming vocabulary"""
        return {
            "adjectives": {
                "warm": ["Velvet", "Golden", "Amber", "Honey", "Sunset", "Autumn"],
                "cold": ["Crystal", "Arctic", "Frozen", "Silver", "Winter", "Glacial"],
                "aggressive": ["Savage", "Brutal", "Fierce", "Raw", "Violent", "Crushing"],
                "smooth": ["Silk", "Liquid", "Flowing", "Gentle", "Soft", "Creamy"],
                "vintage": ["Retro", "Classic", "Antique", "Aged", "Worn", "Dusty"],
                "modern": ["Digital", "Quantum", "Cyber", "Neo", "Ultra", "Hyper"],
                "ethereal": ["Celestial", "Astral", "Cosmic", "Dreamy", "Floating", "Mystic"]
            },
            "nouns": {
                "reverb": ["Chamber", "Cathedral", "Space", "Echo", "Hall", "Room"],
                "delay": ["Echo", "Repeat", "Reflection", "Ghost", "Memory", "Mirror"],
                "distortion": ["Fire", "Fury", "Rage", "Beast", "Monster", "Demon"],
                "modulation": ["Wave", "Tide", "Pulse", "Vortex", "Spiral", "Cycle"],
                "filter": ["Gate", "Portal", "Lens", "Prism", "Window", "Screen"],
                "general": ["Machine", "Engine", "Device", "System", "Unit", "Module"]
            },
            "verbs": ["Breaking", "Rising", "Falling", "Burning", "Freezing", "Flowing"],
            "places": ["Tokyo", "Berlin", "Nashville", "Detroit", "London", "Mars"],
            "years": ["1969", "1977", "1984", "1991", "2001", "2049"],
            "single_words": ["Monolith", "Zenith", "Apex", "Core", "Prime", "Alpha"]
        }

    def generate_name(self, prompt: str, engines: List[Dict], context: Dict) -> str:
        """Main entry point - generates contextually appropriate name"""

        # Analyze prompt to determine naming strategy
        strategy = self._select_strategy(prompt, engines, context)

        # Generate name using selected strategy
        name = strategy(prompt, engines, context)

        # Ensure it's not a recent duplicate
        name = self._ensure_unique(name)

        # Track the name
        self.recent_names.append(name.lower())

        return name

    def _select_strategy(self, prompt: str, engines: List[Dict], context: Dict):
        """Select naming strategy based on prompt analysis"""
        prompt_lower = prompt.lower()

        # Check for specific patterns
        if any(word in prompt_lower for word in ["test", "default", "basic"]):
            return self._technical_name

        if any(word in prompt_lower for word in ["warm", "vintage", "tape", "analog"]):
            return self._vintage_name

        if any(word in prompt_lower for word in ["aggressive", "heavy", "metal", "distortion"]):
            return self._aggressive_name

        if any(word in prompt_lower for word in ["ambient", "pad", "atmosphere", "space"]):
            return self._ethereal_name

        if re.search(r'\d+ms|\d+hz', prompt_lower):
            return self._technical_name

        # Check primary engine type
        if engines and len(engines) > 0:
            primary_engine = engines[0].get("engine_name", "").lower()
            if "reverb" in primary_engine:
                return self._reverb_name
            elif "delay" in primary_engine or "echo" in primary_engine:
                return self._delay_name
            elif "distortion" in primary_engine or "fuzz" in primary_engine:
                return self._distortion_name

        # Default to creative wildcard
        return self._creative_wildcard

    def _technical_name(self, prompt: str, engines: List[Dict], context: Dict) -> str:
        """Generate technical/descriptive names"""
        templates = []

        if engines:
            primary = engines[0].get("engine_name", "Effect")
            # Extract key part of engine name
            key_word = primary.split()[0] if primary else "Custom"

            templates = [
                f"{key_word} {random.choice(['Pro', 'Plus', 'Studio', 'Master'])}",
                f"{key_word} {random.randint(100, 999)}",
                f"Dual {key_word}",
                f"{key_word} Mk{random.choice(['II', 'III', 'IV', 'V'])}",
            ]

        templates.extend([
            f"Preset {random.randint(1, 999)}",
            f"Channel {random.randint(1, 16)}",
            f"Program {chr(random.randint(65, 90))}{random.randint(1, 9)}"
        ])

        return random.choice(templates)

    def _vintage_name(self, prompt: str, engines: List[Dict], context: Dict) -> str:
        """Generate vintage/retro themed names"""
        year = random.choice(self.vocabulary["years"])
        adj = random.choice(self.vocabulary["adjectives"]["vintage"])
        noun = random.choice(["Tape", "Vinyl", "Tube", "Spring", "Valve", "Echo"])

        templates = [
            f"{year}",
            f"{adj} {noun}",
            f"Studio {random.choice(['A', 'B', 'C'])}",
            f"Analog {random.choice(['Dreams', 'Nights', 'Days'])}",
            f"Vintage {random.choice(['Gold', 'Silver', 'Bronze'])}",
            f"The {year}s",
            f"Old School {noun}"
        ]

        return random.choice(templates)

    def _aggressive_name(self, prompt: str, engines: List[Dict], context: Dict) -> str:
        """Generate aggressive/heavy names"""
        adj = random.choice(self.vocabulary["adjectives"]["aggressive"])
        noun = random.choice(self.vocabulary["nouns"]["distortion"])

        templates = [
            f"{adj} {noun}",
            f"{noun} {random.choice(['Crusher', 'Destroyer', 'Annihilator'])}",
            f"Maximum {random.choice(['Damage', 'Force', 'Power'])}",
            f"{random.choice(['Death', 'Doom', 'Chaos'])} {random.choice(['Machine', 'Engine', 'Generator'])}",
            f"The {adj}",
            f"{random.choice(['Red', 'Black', 'Dark'])} {noun}"
        ]

        return random.choice(templates)

    def _ethereal_name(self, prompt: str, engines: List[Dict], context: Dict) -> str:
        """Generate ambient/atmospheric names"""
        adj = random.choice(self.vocabulary["adjectives"]["ethereal"])

        templates = [
            f"{adj} {random.choice(['Dreams', 'Visions', 'Waves'])}",
            f"{random.choice(['Star', 'Moon', 'Sun'])} {random.choice(['Field', 'Gate', 'Bridge'])}",
            f"Deep {random.choice(['Space', 'Ocean', 'Forest'])}",
            f"{random.choice(['Aurora', 'Nebula', 'Galaxy'])} {random.choice(['One', 'Prime', 'Core'])}",
            f"Floating {random.choice(['Island', 'City', 'Garden'])}",
            f"{adj}"
        ]

        return random.choice(templates)

    def _reverb_name(self, prompt: str, engines: List[Dict], context: Dict) -> str:
        """Reverb-specific names"""
        size = random.choice(["Small", "Medium", "Large", "Huge", "Infinite"])
        space = random.choice(self.vocabulary["nouns"]["reverb"])

        templates = [
            f"{size} {space}",
            f"{random.choice(self.vocabulary['adjectives']['ethereal'])} {space}",
            f"The {space}",
            f"{space} {random.choice(['Ambience', 'Atmosphere', 'Aura'])}",
            f"Reverb {random.randint(1, 9)}"
        ]

        return random.choice(templates)

    def _delay_name(self, prompt: str, engines: List[Dict], context: Dict) -> str:
        """Delay-specific names"""
        templates = [
            f"{random.choice(['Tape', 'Digital', 'Analog'])} {random.choice(self.vocabulary['nouns']['delay'])}",
            f"{random.choice(['Single', 'Dual', 'Multi'])} Tap",
            f"Echo {random.choice(['Chamber', 'Unit', 'Box'])}",
            f"{random.randint(100, 999)}ms {random.choice(['Delay', 'Echo'])}",
            f"The {random.choice(['Repeater', 'Echobox', 'Reflector'])}"
        ]

        return random.choice(templates)

    def _distortion_name(self, prompt: str, engines: List[Dict], context: Dict) -> str:
        """Distortion-specific names"""
        templates = [
            f"{random.choice(['Fuzz', 'Drive', 'Crush'])} {random.choice(['Box', 'Pedal', 'Unit'])}",
            f"{random.choice(self.vocabulary['adjectives']['aggressive'])} {random.choice(['Tone', 'Sound', 'Voice'])}",
            f"Gain {random.choice(['Monster', 'Machine', 'Stage'])}",
            f"{random.choice(['Hot', 'Burning', 'Melting'])} {random.choice(['Tubes', 'Circuits', 'Wires'])}",
            f"Distortion {random.randint(1, 11)}"
        ]

        return random.choice(templates)

    def _creative_wildcard(self, prompt: str, engines: List[Dict], context: Dict) -> str:
        """Completely creative/random approaches"""

        strategies = [
            # Single word
            lambda: random.choice(self.vocabulary["single_words"]),

            # Contradiction
            lambda: f"{random.choice(['Silent', 'Quiet', 'Soft'])} {random.choice(['Storm', 'Thunder', 'Explosion'])}",

            # Place + Thing
            lambda: f"{random.choice(self.vocabulary['places'])} {random.choice(['Nights', 'Dreams', 'Underground'])}",

            # Number/Code
            lambda: f"Unit {random.randint(1, 999)}",
            lambda: f"Model {chr(random.randint(65, 90))}-{random.randint(100, 999)}",

            # Verb + Noun
            lambda: f"{random.choice(self.vocabulary['verbs'])} {random.choice(['Glass', 'Steel', 'Ice', 'Fire'])}",

            # Color + Abstract
            lambda: f"{random.choice(['Red', 'Blue', 'Black', 'White'])} {random.choice(['Matter', 'Energy', 'Force', 'Wave'])}",

            # Pop culture style (generic to avoid copyright)
            lambda: f"The {random.choice(['Matrix', 'Odyssey', 'Journey', 'Quest'])}",

            # Compound
            lambda: f"{random.choice(['Neo', 'Meta', 'Ultra'])}-{random.choice(['Verb', 'Drive', 'Wave'])}",

            # Question/Phrase
            lambda: random.choice(["What If", "Never Again", "First Light", "Last Call", "No Return"])
        ]

        return random.choice(strategies)()

    def _ensure_unique(self, name: str) -> str:
        """Ensure name hasn't been used recently"""
        name_lower = name.lower()

        # Check if it's a duplicate
        if name_lower in self.recent_names:
            # Try adding a variation
            variations = [
                f"{name} II",
                f"{name} Plus",
                f"{name} Pro",
                f"{name} {random.choice(['Remix', 'Redux', 'Reborn'])}",
                f"{name} {chr(random.randint(65, 90))}",  # Add letter
                f"{name} {random.randint(2, 99)}"  # Add number
            ]

            for variant in variations:
                if variant.lower() not in self.recent_names:
                    return variant

            # If all variations used, add unique hash
            return f"{name} {hashlib.md5(str(random.random()).encode()).hexdigest()[:4].upper()}"

        return name


# Standalone test function
if __name__ == "__main__":
    namer = IntelligentPresetNamer()

    test_prompts = [
        "warm vintage tape delay",
        "aggressive metal distortion",
        "smooth jazz reverb",
        "test",
        "ambient space pad",
        "300ms delay",
        "bit crusher",
        "ethereal shimmer",
    ]

    print("Testing Intelligent Preset Namer:\n")
    for prompt in test_prompts:
        # Simulate some engines
        engines = [{"engine_name": prompt.split()[0].title() + " Effect"}]
        context = {"intensity": "moderate"}

        name = namer.generate_name(prompt, engines, context)
        print(f"'{prompt}' → '{name}'")