import os, sys

class AgentArtist:
    def __init__(self):
        self.asset_dir = os.getcwd()
        os.makedirs(self.asset_dir, exist_ok=True)

    def generate_sprite_anatomy(self, character_name):
        character_name = character_name.replace(" ", "_")
        anatomy = f"""# ANATOMY OF {character_name.upper()}
# Base Size: 32x32
# Technical Standard: Vertex Stacking
# Created from S7 Visual Cortex

[ANIMATION_STREAMS]
IDLE: [0, 1] @ 200ms
ACTION: [2, 3, 4, 5] @ 100ms
"""
        file_path = f"{self.asset_dir}/{character_name}_anatomy.txt"
        with open(file_path, 'w') as f:
            f.write(anatomy)
        print(f"[ARTIST] Sprite anatomy for '{character_name}' locked at {file_path}")

if __name__ == "__main__":
    name = sys.argv[1] if len(sys.argv) > 1 else "Unknown_Entity"
    artist = AgentArtist()
    artist.generate_sprite_anatomy(name)
