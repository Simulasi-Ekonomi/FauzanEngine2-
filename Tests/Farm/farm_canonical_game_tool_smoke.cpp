#include "Systems/FarmCanonicalGameTool.h"
#include "Systems/TrustSafetySystem.h"

#include <cstdio>
#include <vector>

namespace {

constexpr char kPlayerId[] = "canonical-tool-player";

bool InitializeWorld(NeoEngine::FarmSystem& farm, NeoEngine::TrustSafetySystem& trust, NeoEngine::FarmWorldTool& world) {
    farm.SetTrustSafety(&trust, kPlayerId);
    NeoEngine::FarmWorldConfig config{};
    config.worldWidth = 8;
    config.worldHeight = 8;
    config.npcCount = 4;
    config.maxBuildings = 16;
    config.maxQuests = 16;
    return world.Initialize(farm, trust, kPlayerId, config);
}

} // namespace

int main() {
    const auto fail = [](const char* stage) {
        std::fprintf(stderr, "FARM_CANONICAL_GAME_TOOL_SMOKE_FAIL stage=%s\n", stage);
        return 1;
    };
    NeoEngine::FarmSystem farm(8, 8, 100);
    NeoEngine::TrustSafetySystem trust;
    NeoEngine::FarmWorldTool world;
    if (!InitializeWorld(farm, trust, world)) return fail("world-init");

    NeoEngine::FarmCanonicalGameTool tool;
    NeoEngine::FarmToolRules rules{};
    rules.maxCommands = 32;
    rules.maxContentEntries = 8;
    rules.maxReplaySteps = 16;
    rules.maxMoveDistance = 1;
    if (!tool.Initialize(world, rules) || !tool.RegisterContent("farm.scene", 1, 0x1111) ||
        !tool.RegisterContent("farm.tile", 2, 0x2222) || !tool.HasContent("farm.scene") || !tool.HasContent("farm.tile") ||
        tool.Content().size() != 2U || tool.Content()[0].id != "farm.scene") {
        return fail("typed-contract");
    }

    const uint64_t beforeInvalidContent = tool.DeterministicState();
    if (tool.RegisterContent("../escape", 1, 0x3333) || tool.DeterministicState() != beforeInvalidContent ||
        tool.LastError() != NeoEngine::FarmCanonicalToolError::InvalidContent) {
        return fail("invalid-content");
    }
    const NeoEngine::FarmToolRules invalidRules{0, 8, 16, 1};
    if (tool.ApplyRules(invalidRules) || tool.DeterministicState() != beforeInvalidContent ||
        tool.LastError() != NeoEngine::FarmCanonicalToolError::InvalidRules) {
        return fail("invalid-rules");
    }

    const std::vector<NeoEngine::FarmToolCommand> commands{
        {1, NeoEngine::FarmToolCommandKind::Move, 1, 0, 0, 0},
        {2, NeoEngine::FarmToolCommandKind::Move, 0, 1, 0, 0},
        {3, NeoEngine::FarmToolCommandKind::Till, 0, 0, 1, 1},
    };
    const auto replay = tool.Replay(commands);
    if (!replay.accepted || replay.appliedCommands != commands.size() || replay.deterministicState == 0U || tool.LastAppliedSequence() != 3U) {
        return fail("replay");
    }
    const uint64_t afterReplay = tool.DeterministicState();
    const auto duplicate = tool.Apply({3, NeoEngine::FarmToolCommandKind::Move, 0, 0, 0, 0});
    if (duplicate.accepted || duplicate.error != NeoEngine::FarmCanonicalToolError::InvalidSequence || tool.DeterministicState() != afterReplay) {
        return fail("sequence-rejection");
    }
    const auto tooFar = tool.Apply({4, NeoEngine::FarmToolCommandKind::Move, 2, 0, 0, 0});
    if (tooFar.accepted || tooFar.error != NeoEngine::FarmCanonicalToolError::InvalidCommand || tool.DeterministicState() != afterReplay) {
        return fail("command-rejection");
    }

    NeoEngine::FarmSystem replayFarm(8, 8, 100);
    NeoEngine::TrustSafetySystem replayTrust;
    NeoEngine::FarmWorldTool replayWorld;
    NeoEngine::FarmCanonicalGameTool replayTool;
    if (!InitializeWorld(replayFarm, replayTrust, replayWorld) || !replayTool.Initialize(replayWorld, rules) ||
        !replayTool.RegisterContent("farm.scene", 1, 0x1111) || !replayTool.RegisterContent("farm.tile", 2, 0x2222) ||
        !replayTool.Replay(commands).accepted || replayTool.DeterministicState() != afterReplay) {
        return fail("determinism");
    }

    const std::vector<uint8_t> saved = tool.Save();
    if (saved.empty()) return fail("save");
    NeoEngine::FarmSystem restoredFarm(8, 8, 100);
    NeoEngine::TrustSafetySystem restoredTrust;
    NeoEngine::FarmWorldTool restoredWorld;
    NeoEngine::FarmCanonicalGameTool restoredTool;
    if (!InitializeWorld(restoredFarm, restoredTrust, restoredWorld) || !restoredTool.Initialize(restoredWorld, rules) ||
        !restoredTool.Load(saved) || restoredTool.DeterministicState() != afterReplay || restoredTool.Content().size() != 2U ||
        restoredTool.LastAppliedSequence() != 3U) {
        return fail("v2-load");
    }

    const std::vector<uint8_t> legacy = tool.SerializeLegacyV1ForCompatibility();
    NeoEngine::FarmSystem migratedFarm(8, 8, 100);
    NeoEngine::TrustSafetySystem migratedTrust;
    NeoEngine::FarmWorldTool migratedWorld;
    NeoEngine::FarmCanonicalGameTool migratedTool;
    if (legacy.empty() || !InitializeWorld(migratedFarm, migratedTrust, migratedWorld) || !migratedTool.Initialize(migratedWorld, rules) ||
        !migratedTool.Load(legacy) || migratedTool.DeterministicState() != afterReplay || migratedTool.Rules().maxMoveDistance != 1U ||
        migratedTool.Content().size() != 2U || !migratedTool.HasContent("farm.scene") || !migratedTool.HasContent("farm.tile")) {
        std::fprintf(stderr, "FARM_CANONICAL_GAME_TOOL_MIGRATION_DIAG bytes=%zu error=%u state=%llu expected=%llu content=%zu sequence=%llu\n",
            legacy.size(), static_cast<unsigned>(migratedTool.LastError()), static_cast<unsigned long long>(migratedTool.DeterministicState()),
            static_cast<unsigned long long>(afterReplay), migratedTool.Content().size(), static_cast<unsigned long long>(migratedTool.LastAppliedSequence()));
        return fail("v1-migration");
    }

    std::vector<uint8_t> corrupt = saved;
    corrupt.back() ^= 0x01U;
    const uint64_t beforeCorruptLoad = restoredTool.DeterministicState();
    if (restoredTool.Load(corrupt) || restoredTool.LastError() != NeoEngine::FarmCanonicalToolError::MalformedPayload ||
        restoredTool.DeterministicState() != beforeCorruptLoad) {
        return fail("corrupt-rejection");
    }

    std::printf(
        "FARM_CANONICAL_GAME_TOOL_SMOKE_OK version=%u content=%zu commands=%u migration=1 invalid=1 deterministic=1 bytes=%zu state=%llu\n",
        NeoEngine::FarmCanonicalGameTool::kCurrentVersion,
        tool.Content().size(),
        replay.appliedCommands,
        saved.size(),
        static_cast<unsigned long long>(afterReplay));
    return 0;
}
