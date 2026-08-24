import importlib
import sys
import os
# Import Cognitive Layer dari folder synapse
sys.path.append(os.getcwd())
import cognitive_layer

try:
    import aries_agents
    aries_agents.patch_sys_path()
    from aries_agents import AGENT_MAP
except ImportError:
    print("❌ Error: Run 'rebuild_agent_map.py' first!")
    sys.exit(1)

class StandbyBridge:
    _active_instances = {}

    @classmethod
    def wake_agent(cls, agent_name):
        if agent_name in cls._active_instances:
            return cls._active_instances[agent_name]

        if agent_name in AGENT_MAP:
            try:
                module = importlib.import_module(agent_name)
                agent_class = getattr(module, agent_name)
                instance = agent_class()
                
                # INJEKSI KEPINTARAN DARI FOLDER SYNAPSE
                cognitive_layer.inject_wisdom(instance, agent_name)
                
                cls._active_instances[agent_name] = instance
                return instance
            except Exception as e:
                return f"❌ Bridge Error: {str(e)}"
        return "❌ Agent not found."

def execute(agent_name, instruction="Status Check"):
    target = StandbyBridge.wake_agent(agent_name)
    if isinstance(target, str) and target.startswith("❌"): return target 
    
    protocols = ["handle_task", "task", "execute"]
    for p in protocols:
        if hasattr(target, p):
            func = getattr(target, p)
            # Agen sekarang sudah punya self.identity karena diinjeksi
            return func(instruction)
            
    return f"⚠️ Agent {agent_name} lacks protocols."
