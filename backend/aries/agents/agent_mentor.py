import os

class AgentMentor:
    def __init__(self):
        self.path = os.getcwd()
        os.makedirs(self.path, exist_ok=True)

    def write_mastery_template(self, level):
        templates = {
            1: ("Lvl1_Basics.cpp", "// Focus: Basic Syntax & Control Flow"),
            5: ("Lvl5_ComponentSystem.cpp", "// Focus: ECS (Entity Component System)"),
            8: ("Lvl8_Optimization.cpp", "// Focus: Data Oriented Design & Cache Locality")
        }
        
        filename, focus = templates.get(level, ("Lvl_Unknown.cpp", "// General Logic"))
        content = f"""/* * S7 MASTERY SERIES: LEVEL {level}
 * {focus}
 */
#include <iostream>

int main() {{
    std::cout << "Executing Mastery Level {level} Code..." << std::endl;
    // S7 Insight: Always use Smart Pointers for Level 5+
    return 0;
}}
"""
        with open(f"{self.path}/{filename}", 'w') as f:
            f.write(content)
        print(f"[MENTOR] Level {level} Mastery file generated at {filename}")

if __name__ == "__main__":
    mentor = AgentMentor()
    for lvl in [1, 5, 8]: mentor.write_mastery_template(lvl)
