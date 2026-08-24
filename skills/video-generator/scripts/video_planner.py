#!/usr/bin/env python3
import json
import sys

def create_blueprint(project_name):
    blueprint = {
        "project": project_name,
        "clips": [],
        "audio_track": {"bgm": "", "narration": []},
        "status": "Planning"
    }
    with open(f"{project_name}_blueprint.json", "w") as f:
        json.dump(blueprint, f, indent=4)
    print(f"🎬 Blueprint video '{project_name}' telah dibuat.")

if __name__ == "__main__":
    if len(sys.argv) > 1:
        create_blueprint(sys.argv[1])
    else:
        print("Usage: python3 video_planner.py <project_name>")
