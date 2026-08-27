#pragma once

#include "FarmSystem.h"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace NeoEngine {

class FarmWorldTool;

enum class FarmCommerceError : uint8_t { None, NotInitialized, InvalidConfiguration, InvalidReceipt, WrongPlayer, VerifierRejected, Duplicate, Reversed, ApplyRejected, Capacity, ReconciliationMismatch };
enum class FarmCommerceAuditKind : uint8_t { Approved, Rejected }; 

struct FarmProviderReceipt {
    uint64_t providerReceiptId = 0U;
    std::string playerId;
    int64_t entitlementCoins = 0;
    std::string authorityPayload;
    bool reversed = false;
};

struct FarmCommerceAuditReceipt {
    static constexpr uint16_t kVersion = 1U;
    uint16_t version = kVersion;
    uint64_t sequence = 0U;
    FarmCommerceAuditKind kind = FarmCommerceAuditKind::Rejected;
    FarmCommerceError error = FarmCommerceError::None;
    uint64_t providerReceiptId = 0U;
    int64_t entitlementCoins = 0;
    uint64_t worldSimulationTick = 0U;
};

class FarmCommerceEntitlementLedger {
public:
    static constexpr uint16_t kMaxAcceptedReceipts = 1024U;
    static constexpr uint16_t kMaxAuditReceipts = 256U;
    using ReceiptVerifier = std::function<bool(const FarmProviderReceipt&)>;

    bool Initialize(FarmWorldTool& world, std::string configuredPlayerId, ReceiptVerifier verifier);
    bool Apply(const FarmProviderReceipt& receipt, FarmCommerceAuditReceipt& audit);
    bool Reconcile(uint64_t providerReceiptId, int64_t expectedCoins, FarmCommerceAuditReceipt& audit);

    [[nodiscard]] bool IsReady() const { return initialized_; }
    [[nodiscard]] FarmCommerceError LastError() const { return lastError_; }
    [[nodiscard]] uint32_t AcceptedReceiptCount() const { return static_cast<uint32_t>(accepted_.size()); }
    [[nodiscard]] const FarmCommerceAuditReceipt* LastAudit() const { return hasLastAudit_ ? &lastAudit_ : nullptr; }

private:
    struct AcceptedReceipt { uint64_t providerReceiptId = 0U; int64_t entitlementCoins = 0; };
    bool Fail(FarmCommerceError error, const FarmProviderReceipt& receipt, FarmCommerceAuditReceipt& audit);
    bool Audit(FarmCommerceAuditKind kind, FarmCommerceError error, uint64_t providerReceiptId, int64_t coins, FarmCommerceAuditReceipt& audit);
    const AcceptedReceipt* Find(uint64_t providerReceiptId) const;

    FarmWorldTool* world_ = nullptr;
    std::string configuredPlayerId_;
    ReceiptVerifier verifier_;
    std::vector<AcceptedReceipt> accepted_;
    std::vector<FarmCommerceAuditReceipt> auditLog_;
    uint64_t nextAuditSequence_ = 1U;
    FarmCommerceAuditReceipt lastAudit_{};
    bool hasLastAudit_ = false;
    FarmCommerceError lastError_ = FarmCommerceError::None;
    bool initialized_ = false;
};

} // namespace NeoEngine
