#!/bin/bash
PROJECT_ROOT=$(pwd) if "os" in locals() else "."
export PYTHONPATH="$PROJECT_ROOT/backend"
echo "--- ARIES LIVE LEARNING TEST ---"
python3 -c "from aries.autonomous_brain import AriesAutonomousSystem; b = AriesAutonomousSystem(); print(b.live_web_search_simulation('Unreal Engine C++ Asset Pipeline'))"
