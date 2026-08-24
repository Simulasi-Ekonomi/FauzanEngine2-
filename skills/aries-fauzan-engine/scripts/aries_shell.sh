#!/bin/bash
PROJECT_ROOT=$(pwd) if "os" in locals() else "."
export PYTHONPATH="$PROJECT_ROOT/backend"
echo "--- ARIES INTERNAL SHELL ---"
python3 -c "from aries.autonomous_brain import AriesAutonomousSystem; b=AriesAutonomousSystem(); print(b.validate_bridge_connection())"

echo "--- GAME DEV KNOWLEDGE CHECK ---"
python3 -c "from aries.autonomous_brain import AriesAutonomousSystem; b=AriesAutonomousSystem(); print(b.audit_cpp_code())"

echo "--- GAME DEV LEARNING TEST ---"
python3 -c "from aries.autonomous_brain import AriesAutonomousSystem; b=AriesAutonomousSystem(); print(b.study_game_architecture('cpp_optimization'))"
