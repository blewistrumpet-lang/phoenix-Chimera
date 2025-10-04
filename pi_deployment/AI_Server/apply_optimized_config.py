#!/usr/bin/env python3
"""
Apply optimized configuration to Trinity Pipeline
This is the proper way to update the pipeline after training
"""

import json
import shutil
from pathlib import Path
from datetime import datetime

def apply_configuration(config_file="best_forced_cloud_config.json"):
    """Apply learned configuration to the Trinity pipeline"""
    
    print("\n" + "="*70)
    print("APPLYING OPTIMIZED CONFIGURATION TO TRINITY PIPELINE")
    print("="*70)
    
    # 1. Load the optimized config
    if not Path(config_file).exists():
        print(f"❌ Config file not found: {config_file}")
        return False
    
    with open(config_file, 'r') as f:
        optimized_config = json.load(f)
    
    # 2. Backup current config
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    backup_dir = Path("config_backups")
    backup_dir.mkdir(exist_ok=True)
    
    # 3. Create new Trinity configuration file
    trinity_config = {
        "pipeline_version": "3.0",
        "optimized_date": timestamp,
        "training_method": "forced_cloud_50_generations",
        
        # Apply learned parameters to each component
        "visionary_config": optimized_config.get("visionary", {}),
        "oracle_config": optimized_config.get("oracle", {}),
        "calculator_config": optimized_config.get("calculator", {}),
        "alchemist_config": optimized_config.get("alchemist", {})
    }
    
    # 4. Save as active configuration
    with open("trinity_active_config.json", 'w') as f:
        json.dump(trinity_config, f, indent=2)
    
    print("✅ Configuration saved to: trinity_active_config.json")
    
    # 5. Update main.py to use the optimized configuration
    update_main_py = '''
# Add this to main.py to load optimized configuration
def load_optimized_config():
    """Load optimized Trinity configuration if available"""
    config_path = Path("trinity_active_config.json")
    if config_path.exists():
        with open(config_path, 'r') as f:
            config = json.load(f)
            return config
    return None

# In the main server initialization:
optimized_config = load_optimized_config()
if optimized_config:
    logger.info(f"Loading optimized config from {optimized_config.get('optimized_date')}")
    
    # Apply to components
    # This would modify how oracle, calculator, etc are initialized
    # For example:
    # oracle.engine_weight = optimized_config['oracle_config']['engine_weight']
'''
    
    # 6. Create config loader module
    with open("config_loader.py", 'w') as f:
        f.write('''"""
Trinity Configuration Loader
Loads and applies optimized configurations from training
"""

import json
from pathlib import Path
import logging

logger = logging.getLogger(__name__)

class TrinityConfigLoader:
    """Manages Trinity pipeline configuration"""
    
    def __init__(self, config_file="trinity_active_config.json"):
        self.config = self.load_config(config_file)
        
    def load_config(self, config_file):
        """Load configuration from file"""
        path = Path(config_file)
        if path.exists():
            with open(path, 'r') as f:
                config = json.load(f)
                logger.info(f"Loaded optimized config from {config.get('optimized_date', 'unknown')}")
                return config
        else:
            logger.warning("No optimized config found, using defaults")
            return self.get_default_config()
    
    def get_default_config(self):
        """Default Trinity configuration"""
        return {
            "visionary_config": {
                "temperature": 0.7,
                "use_cloud": True,
                "keyword_weight": 1.0,
                "creativity_bias": 0.5
            },
            "oracle_config": {
                "engine_weight": 10.0,
                "vibe_weight": 1.0,
                "search_k": 5,
                "similarity_threshold": 0.3
            },
            "calculator_config": {
                "nudge_intensity": 0.5,
                "keyword_sensitivity": 1.0,
                "harmonic_balance": 0.5,
                "parameter_range": 0.3
            },
            "alchemist_config": {
                "validation_strictness": 0.5,
                "name_creativity": 0.7,
                "safety_threshold": 0.8,
                "max_effects": 6
            }
        }
    
    def apply_to_pipeline(self, oracle, calculator, alchemist):
        """Apply configuration to Trinity components"""
        
        # Apply Oracle configuration
        if hasattr(oracle, 'engine_weight'):
            oracle.engine_weight = self.config['oracle_config'].get('engine_weight', 10.0)
        
        # Apply Calculator configuration
        if hasattr(calculator, 'keyword_sensitivity'):
            calculator.keyword_sensitivity = self.config['calculator_config'].get('keyword_sensitivity', 1.0)
        
        # Apply Alchemist configuration
        if hasattr(alchemist, 'max_effects'):
            alchemist.max_effects = self.config['alchemist_config'].get('max_effects', 6)
        
        logger.info("Configuration applied to Trinity pipeline")
        return True

# Global config loader instance
config_loader = TrinityConfigLoader()
''')
    
    print("✅ Created config_loader.py module")
    
    # 7. Show configuration summary
    print("\n📊 Optimized Configuration Summary:")
    print(f"  Cloud AI: {optimized_config['visionary']['use_cloud']} (must be True)")
    print(f"  Engine Weight: {optimized_config['oracle']['engine_weight']:.1f}")
    print(f"  Keyword Sensitivity: {optimized_config['calculator']['keyword_sensitivity']:.2f}")
    print(f"  Max Effects: {optimized_config['alchemist']['max_effects']}")
    
    # 8. Integration instructions
    print("\n📝 To integrate into main.py:")
    print("  1. Import: from config_loader import config_loader")
    print("  2. After initializing components:")
    print("     config_loader.apply_to_pipeline(oracle, calculator, alchemist)")
    print("  3. Restart server to apply changes")
    
    return True

def verify_and_test():
    """Verify the configuration is properly applied"""
    print("\n🧪 Testing configuration...")
    
    # Test that config can be loaded
    try:
        with open("trinity_active_config.json", 'r') as f:
            config = json.load(f)
        
        # Verify cloud AI is on
        if not config['visionary_config'].get('use_cloud', False):
            print("❌ WARNING: Cloud AI is OFF in config!")
            return False
        
        print("✅ Configuration valid and cloud AI enabled")
        return True
    except Exception as e:
        print(f"❌ Error loading config: {e}")
        return False

if __name__ == "__main__":
    # Apply the configuration
    if apply_configuration():
        verify_and_test()
        
        print("\n" + "="*70)
        print("✅ CONFIGURATION UPDATE COMPLETE")
        print("  Next step: Restart the Trinity server")
        print("="*70)