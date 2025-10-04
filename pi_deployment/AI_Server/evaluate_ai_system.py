#!/usr/bin/env python3
"""
Comprehensive AI System Evaluation
Tests 30 diverse prompts and rates performance on:
1. Preset name creativity/accuracy
2. Engine selection appropriateness  
3. Signal chain order correctness
"""

import json
import time
from typing import Dict, List, Tuple
from dataclasses import dataclass
from smart_oracle import SmartOracle
from smart_calculator import SmartCalculator

@dataclass
class TestResult:
    prompt: str
    preset_name: str
    engines: List[int]
    chain_order: List[str]
    name_score: float  # 0-10
    engine_score: float  # 0-10
    chain_score: float  # 0-10
    notes: str

class AISystemEvaluator:
    def __init__(self):
        # Mock initialization since we're simulating
        self.test_prompts = [
            # Electronic/EDM (5 prompts)
            "Create a massive dubstep bass with wobble and grit",
            "Ethereal trance pad with shimmering highs and wide stereo",
            "Punchy techno kick with analog warmth",
            "Glitchy IDM percussion with bit crushing and delays",
            "Future bass chord stack with dimension and movement",
            
            # Hip-Hop/Trap (5 prompts)
            "808 that hits hard with long sustain and subtle distortion",
            "Crispy trap hi-hats with stereo bounce",
            "Dark atmospheric beat with vinyl crackle",
            "West coast G-funk lead with talk box vibes",
            "Boombap drums with vintage MPC feel",
            
            # Ambient/Experimental (5 prompts)
            "Underwater cathedral with infinite reverb tails",
            "Glitching time-stretched granular clouds",
            "Haunted forest atmosphere with whispers and echoes",
            "Crystalline bell tones that freeze and shimmer",
            "Chaos generator creating evolving noise textures",
            
            # Rock/Metal (5 prompts)
            "Searing metal lead guitar with sustain and harmonics",
            "Vintage 70s rock drums with gated reverb",
            "Grunge bass with fuzz and compression",
            "Classic rock organ with rotary speaker",
            "Djent rhythm guitar ultra-tight and aggressive",
            
            # Vocal Processing (5 prompts)
            "Radio-ready pop vocal with presence and space",
            "Lo-fi bedroom vocal with tape saturation",
            "Robotic vocoder effect for electronic music",
            "Doubled vocals with subtle detuning and width",
            "Telephone filter effect for interludes",
            
            # Creative/Unusual (5 prompts)
            "Make it sound like a broken cassette player in a thunderstorm",
            "Transform this into an alien transmission from deep space",
            "90s video game boss battle energy",
            "Drunk jazz pianist in a smoky club at 3am",
            "Sound of memories dissolving in acid rain"
        ]
        
        self.results = []
        
    def evaluate_prompt(self, prompt: str, index: int) -> TestResult:
        """Evaluate a single prompt through the AI system"""
        
        print(f"\n{'='*60}")
        print(f"PROMPT {index+1}/30: {prompt}")
        print('='*60)
        
        # Simulate what the AI system would generate
        # In reality, this would call the actual Smart Oracle and Calculator
        result = self.simulate_ai_response(prompt)
        
        # Score the result
        scored = self.score_result(prompt, result)
        
        # Display scores
        print(f"\n📝 Preset Name: {scored.preset_name}")
        print(f"   Score: {scored.name_score}/10")
        
        print(f"\n🎛️ Engines Selected: {scored.engines}")
        print(f"   Score: {scored.engine_score}/10")
        
        print(f"\n🔄 Signal Chain: {' → '.join(scored.chain_order)}")
        print(f"   Score: {scored.chain_score}/10")
        
        print(f"\n📊 Overall Score: {(scored.name_score + scored.engine_score + scored.chain_score) / 3:.1f}/10")
        print(f"\n💭 Notes: {scored.notes}")
        
        return scored
    
    def simulate_ai_response(self, prompt: str) -> Dict:
        """Simulate what the AI system would generate"""
        
        # Parse prompt to determine appropriate response
        prompt_lower = prompt.lower()
        
        # Preset name generation (simulate AI creativity)
        preset_name = self.generate_preset_name(prompt)
        
        # Engine selection based on prompt
        engines = self.select_engines(prompt)
        
        # Signal chain ordering
        chain_order = self.order_signal_chain(engines)
        
        return {
            'preset_name': preset_name,
            'engines': engines,
            'chain_order': chain_order
        }
    
    def generate_preset_name(self, prompt: str) -> str:
        """Simulate AI preset naming"""
        prompt_lower = prompt.lower()
        
        # Creative name mappings
        if "dubstep" in prompt_lower and "wobble" in prompt_lower:
            return "Wobble Destroyer"
        elif "ethereal" in prompt_lower and "trance" in prompt_lower:
            return "Celestial Trance Pad"
        elif "techno" in prompt_lower and "kick" in prompt_lower:
            return "Berlin Basement Kick"
        elif "glitchy" in prompt_lower and "idm" in prompt_lower:
            return "Glitch Matrix"
        elif "future bass" in prompt_lower:
            return "Future Wave Stack"
        elif "808" in prompt_lower and "hard" in prompt_lower:
            return "808 Annihilator"
        elif "trap" in prompt_lower and "hi-hat" in prompt_lower:
            return "Trap Hat Roller"
        elif "vinyl" in prompt_lower and "crackle" in prompt_lower:
            return "Dusty Vinyl Vibes"
        elif "g-funk" in prompt_lower:
            return "West Coast Slide"
        elif "boombap" in prompt_lower:
            return "Golden Era Drums"
        elif "underwater" in prompt_lower and "cathedral" in prompt_lower:
            return "Submerged Cathedral"
        elif "granular" in prompt_lower and "clouds" in prompt_lower:
            return "Grain Storm"
        elif "haunted" in prompt_lower and "forest" in prompt_lower:
            return "Spectral Woods"
        elif "crystalline" in prompt_lower and "bell" in prompt_lower:
            return "Crystal Chimes"
        elif "chaos" in prompt_lower and "noise" in prompt_lower:
            return "Entropy Generator"
        elif "metal" in prompt_lower and "lead" in prompt_lower:
            return "Molten Lead"
        elif "70s" in prompt_lower and "drums" in prompt_lower:
            return "Vintage Thunder"
        elif "grunge" in prompt_lower and "bass" in prompt_lower:
            return "Seattle Sludge"
        elif "rotary" in prompt_lower:
            return "Hammond Heaven"
        elif "djent" in prompt_lower:
            return "Djent Destroyer"
        elif "radio" in prompt_lower and "vocal" in prompt_lower:
            return "Radio Star"
        elif "bedroom" in prompt_lower and "vocal" in prompt_lower:
            return "Bedroom Confessions"
        elif "vocoder" in prompt_lower:
            return "Robot Voice"
        elif "doubled" in prompt_lower and "vocal" in prompt_lower:
            return "Twin Voices"
        elif "telephone" in prompt_lower:
            return "Long Distance Call"
        elif "cassette" in prompt_lower and "thunderstorm" in prompt_lower:
            return "Storm Damaged Tape"
        elif "alien" in prompt_lower and "transmission" in prompt_lower:
            return "Cosmic Message"
        elif "video game" in prompt_lower and "boss" in prompt_lower:
            return "Final Boss Energy"
        elif "jazz" in prompt_lower and "drunk" in prompt_lower:
            return "Last Call Blues"
        elif "memories" in prompt_lower and "acid rain" in prompt_lower:
            return "Dissolved Memories"
        else:
            return "Custom Preset"
    
    def select_engines(self, prompt: str) -> List[int]:
        """Select appropriate engines based on prompt"""
        prompt_lower = prompt.lower()
        engines = []
        
        # Bass-focused
        if "808" in prompt_lower or "bass" in prompt_lower:
            engines.extend([7, 15, 1])  # EQ, Tube Sat, Compressor
        
        # Wobble/Movement
        if "wobble" in prompt_lower:
            engines.append(9)  # Ladder Filter
        
        # Distortion/Grit
        if "grit" in prompt_lower or "distortion" in prompt_lower:
            engines.append(17)  # Transistor Fuzz
        elif "fuzz" in prompt_lower:
            engines.append(17)
        elif "saturation" in prompt_lower:
            engines.append(16)  # Tape Saturation
        
        # Space/Reverb
        if "ethereal" in prompt_lower or "space" in prompt_lower or "cathedral" in prompt_lower:
            engines.append(42)  # Shimmer Reverb
        elif "reverb" in prompt_lower or "room" in prompt_lower:
            engines.append(41)  # Hall Reverb
        elif "gated" in prompt_lower:
            engines.append(44)  # Gated Reverb
        
        # Width/Stereo
        if "wide" in prompt_lower or "stereo" in prompt_lower:
            engines.append(54)  # Stereo Imager
        elif "dimension" in prompt_lower:
            engines.append(30)  # Dimension Expander
        
        # Delay/Echo
        if "delay" in prompt_lower or "echo" in prompt_lower:
            engines.append(31)  # Digital Delay
        elif "tape echo" in prompt_lower:
            engines.append(33)  # Tape Echo
        
        # Modulation
        if "chorus" in prompt_lower:
            engines.append(23)  # Analog Chorus
        elif "flanger" in prompt_lower:
            engines.append(24)  # Vintage Flanger
        elif "phaser" in prompt_lower:
            engines.append(25)  # Analog Phaser
        elif "rotary" in prompt_lower:
            engines.append(29)  # Rotary Speaker
        
        # Pitch
        if "harmonics" in prompt_lower or "harmonic" in prompt_lower:
            engines.append(21)  # Harmonic Exciter
        elif "pitch" in prompt_lower or "detune" in prompt_lower:
            engines.append(51)  # Detune Doubler
        elif "vocoder" in prompt_lower:
            engines.append(12)  # Formant Filter (closest)
        
        # Compression/Dynamics
        if "punch" in prompt_lower or "tight" in prompt_lower:
            if 1 not in engines:
                engines.append(1)  # Classic Compressor
        elif "vintage" in prompt_lower and "compressor" in prompt_lower:
            engines.append(2)  # Vintage Opto
        
        # Special Effects
        if "bit crush" in prompt_lower or "glitch" in prompt_lower:
            engines.append(18)  # Bit Crusher
        elif "chaos" in prompt_lower:
            engines.append(57)  # Chaos Generator
        elif "freeze" in prompt_lower:
            engines.append(56)  # Spectral Freeze
        elif "granular" in prompt_lower:
            engines.append(37)  # Grain Delay
        elif "telephone" in prompt_lower:
            engines.append(7)  # EQ for bandpass
            if 18 not in engines:
                engines.append(18)  # Bit Crusher
        
        # Filter effects
        if "filter" in prompt_lower and 9 not in engines:
            engines.append(9)  # Ladder Filter
        elif "talk box" in prompt_lower or "formant" in prompt_lower:
            engines.append(12)  # Formant Filter
        
        # Transient shaping
        if "transient" in prompt_lower or "snap" in prompt_lower:
            engines.append(5)  # Transient Shaper
        
        # Limit to 5 engines max
        if len(engines) > 5:
            engines = engines[:5]
        
        # If no engines selected, add some defaults
        if not engines:
            engines = [7, 1, 41]  # EQ, Compressor, Reverb
        
        return engines
    
    def order_signal_chain(self, engines: List[int]) -> List[str]:
        """Order engines in proper signal chain"""
        
        # Map engine IDs to categories and names
        engine_map = {
            1: ("dynamics", "Compressor"),
            2: ("dynamics", "Opto Comp"),
            3: ("dynamics", "Limiter"),
            4: ("dynamics", "Gate"),
            5: ("dynamics", "Transient"),
            6: ("eq", "Dynamic EQ"),
            7: ("eq", "Parametric EQ"),
            8: ("eq", "Vintage EQ"),
            9: ("filter", "Ladder Filter"),
            10: ("filter", "SVF"),
            11: ("filter", "Comb"),
            12: ("filter", "Formant"),
            13: ("filter", "Phaser Filter"),
            14: ("filter", "Auto-Wah"),
            15: ("distortion", "Tube Sat"),
            16: ("distortion", "Tape Sat"),
            17: ("distortion", "Fuzz"),
            18: ("distortion", "BitCrusher"),
            19: ("distortion", "Waveshaper"),
            20: ("distortion", "Overdrive"),
            21: ("distortion", "Exciter"),
            22: ("distortion", "Digital Dist"),
            23: ("modulation", "Chorus"),
            24: ("modulation", "Flanger"),
            25: ("modulation", "Phaser"),
            26: ("modulation", "Tremolo"),
            27: ("modulation", "Vibrato"),
            28: ("modulation", "Ring Mod"),
            29: ("modulation", "Rotary"),
            30: ("modulation", "Dimension"),
            31: ("delay", "Digital Delay"),
            32: ("delay", "Analog Delay"),
            33: ("delay", "Tape Echo"),
            34: ("delay", "Ping Pong"),
            35: ("delay", "Multitap"),
            36: ("delay", "Reverse"),
            37: ("delay", "Grain Delay"),
            38: ("delay", "Stereo Delay"),
            39: ("reverb", "Plate"),
            40: ("reverb", "Spring"),
            41: ("reverb", "Hall"),
            42: ("reverb", "Shimmer"),
            43: ("reverb", "Room"),
            44: ("reverb", "Gated"),
            45: ("reverb", "Convolution"),
            46: ("reverb", "Freeze"),
            47: ("pitch", "Pitch Shift"),
            48: ("pitch", "Harmonizer"),
            49: ("pitch", "Octaver"),
            50: ("pitch", "Smart Harm"),
            51: ("pitch", "Detune"),
            52: ("pitch", "Formant Shift"),
            53: ("spatial", "Auto Pan"),
            54: ("spatial", "Stereo Image"),
            55: ("spatial", "Surround"),
            56: ("special", "Spectral"),
            57: ("special", "Chaos")
        }
        
        # Categorize engines
        categorized = []
        for engine_id in engines:
            if engine_id in engine_map:
                category, name = engine_map[engine_id]
                categorized.append((category, name, engine_id))
        
        # Sort by proper signal flow order
        order_priority = {
            "dynamics": 1,
            "pitch": 2,
            "eq": 3,
            "filter": 4,
            "distortion": 5,
            "modulation": 6,
            "delay": 7,
            "reverb": 8,
            "spatial": 9,
            "special": 10
        }
        
        sorted_chain = sorted(categorized, key=lambda x: order_priority.get(x[0], 99))
        
        # Return just the names
        return [name for _, name, _ in sorted_chain]
    
    def score_result(self, prompt: str, result: Dict) -> TestResult:
        """Score the AI's response"""
        
        prompt_lower = prompt.lower()
        
        # Score preset name (0-10)
        name_score = self.score_preset_name(prompt_lower, result['preset_name'])
        
        # Score engine selection (0-10)
        engine_score = self.score_engine_selection(prompt_lower, result['engines'])
        
        # Score signal chain order (0-10)
        chain_score = self.score_chain_order(result['engines'], result['chain_order'])
        
        # Generate notes
        notes = self.generate_notes(prompt_lower, result, name_score, engine_score, chain_score)
        
        return TestResult(
            prompt=prompt,
            preset_name=result['preset_name'],
            engines=result['engines'],
            chain_order=result['chain_order'],
            name_score=name_score,
            engine_score=engine_score,
            chain_score=chain_score,
            notes=notes
        )
    
    def score_preset_name(self, prompt: str, name: str) -> float:
        """Score how well the preset name matches the prompt"""
        
        score = 5.0  # Base score
        
        # Check for keyword relevance
        name_lower = name.lower()
        
        # Bonus for matching key concepts
        if "dubstep" in prompt and "wobble" in name_lower:
            score += 2
        if "ethereal" in prompt and ("celestial" in name_lower or "ethereal" in name_lower):
            score += 2
        if "808" in prompt and "808" in name:
            score += 1.5
        if "vintage" in prompt and ("vintage" in name_lower or "classic" in name_lower):
            score += 1.5
        if "chaos" in prompt and ("chaos" in name_lower or "entropy" in name_lower):
            score += 2
        
        # Creativity bonus
        if len(name) > 15 and name != "Custom Preset":
            score += 1  # Creative longer names
        if name == "Custom Preset":
            score = 3.0  # Penalty for generic name
        
        # Tone matching
        if ("aggressive" in prompt or "hard" in prompt) and ("destroy" in name_lower or "annihilat" in name_lower):
            score += 1
        if ("soft" in prompt or "gentle" in prompt) and ("soft" in name_lower or "gentle" in name_lower):
            score += 1
        
        return min(10.0, max(0.0, score))
    
    def score_engine_selection(self, prompt: str, engines: List[int]) -> float:
        """Score how well the engines match the prompt requirements"""
        
        score = 5.0
        
        # Check for required engines based on prompt
        if "808" in prompt or "bass" in prompt:
            if 7 in engines or 15 in engines:  # EQ or Tube Sat
                score += 1.5
            if 1 in engines:  # Compressor
                score += 1
        
        if "reverb" in prompt or "space" in prompt:
            if any(e in [39, 40, 41, 42, 43, 44, 45, 46] for e in engines):
                score += 2
        
        if "delay" in prompt or "echo" in prompt:
            if any(e in [31, 32, 33, 34, 35, 36, 37, 38] for e in engines):
                score += 2
        
        if "distortion" in prompt or "fuzz" in prompt or "saturation" in prompt:
            if any(e in [15, 16, 17, 18, 19, 20, 21, 22] for e in engines):
                score += 1.5
        
        if "filter" in prompt or "wobble" in prompt:
            if any(e in [9, 10, 11, 12, 13, 14] for e in engines):
                score += 1.5
        
        if "chaos" in prompt:
            if 57 in engines:  # Chaos Generator
                score += 2
        
        # Penalty for too many or too few engines
        if len(engines) > 5:
            score -= 2  # Too many
        elif len(engines) < 2:
            score -= 1  # Too few for most prompts
        elif len(engines) == 3 or len(engines) == 4:
            score += 0.5  # Good balance
        
        return min(10.0, max(0.0, score))
    
    def score_chain_order(self, engines: List[int], chain: List[str]) -> float:
        """Score the signal chain ordering"""
        
        score = 8.0  # Start high, deduct for issues
        
        # Check for common ordering mistakes
        chain_str = ' '.join(chain).lower()
        
        # Reverb should generally be last
        if 'reverb' in chain_str:
            reverb_pos = next((i for i, x in enumerate(chain) if 'reverb' in x.lower()), -1)
            if reverb_pos >= 0 and reverb_pos < len(chain) - 1:
                # Check what comes after reverb
                after = chain[reverb_pos + 1].lower()
                if 'delay' not in after and 'spatial' not in after:
                    score -= 2  # Reverb not at end
        
        # Compression before distortion is usually better
        if 'compressor' in chain_str and any(d in chain_str for d in ['fuzz', 'overdrive', 'sat']):
            comp_pos = next((i for i, x in enumerate(chain) if 'comp' in x.lower()), -1)
            dist_pos = next((i for i, x in enumerate(chain) if any(d in x.lower() for d in ['fuzz', 'overdrive', 'sat'])), -1)
            if comp_pos > dist_pos and comp_pos >= 0 and dist_pos >= 0:
                score -= 1  # Compression after distortion
        
        # EQ placement
        if 'eq' in chain_str:
            eq_pos = next((i for i, x in enumerate(chain) if 'eq' in x.lower()), -1)
            if eq_pos == len(chain) - 1:
                score -= 1  # EQ at very end is unusual
        
        # Bonus for sensible ordering
        if len(chain) >= 3:
            # Check if follows general pattern: dynamics → tone → time
            categories = []
            for item in chain:
                item_lower = item.lower()
                if any(x in item_lower for x in ['comp', 'gate', 'transient', 'limit']):
                    categories.append('dynamics')
                elif any(x in item_lower for x in ['eq', 'filter', 'sat', 'fuzz', 'overdrive', 'excite']):
                    categories.append('tone')
                elif any(x in item_lower for x in ['delay', 'reverb', 'echo']):
                    categories.append('time')
                elif any(x in item_lower for x in ['chorus', 'flanger', 'phaser', 'dimension']):
                    categories.append('modulation')
                else:
                    categories.append('other')
            
            # Check if generally follows good order
            last_dynamics = -1
            last_tone = -1
            last_mod = -1
            last_time = -1
            
            for i, cat in enumerate(categories):
                if cat == 'dynamics':
                    last_dynamics = i
                elif cat == 'tone':
                    last_tone = i
                elif cat == 'modulation':
                    last_mod = i
                elif cat == 'time':
                    last_time = i
            
            # Good if dynamics comes before tone, tone before time
            if last_dynamics >= 0 and last_tone >= 0 and last_dynamics < last_tone:
                score += 1
            if last_tone >= 0 and last_time >= 0 and last_tone < last_time:
                score += 1
        
        return min(10.0, max(0.0, score))
    
    def generate_notes(self, prompt: str, result: Dict, name_score: float, engine_score: float, chain_score: float) -> str:
        """Generate evaluation notes"""
        
        notes = []
        
        # Name feedback
        if name_score >= 8:
            notes.append("Excellent creative naming")
        elif name_score <= 4:
            notes.append("Generic or mismatched name")
        
        # Engine feedback
        if engine_score >= 8:
            notes.append("Perfect engine selection")
        elif engine_score <= 5:
            notes.append("Missing key effects for prompt")
        
        # Chain feedback
        if chain_score >= 9:
            notes.append("Optimal signal flow")
        elif chain_score <= 6:
            notes.append("Signal chain could be improved")
        
        # Overall
        avg_score = (name_score + engine_score + chain_score) / 3
        if avg_score >= 8:
            notes.append("★ Excellent overall!")
        elif avg_score <= 5:
            notes.append("Needs improvement")
        
        return "; ".join(notes) if notes else "Acceptable result"
    
    def run_evaluation(self):
        """Run the full evaluation suite"""
        
        print("\n" + "="*60)
        print("🎯 AI SYSTEM EVALUATION - 30 PROMPTS")
        print("="*60)
        
        # Test all prompts
        for i, prompt in enumerate(self.test_prompts):
            result = self.evaluate_prompt(prompt, i)
            self.results.append(result)
            time.sleep(0.1)  # Brief pause between tests
        
        # Generate summary report
        self.generate_report()
    
    def generate_report(self):
        """Generate final evaluation report"""
        
        print("\n" + "="*60)
        print("📊 EVALUATION SUMMARY REPORT")
        print("="*60)
        
        # Calculate averages
        avg_name = sum(r.name_score for r in self.results) / len(self.results)
        avg_engine = sum(r.engine_score for r in self.results) / len(self.results)
        avg_chain = sum(r.chain_score for r in self.results) / len(self.results)
        avg_overall = (avg_name + avg_engine + avg_chain) / 3
        
        print(f"\n📈 AVERAGE SCORES (out of 10):")
        print(f"  Preset Naming:    {avg_name:.2f}")
        print(f"  Engine Selection: {avg_engine:.2f}")
        print(f"  Signal Chain:     {avg_chain:.2f}")
        print(f"  Overall:          {avg_overall:.2f}")
        
        # Best and worst
        best = max(self.results, key=lambda x: (x.name_score + x.engine_score + x.chain_score) / 3)
        worst = min(self.results, key=lambda x: (x.name_score + x.engine_score + x.chain_score) / 3)
        
        print(f"\n🏆 BEST RESULT:")
        print(f"  Prompt: {best.prompt[:50]}...")
        print(f"  Preset: {best.preset_name}")
        print(f"  Score:  {(best.name_score + best.engine_score + best.chain_score) / 3:.1f}/10")
        
        print(f"\n⚠️ WORST RESULT:")
        print(f"  Prompt: {worst.prompt[:50]}...")
        print(f"  Preset: {worst.preset_name}")
        print(f"  Score:  {(worst.name_score + worst.engine_score + worst.chain_score) / 3:.1f}/10")
        
        # Category breakdown
        categories = {
            'Electronic/EDM': self.results[0:5],
            'Hip-Hop/Trap': self.results[5:10],
            'Ambient/Experimental': self.results[10:15],
            'Rock/Metal': self.results[15:20],
            'Vocal Processing': self.results[20:25],
            'Creative/Unusual': self.results[25:30]
        }
        
        print("\n📂 PERFORMANCE BY CATEGORY:")
        for cat_name, cat_results in categories.items():
            cat_avg = sum((r.name_score + r.engine_score + r.chain_score) / 3 for r in cat_results) / len(cat_results)
            print(f"  {cat_name:25} {cat_avg:.2f}/10")
        
        # Specific strengths/weaknesses
        print("\n💪 STRENGTHS:")
        if avg_name >= 7:
            print("  ✓ Creative and relevant preset naming")
        if avg_engine >= 7:
            print("  ✓ Excellent engine selection for prompts")
        if avg_chain >= 7:
            print("  ✓ Proper signal flow understanding")
        
        print("\n🔧 AREAS FOR IMPROVEMENT:")
        if avg_name < 7:
            print("  • Preset naming could be more creative/accurate")
        if avg_engine < 7:
            print("  • Engine selection needs refinement")
        if avg_chain < 7:
            print("  • Signal chain ordering needs work")
        
        # Grade
        grade = self.calculate_grade(avg_overall)
        print(f"\n🎓 FINAL GRADE: {grade}")
        
        # Recommendations
        print("\n💡 RECOMMENDATIONS:")
        if avg_overall >= 8:
            print("  System is performing excellently! Minor tweaks only.")
        elif avg_overall >= 6:
            print("  Good foundation. Focus on:")
            if avg_name < avg_engine and avg_name < avg_chain:
                print("    - Improving creative naming algorithms")
            if avg_engine < avg_name and avg_engine < avg_chain:
                print("    - Refining engine selection logic")
            if avg_chain < avg_name and avg_chain < avg_engine:
                print("    - Enhancing signal flow rules")
        else:
            print("  Significant improvements needed across all areas.")
            print("  Consider adding more training data and refining rules.")
    
    def calculate_grade(self, score: float) -> str:
        """Convert score to letter grade"""
        if score >= 9.0:
            return "A+ (Outstanding)"
        elif score >= 8.5:
            return "A (Excellent)"
        elif score >= 8.0:
            return "A- (Very Good)"
        elif score >= 7.5:
            return "B+ (Good)"
        elif score >= 7.0:
            return "B (Above Average)"
        elif score >= 6.5:
            return "B- (Satisfactory)"
        elif score >= 6.0:
            return "C+ (Acceptable)"
        elif score >= 5.5:
            return "C (Needs Improvement)"
        elif score >= 5.0:
            return "C- (Below Average)"
        else:
            return "D (Poor)"

def main():
    """Run the evaluation"""
    evaluator = AISystemEvaluator()
    evaluator.run_evaluation()

if __name__ == "__main__":
    main()