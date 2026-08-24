import threading
import time
import traceback
import random
import copy
from collections import deque

class AriesCognitiveBridge:
    def __init__(self, brain):
        self.brain = brain
        self.modules = {}
        self.interval = 777
        self.status = "Initializing"
        self.reward_history = deque(maxlen=20)  # buffer untuk 20 reward terakhir

    def start_autonomy(self):
        """Start the autonomous cognitive bridge lifecycle in a daemon thread."""
        if not self._safe_import():
            print("[BRIDGE] Safe import failed, autonomy not started")
            return
        t = threading.Thread(target=self._main_lifecycle, daemon=True)
        t.start()
        self.status = "Running"
        print("[BRIDGE] Autonomous cognitive lifecycle started")

    # =========================================================
    # SAFE IMPORT
    # =========================================================
    def _safe_import(self):
        try:
            from core_modules.global_generator_guard import GlobalGeneratorGuard
            from core_modules.sigma_core import SigmaCore
            from core_modules.omega_core import OmegaCore
            from core_modules.omega_kernel import OmegaKernel
            from core_modules.aries_evolution import AriesEvolutionEngine
            from core_modules.aries_goal_generator import AriesAdaptiveGoalGenerator
            from core_modules.aries_goal_translator import AriesGoalActionTranslator
            from core_modules.aries_action_executor import AriesActionExecutor
            from core_modules.aries_reward_engine import AriesRewardEngine
            from core_modules.aries_policy_memory import AriesPolicyMemory
            from core_modules.aries_safety_engine import AriesSafetyConstraintEngine
            from core_modules.aries_strategy_memory import AriesStrategyAbstractionEngine
            from core_modules.aries_meta_policy import AriesMetaPolicyEngine
            from core_modules.aries_predictive_simulator import AriesPredictiveSimulationEngine
            from core_modules.aries_mcts_planner import AriesMCTSPlanner
            from core_modules.aries_world_model import AriesWorldModel
            from core_modules.aries_value_network import AriesValueNetwork

            guard = GlobalGeneratorGuard()
            self.modules["sigma"] = SigmaCore(guard=guard)
            self.modules["omega_core"] = OmegaCore(guard=guard)
            self.modules["omega_kernel"] = OmegaKernel(guard=guard)
            self.modules['meta'] = AriesMetaPolicyEngine(self.brain)
            self.modules['world_model'] = AriesWorldModel(self.brain)
            self.modules['policy'] = AriesPolicyMemory(self.brain)
            self.modules['evolution'] = AriesEvolutionEngine(self.brain)
            self.modules['goal_gen'] = AriesAdaptiveGoalGenerator(self.brain)
            self.modules['translator'] = AriesGoalActionTranslator(self.brain)
            self.modules['executor'] = AriesActionExecutor(self.brain)
            # ✅ RewardEngine tanpa clip_min
            self.modules['reward'] = AriesRewardEngine(self.brain)
            self.modules['safety'] = AriesSafetyConstraintEngine(self.brain)
            self.modules['strategy'] = AriesStrategyAbstractionEngine(self.brain)
            self.modules['predictor'] = AriesPredictiveSimulationEngine(self.brain)
            self.modules['mcts'] = AriesMCTSPlanner(self.brain)
            self.modules['value_net'] = AriesValueNetwork(lr=0.01)

            self.brain.modules = self.modules
            self.brain.evolution_engine = self.modules['evolution']
            self.brain.reward_engine = self.modules['reward']

            self.modules['world_model'].train()
            self.modules["sigma"].activate()

            return True
        except Exception as e:
            print(f"❌ [BRIDGE IMPORT ERROR] {e}", flush=True)
            traceback.print_exc()
            return False

    # =========================================================
    # ADVANCED REWARD NORMALIZATION
    # =========================================================
    def _normalize_reward_advanced(self, raw_reward):
        clipped = max(0.0, raw_reward)
        self.reward_history.append(clipped)
        if len(self.reward_history) > 0:
            avg_hist = sum(self.reward_history) / len(self.reward_history)
            max_hist = max(self.reward_history)
        else:
            avg_hist = 0.0
            max_hist = 1.0
        scaled = clipped / max_hist if max_hist > 0 else clipped
        normalized = max(0.0, scaled - (avg_hist * 0.1))  # 10% penalti relatif ke rata-rata
        return normalized

    # =========================================================
    # MAIN LOOP
    # =========================================================
    def _main_lifecycle(self):
        time.sleep(15)
        cycle_count = 0
        while True:
            try:
                s_before = self.modules['reward'].evaluate_state()

                if s_before["node_count"] < 5:
                    print("🚨 Emergency Reseed Triggered", flush=True)
                    self.modules['evolution'].force_reseed(10)
                    time.sleep(2)
                    continue

                sigma_result = self.modules["sigma"].research_cycle()
                best_action = self.modules['mcts'].search(s_before)
                cycle_reward = 0.0

                if best_action:
                    action_type = best_action.get('action_type')
                    print(f"⚙️ MCTS Action: {action_type}", flush=True)

                    safe_actions = self.modules['safety'].apply_constraints([best_action])
                    if safe_actions:
                        self.modules['executor'].force_execute(safe_actions[0])
                        time.sleep(2)
                        self.modules['evolution'].evolve()
                        time.sleep(2)

                        s_after = self.modules['reward'].evaluate_state()
                        self.modules['world_model'].record_transition(s_before, action_type, s_after)

                        raw_reward = self.modules['reward'].evaluate_tca(s_before, s_after)
                        cycle_reward = self._normalize_reward_advanced(raw_reward)

                        value_net = self.modules.get("value_net")
                        meta = self.modules['meta'].load_meta()
                        gamma = meta.get("discount_factor", 0.85)
                        if value_net:
                            td_target = cycle_reward + gamma * value_net.predict(s_after)
                            value_net.update(s_before, td_target)

                        omega_out = self.modules["omega_core"].process(action_type, str(s_after))
                        kernel_out = self.modules["omega_kernel"].run_cycle(action_type)

                        self.modules['policy'].update_policy([action_type], cycle_reward)
                        self.modules['strategy'].record_strategy([action_type], cycle_reward)
                        print(f"✅ Adv. Reward: {round(cycle_reward,4)} (Raw: {round(raw_reward,4)})", flush=True)
                    else:
                        print("⚠️ Action blocked by Safety", flush=True)
                else:
                    print("🔍 Fallback Goal Generation", flush=True)
                    self.modules['goal_gen'].generate_goals()
                    self.modules['translator'].process_goals()
                    pending = self.modules['executor'].process_actions()
                    if pending:
                        self.modules['evolution'].evolve()

                self.modules['meta'].evolve_meta(cycle_reward)
                cycle_count += 1

                if cycle_count % 5 == 0:
                    self.modules['world_model'].train()

                time.sleep(self.interval)

            except Exception as e:
                print(f"⚠️ [BRIDGE CRASH] {e}", flush=True)
                traceback.print_exc()
                time.sleep(self.interval)
