"""
Disable automatic utility engine addition in Calculator and Alchemist
Only add utilities when explicitly requested by the user
"""

def disable_auto_utilities():
    """Monkey patch to disable automatic utility engine addition"""
    
    # Import the modules
    import calculator
    import alchemist
    
    # 1. Disable Calculator utility addition
    def no_op_analyze_and_add_utility_engines(self, preset, prompt_lower, blueprint, nudge_log):
        """Disabled - no automatic utility engines"""
        nudge_log["utility_engines_added"] = []
        logger.info("Calculator: Automatic utility engine addition disabled")
    
    # Replace the method
    calculator.Calculator._analyze_and_add_utility_engines = no_op_analyze_and_add_utility_engines
    
    # 2. Disable Alchemist utility addition
    def no_op_final_utility_engine_check(self, preset):
        """Disabled - no automatic utility engines"""
        if "alchemist_metadata" not in preset:
            preset["alchemist_metadata"] = {}
        preset["alchemist_metadata"]["final_utility_additions"] = []
        logger.info("Alchemist: Automatic utility engine addition disabled")
    
    # Replace the method
    alchemist.Alchemist._final_utility_engine_check = no_op_final_utility_engine_check
    
    print("✅ Automatic utility engine addition DISABLED")
    print("   Utilities will only be added when explicitly requested")
    
    import logging
    logger = logging.getLogger(__name__)

if __name__ == "__main__":
    disable_auto_utilities()