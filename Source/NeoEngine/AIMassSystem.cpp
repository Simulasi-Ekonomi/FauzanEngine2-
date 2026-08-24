#include "AIControllerCore.h"
#include <vector>

class FAIMassSystem {
    struct FAIAgent { float Pos[3]; float Target[3]; };
    std::vector<FAIAgent> Agents;

    void BulkUpdateAI() {
        // AI logic untuk 1000+ NPC tanpa drop FPS
    }
};
