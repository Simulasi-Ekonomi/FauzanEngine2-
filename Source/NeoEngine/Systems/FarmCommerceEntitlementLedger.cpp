#include "FarmCommerceEntitlementLedger.h"

#include "FarmWorldTool.h"

#include <algorithm>

namespace NeoEngine {
namespace {
bool ValidText(const std::string& value, size_t minimum) {
    return value.size() >= minimum && value.size() <= 96U && std::all_of(value.begin(), value.end(), [](unsigned char ch) { return ch >= 0x21U && ch <= 0x7EU; });
}
}

bool FarmCommerceEntitlementLedger::Initialize(FarmWorldTool& world, std::string configuredPlayerId, ReceiptVerifier verifier) {
    world_ = nullptr; configuredPlayerId_.clear(); verifier_ = {}; accepted_.clear(); auditLog_.clear(); nextAuditSequence_ = 1U; lastAudit_ = {}; hasLastAudit_ = false; lastError_ = FarmCommerceError::None; initialized_ = false;
    if (!world.IsReady() || !ValidText(configuredPlayerId, 1U) || !verifier) { lastError_ = FarmCommerceError::InvalidConfiguration; return false; }
    world_ = &world; configuredPlayerId_ = std::move(configuredPlayerId); verifier_ = std::move(verifier); accepted_.reserve(kMaxAcceptedReceipts); auditLog_.reserve(kMaxAuditReceipts); initialized_ = true;
    return true;
}

bool FarmCommerceEntitlementLedger::Apply(const FarmProviderReceipt& receipt, FarmCommerceAuditReceipt& audit) {
    audit = {};
    if (!initialized_ || world_ == nullptr) return Fail(FarmCommerceError::NotInitialized, receipt, audit);
    if (receipt.providerReceiptId == 0U || !ValidText(receipt.playerId, 1U) || !ValidText(receipt.authorityPayload, 1U) || receipt.entitlementCoins <= 0) return Fail(FarmCommerceError::InvalidReceipt, receipt, audit);
    if (receipt.playerId != configuredPlayerId_) return Fail(FarmCommerceError::WrongPlayer, receipt, audit);
    if (receipt.reversed) return Fail(FarmCommerceError::Reversed, receipt, audit);
    if (Find(receipt.providerReceiptId) != nullptr) return Fail(FarmCommerceError::Duplicate, receipt, audit);
    if (accepted_.size() >= kMaxAcceptedReceipts || auditLog_.size() >= kMaxAuditReceipts || nextAuditSequence_ == 0U) return Fail(FarmCommerceError::Capacity, receipt, audit);
    if (!verifier_(receipt)) return Fail(FarmCommerceError::VerifierRejected, receipt, audit);
    if (!world_->PlayerApplyVerifiedTopUp({receipt.providerReceiptId, receipt.entitlementCoins, receipt.authorityPayload})) return Fail(FarmCommerceError::ApplyRejected, receipt, audit);
    accepted_.push_back({receipt.providerReceiptId, receipt.entitlementCoins});
    return Audit(FarmCommerceAuditKind::Approved, FarmCommerceError::None, receipt.providerReceiptId, receipt.entitlementCoins, audit);
}

bool FarmCommerceEntitlementLedger::Reconcile(uint64_t providerReceiptId, int64_t expectedCoins, FarmCommerceAuditReceipt& audit) {
    audit = {};
    const FarmProviderReceipt input{providerReceiptId, configuredPlayerId_, expectedCoins, "reconcile", false};
    if (!initialized_ || world_ == nullptr) return Fail(FarmCommerceError::NotInitialized, input, audit);
    const AcceptedReceipt* accepted = Find(providerReceiptId);
    if (providerReceiptId == 0U || expectedCoins <= 0 || accepted == nullptr || accepted->entitlementCoins != expectedCoins) return Fail(FarmCommerceError::ReconciliationMismatch, input, audit);
    return Audit(FarmCommerceAuditKind::Approved, FarmCommerceError::None, providerReceiptId, expectedCoins, audit);
}

bool FarmCommerceEntitlementLedger::Fail(FarmCommerceError error, const FarmProviderReceipt& receipt, FarmCommerceAuditReceipt& audit) {
    lastError_ = error;
    if (initialized_ && auditLog_.size() < kMaxAuditReceipts && nextAuditSequence_ != 0U) Audit(FarmCommerceAuditKind::Rejected, error, receipt.providerReceiptId, receipt.entitlementCoins, audit);
    return false;
}

bool FarmCommerceEntitlementLedger::Audit(FarmCommerceAuditKind kind, FarmCommerceError error, uint64_t providerReceiptId, int64_t coins, FarmCommerceAuditReceipt& audit) {
    if (auditLog_.size() >= kMaxAuditReceipts || nextAuditSequence_ == 0U) { lastError_ = FarmCommerceError::Capacity; return false; }
    const uint64_t stateRevision = world_ == nullptr ? 0U : world_->Snapshot().simulationTick;
    const FarmCommerceAuditReceipt candidate{FarmCommerceAuditReceipt::kVersion, nextAuditSequence_++, kind, error, providerReceiptId, coins, stateRevision};
    auditLog_.push_back(candidate); audit = candidate; lastAudit_ = candidate; hasLastAudit_ = true; lastError_ = error; return error == FarmCommerceError::None;
}

const FarmCommerceEntitlementLedger::AcceptedReceipt* FarmCommerceEntitlementLedger::Find(uint64_t providerReceiptId) const {
    const auto found = std::find_if(accepted_.begin(), accepted_.end(), [providerReceiptId](const AcceptedReceipt& entry) { return entry.providerReceiptId == providerReceiptId; });
    return found == accepted_.end() ? nullptr : &*found;
}

} // namespace NeoEngine
