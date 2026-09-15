#include "../Source/NeoEngine/Systems/TelemetryOutbox.h"
#include <iostream>

int main() {
    std::cout << "[SMOKE TEST] R9 Live Operations & Telemetry..." << std::endl;
    NeoEngine::TelemetryOutbox outbox;
    if (!outbox.Enqueue("evt_001", "{\"playerId\":\"P100\",\"timestamp\":123456789}") ||
        !outbox.Enqueue("evt_002", "{\"item\":\"Excalibur\",\"coins\":500}") ||
        outbox.Pending().size() != 2) return 1;
    if (!outbox.Acknowledge("evt_001") || outbox.Pending().size() != 1) return 1;
    std::cout << "R9 Live Operations & Telemetry Smoke Test Passed" << std::endl;
    return 0;
}
