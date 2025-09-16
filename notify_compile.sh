#!/bin/bash
# Wrapper script that notifies before compiling/running tests

# Play notification sound
afplay /System/Library/Sounds/Hero.aiff &

# Show what we're about to do
echo "🔔 PERMISSION NEEDED: About to compile and run test"
echo "Command: $@"

# Execute the command
"$@"

# Notify completion
afplay /System/Library/Sounds/Glass.aiff &
echo "🔔 COMPLETED: Test execution finished"