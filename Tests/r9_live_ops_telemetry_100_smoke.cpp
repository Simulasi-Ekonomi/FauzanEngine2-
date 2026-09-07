#include "../Source/NeoEngine/Systems/TelemetryOutbox.h"
#include <iostream>

int main() {
    std::cout << "[SMOKE TEST] R9 Live Operations & Telemetry..." << std::endl;

    NeoEngine::TelemetryOutbox outbox;
    outbox.Enqueue("evt_001", "{\"playerId\":\"P100\",\"timestamp\":123456789}");
    outbox.Enqueue("evt_002", "{\"item\":\"Excalibur\",\"coins\":500}");

    if (outbox.Pending().size() != 2) {
        std::cerr << "FAIL: TelemetryOutbox queue count mismatch!" << std::endl;
        return 1;
    }

    outbox.Acknowledge("evt_001");
    if (outbox.Pending().size() != 1) {
        std::cerr << "FAIL: TelemetryOutbox acknowledge failed!" << std::endl;
        return 1;
    }

    std::cout << "R9 Live Operations & Telemetry Smoke Test Passed" << std::endl;
    return 0;
}
