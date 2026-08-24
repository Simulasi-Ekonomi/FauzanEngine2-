/*
 * FAUZA ENGINE - CORE GENERATED COMPONENT
 * Powered by Aries S7 Foundation Knowledge
 * Focus: NPC Intelligence & State Machines
 * Ref: YOUTUBE MASTERY Level 6 & Angelica Sovereign Core
 */

#include <iostream>
#include <string>
#include <map>

enum class NPCState {
    IDLE,
    PATROL,
    CHASE,
    ATTACK
};

class AriesNPCBrain {
private:
    NPCState currentState;
    std::string npcName;
    float detectionRange = 15.0f;

public:
    AriesNPCBrain(std::string name) : npcName(name), currentState(NPCState::IDLE) {
        std::cout << "[ARIES] NPC Brain '" << name << "' initialized with State Machine logic." << std::endl;
    }

    // Standar Level 6: State Transition Logic
    void updateState(float distanceToPlayer) {
        NPCState oldState = currentState;

        if (distanceToPlayer < 5.0f) {
            currentState = NPCState::ATTACK;
        } else if (distanceToPlayer < detectionRange) {
            currentState = NPCState::CHASE;
        } else {
            currentState = NPCState::PATROL;
        }

        if (oldState != currentState) {
            notifyStateChange();
        }
    }

    void notifyStateChange() {
        // Insight S7: State changes should be event-driven to save CPU
        std::cout << "[NPC:" << npcName << "] Transitioned to new state." << std::endl;
    }

    void executeBehavior() {
        switch (currentState) {
            case NPCState::IDLE: /* Do nothing */ break;
            case NPCState::PATROL: /* Move between waypoints */ break;
            case NPCState::CHASE: /* Move towards player */ break;
            case NPCState::ATTACK: /* Play attack animation */ break;
        }
    }
};
