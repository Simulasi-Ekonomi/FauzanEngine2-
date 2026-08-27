#include "Systems/FarmCommerceEntitlementLedger.h"
#include "Systems/FarmWorldTool.h"
#include "Systems/TrustSafetySystem.h"

#include <cstdio>

using namespace NeoEngine;

int main() {
    FarmSystem farm(4U, 4U, 100);
    TrustSafetySystem trust;
    farm.SetTrustSafety(&trust, "player-a");
    farm.SetReceiptVerifier([](const VerifiedTopUpReceipt& receipt) { return receipt.authorityPayload == "provider-ok"; });
    FarmWorldTool world;
    FarmWorldConfig worldConfig{};
    worldConfig.worldWidth = 4U; worldConfig.worldHeight = 4U; worldConfig.npcCount = 2U;
    FarmCommerceEntitlementLedger ledger;
    FarmCommerceAuditReceipt audit{};
    bool ok = !ledger.Initialize(world, "player-a", [](const FarmProviderReceipt&) { return true; }) && ledger.LastError() == FarmCommerceError::InvalidConfiguration;
    ok = ok && world.Initialize(farm, trust, "player-a", worldConfig) && ledger.Initialize(world, "player-a", [](const FarmProviderReceipt& receipt) { return receipt.authorityPayload == "provider-ok"; }, &trust);
    const int64_t initialCoins = farm.Coins();
    ok = ok && !ledger.Apply({1U, "player-b", 25, "provider-ok", false}, audit) && ledger.LastError() == FarmCommerceError::WrongPlayer && farm.Coins() == initialCoins;
    ok = ok && !ledger.Apply({1U, "player-a", 25, "bad", false}, audit) && ledger.LastError() == FarmCommerceError::VerifierRejected && farm.Coins() == initialCoins;
    ok = ok && ledger.Apply({1U, "player-a", 25, "provider-ok", false}, audit) && audit.version == FarmCommerceAuditReceipt::kVersion && audit.kind == FarmCommerceAuditKind::Approved && audit.providerReceiptId == 1U && audit.entitlementCoins == 25 && ledger.AcceptedReceiptCount() == 1U && farm.Coins() == initialCoins + 25;
    const FarmCommerceAuditReceipt approvedAudit = audit;
    audit.providerReceiptId = 99U;
    ok = ok && ledger.LastAudit() && ledger.LastAudit()->providerReceiptId == approvedAudit.providerReceiptId;
    ok = ok && !ledger.Apply({1U, "player-a", 25, "provider-ok", false}, audit) && ledger.LastError() == FarmCommerceError::Duplicate && trust.Score("player-a") == 3U && !trust.IsBanned("player-a") && farm.Coins() == initialCoins + 25;
    ok = ok && !ledger.Apply({1U, "player-a", 25, "provider-ok", true}, audit) && ledger.LastError() == FarmCommerceError::Reversed && farm.Coins() == initialCoins + 25;
    ok = ok && !ledger.Reconcile(1U, 24, audit) && ledger.LastError() == FarmCommerceError::ReconciliationMismatch && trust.Score("player-a") == 8U && trust.IsBanned("player-a") && ledger.Reconcile(1U, 25, audit) && audit.kind == FarmCommerceAuditKind::Approved && audit.providerReceiptId == 1U;
    std::vector<FarmCommerceAuditReceipt> exported;
    ok = ok && ledger.ExportAuditLog(exported) && exported.size() == 7U && exported.front().sequence == 1U && exported.back().kind == FarmCommerceAuditKind::Approved && exported.back().providerReceiptId == 1U;
    if (!exported.empty()) exported.front().providerReceiptId = 999U;
    ok = ok && ledger.LastAudit() && ledger.LastAudit()->providerReceiptId == 1U;
    if (!ok) { std::fprintf(stderr, "FARM_COMMERCE_ENTITLEMENT_SMOKE_FAIL\n"); return 1; }
    std::printf("FARM_COMMERCE_ENTITLEMENT_SMOKE_OK approved=1 duplicate=1 reversal=1 reconcile=1 fraud_report=duplicate,reconcile ban=1 audit_export=%zu\n", exported.size());
    return 0;
}
