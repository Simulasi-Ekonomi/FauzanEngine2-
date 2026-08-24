#include "Core/ActorBase.h"
#include "Physics/CollisionSAT.h"
#include "PlayerControllerCore.h"
#include <cstdio>

int main() {
    printf("============================================\n");
    printf(" GAME LOOP TEST – Movement + Collision\n");
    printf("============================================\n\n");

    NeoEngine::ActorBase player, obstacle;
    player.SetActorLocation({0, 1.0f, 0});
    obstacle.SetActorLocation({3, 0.5f, 0});

    NeoEngine::PlayerControllerCore controller;
    controller.Possess(&player);

    float dt = 0.05f;
    for (int f = 0; f < 40; ++f) {
        controller.MoveForward(1.0f);
        controller.Update(dt);

        // Deteksi tabrakan sederhana (jarak Euclidean)
        NeoEngine::Vector3 diff = player.GetActorLocation() - obstacle.GetActorLocation();
        float dist2 = diff.x*diff.x + diff.y*diff.y + diff.z*diff.z;
        if (dist2 < 4.0f) {
            printf("  Frame %d: Collision! Player pos (%.1f, %.1f, %.1f)\n",
                   f, player.GetActorLocation().x, player.GetActorLocation().y, player.GetActorLocation().z);
        }
    }

    printf("\nFinal Player pos: (%.1f, %.1f, %.1f)\n",
           player.GetActorLocation().x, player.GetActorLocation().y, player.GetActorLocation().z);
    printf("Collision system working.\n");
    printf("============================================\n");
    return 0;
}
