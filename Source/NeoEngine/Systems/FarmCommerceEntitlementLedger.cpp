#include "FarmCommerceEntitlementLedger.h"

#include "FarmWorldTool.h"

#include <algorithm>
#include <limits>

namespace NeoEngine {
namespace {

constexpr uint32_t kStateMagic = 0x434D5246U; // FRMC
constexpr uint16_t kStateVersion = 1U;
constexpr uint64_t kHashOffset = 1469598103934665603ULL;
constexpr uint64_t kHashPrime = 1099511628211ULL;

template <typename T>
void Append(std::vector<uint8_t>& bytes, T value) {
    for (size_t index = 0U; index < sizeof(T); ++index) bytes.push_back(static_cast<uint8_t>((static_cast<uint64_t>(value) >> (index * 8U)) & 0xFFU));
}

template <typename T>
bool Read(std::span<const uint8_t> bytes, size_t& offset, T& value) {
    if (offset > bytes.size() || bytes.size() - offset < sizeof(T)) return false;
    uint64_t raw = 0U;
    for (size_t index = 0U; index < sizeof(T); ++index) raw |= static_cast<uint64_t>(bytes[offset + index]) << (index * 8U);
    value = static_cast<T>(raw);
    offset += sizeof(T);
    return true;
}

uint64_t Hash(std::span<const uint8_t> bytes) {
    uint64_t hash = kHashOffset;
    for (uint8_t byte : bytes) { hash ^= byte; hash *= kHashPrime; }
    return hash;
}

bool ValidText(const std::string& value, size_t minimum) {
    return value.size() >= minimum && value.size() <= 96U && std::all_of(value.begin(), value.end(), [](unsigned char ch) { return ch >= 0x21U && ch <= 0x7EU; });
}

bool SameAudit(const FarmCommerceAuditReceipt& left, const FarmCommerceAuditReceipt& right) {
    return left.version == right.version && left.sequence == right.sequence && left.kind == right.kind && left.error == right.error &&
           left.providerReceiptId == right.providerReceiptId && left.entitlementCoins == right.entitlementCoins && left.worldSimulationTick == right.worldSimulationTick;
}

bool ValidAudit(const FarmCommerceAuditReceipt& receipt) {
    const uint8_t kind = static_cast<uint8_t>(receipt.kind);
    const uint8_t error = static_cast<uint8_t>(receipt.error);
    return receipt.version == FarmCommerceAuditReceipt::kVersion && receipt.sequence != 0U && kind <= static_cast<uint8_t>(FarmCommerceAuditKind::Rejected) &&
           error <= static_cast<uint8_t>(FarmCommerceError::CorruptState) &&
           ((receipt.kind == FarmCommerceAuditKind::Approved && receipt.error == FarmCommerceError::None) ||
            (receipt.kind == FarmCommerceAuditKind::Rejected && receipt.error != FarmCommerceError::None));
}

void AppendAudit(std::vector<uint8_t>& bytes, const FarmCommerceAuditReceipt& receipt) {
    Append<uint16_t>(bytes, receipt.version);
    Append<uint64_t>(bytes, receipt.sequence);
    Append<uint8_t>(bytes, static_cast<uint8_t>(receipt.kind));
    Append<uint8_t>(bytes, static_cast<uint8_t>(receipt.error));
    Append<uint64_t>(bytes, receipt.providerReceiptId);
    Append<int64_t>(bytes, receipt.entitlementCoins);
    Append<uint64_t>(bytes, receipt.worldSimulationTick);
}

bool ReadAudit(std::span<const uint8_t> bytes, size_t& offset, FarmCommerceAuditReceipt& receipt) {
    uint8_t kind = 0U;
    uint8_t error = 0U;
    if (!Read<uint16_t>(bytes, offset, receipt.version) || !Read<uint64_t>(bytes, offset, receipt.sequence) ||
        !Read<uint8_t>(bytes, offset, kind) || !Read<uint8_t>(bytes, offset, error) ||
        !Read<uint64_t>(bytes, offset, receipt.providerReceiptId) || !Read<int64_t>(bytes, offset, receipt.entitlementCoins) ||
        !Read<uint64_t>(bytes, offset, receipt.worldSimulationTick)) return false;
    receipt.kind = static_cast<FarmCommerceAuditKind>(kind);
    receipt.error = static_cast<FarmCommerceError>(error);
    return ValidAudit(receipt);
}

} // namespace

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

std::vector<uint8_t> FarmCommerceEntitlementLedger::SerializeState() const {
    if (!initialized_ || world_ == nullptr || !ValidText(configuredPlayerId_, 1U) || accepted_.size() > kMaxAcceptedReceipts || auditLog_.size() > kMaxAuditReceipts || nextAuditSequence_ == 0U) return {};
    if ((auditLog_.empty() && hasLastAudit_) || (!auditLog_.empty() && (!hasLastAudit_ || !SameAudit(lastAudit_, auditLog_.back())))) return {};
    try {
        std::vector<uint8_t> bytes;
        bytes.reserve(32U + configuredPlayerId_.size() + accepted_.size() * 16U + auditLog_.size() * 36U + (hasLastAudit_ ? 36U : 0U));
        Append<uint32_t>(bytes, kStateMagic);
        Append<uint16_t>(bytes, kStateVersion);
        Append<uint16_t>(bytes, static_cast<uint16_t>(configuredPlayerId_.size()));
        Append<uint16_t>(bytes, static_cast<uint16_t>(accepted_.size()));
        Append<uint16_t>(bytes, static_cast<uint16_t>(auditLog_.size()));
        Append<uint64_t>(bytes, nextAuditSequence_);
        Append<uint8_t>(bytes, hasLastAudit_ ? 1U : 0U);
        bytes.insert(bytes.end(), configuredPlayerId_.begin(), configuredPlayerId_.end());
        for (const AcceptedReceipt& receipt : accepted_) {
            if (receipt.providerReceiptId == 0U || receipt.entitlementCoins <= 0) return {};
            Append<uint64_t>(bytes, receipt.providerReceiptId);
            Append<int64_t>(bytes, receipt.entitlementCoins);
        }
        uint64_t expectedSequence = 1U;
        for (const FarmCommerceAuditReceipt& receipt : auditLog_) {
            if (!ValidAudit(receipt) || receipt.sequence != expectedSequence || expectedSequence == std::numeric_limits<uint64_t>::max()) return {};
            AppendAudit(bytes, receipt);
            ++expectedSequence;
        }
        if (nextAuditSequence_ != expectedSequence) return {};
        if (hasLastAudit_) AppendAudit(bytes, lastAudit_);
        if (bytes.size() + sizeof(uint64_t) > kMaxSnapshotBytes) return {};
        Append<uint64_t>(bytes, Hash(bytes));
        return bytes;
    } catch (...) {
        return {};
    }
}

bool FarmCommerceEntitlementLedger::RestoreState(std::span<const uint8_t> bytes) {
    if (!initialized_ || world_ == nullptr) { lastError_ = FarmCommerceError::NotInitialized; return false; }
    if (bytes.size() < 30U || bytes.size() > kMaxSnapshotBytes) { lastError_ = FarmCommerceError::CorruptState; return false; }
    const size_t hashOffset = bytes.size() - sizeof(uint64_t);
    uint64_t expectedHash = 0U;
    size_t hashReadOffset = hashOffset;
    if (!Read<uint64_t>(bytes, hashReadOffset, expectedHash) || hashReadOffset != bytes.size() || Hash(bytes.first(hashOffset)) != expectedHash) { lastError_ = FarmCommerceError::CorruptState; return false; }
    size_t offset = 0U;
    uint32_t magic = 0U;
    uint16_t version = 0U;
    uint16_t playerLength = 0U;
    uint16_t acceptedCount = 0U;
    uint16_t auditCount = 0U;
    uint64_t nextSequence = 0U;
    uint8_t hasLastAudit = 0U;
    if (!Read<uint32_t>(bytes, offset, magic) || !Read<uint16_t>(bytes, offset, version) || !Read<uint16_t>(bytes, offset, playerLength) ||
        !Read<uint16_t>(bytes, offset, acceptedCount) || !Read<uint16_t>(bytes, offset, auditCount) || !Read<uint64_t>(bytes, offset, nextSequence) ||
        !Read<uint8_t>(bytes, offset, hasLastAudit) || magic != kStateMagic || version != kStateVersion || playerLength == 0U || playerLength > 96U ||
        acceptedCount > kMaxAcceptedReceipts || auditCount > kMaxAuditReceipts || nextSequence == 0U || hasLastAudit > 1U || offset + playerLength > hashOffset) {
        lastError_ = FarmCommerceError::CorruptState;
        return false;
    }
    try {
        std::string candidatePlayer(reinterpret_cast<const char*>(bytes.data() + offset), playerLength);
        offset += playerLength;
        if (!ValidText(candidatePlayer, 1U) || candidatePlayer != configuredPlayerId_) { lastError_ = FarmCommerceError::CorruptState; return false; }
        std::vector<AcceptedReceipt> candidateAccepted;
        std::vector<FarmCommerceAuditReceipt> candidateAuditLog;
        candidateAccepted.reserve(acceptedCount);
        candidateAuditLog.reserve(auditCount);
        for (uint16_t index = 0U; index < acceptedCount; ++index) {
            AcceptedReceipt receipt{};
            if (!Read<uint64_t>(bytes, offset, receipt.providerReceiptId) || !Read<int64_t>(bytes, offset, receipt.entitlementCoins) || receipt.providerReceiptId == 0U || receipt.entitlementCoins <= 0 ||
                std::any_of(candidateAccepted.begin(), candidateAccepted.end(), [&receipt](const AcceptedReceipt& value) { return value.providerReceiptId == receipt.providerReceiptId; })) { lastError_ = FarmCommerceError::CorruptState; return false; }
            candidateAccepted.push_back(receipt);
        }
        uint64_t expectedSequence = 1U;
        for (uint16_t index = 0U; index < auditCount; ++index) {
            FarmCommerceAuditReceipt receipt{};
            if (!ReadAudit(bytes, offset, receipt) || receipt.sequence != expectedSequence || expectedSequence == std::numeric_limits<uint64_t>::max()) { lastError_ = FarmCommerceError::CorruptState; return false; }
            candidateAuditLog.push_back(receipt);
            ++expectedSequence;
        }
        FarmCommerceAuditReceipt candidateLast{};
        if (hasLastAudit != 0U && (!ReadAudit(bytes, offset, candidateLast) || candidateAuditLog.empty() || !SameAudit(candidateLast, candidateAuditLog.back()))) { lastError_ = FarmCommerceError::CorruptState; return false; }
        if ((hasLastAudit == 0U && !candidateAuditLog.empty()) || nextSequence != expectedSequence || offset != hashOffset) { lastError_ = FarmCommerceError::CorruptState; return false; }
        accepted_ = std::move(candidateAccepted);
        auditLog_ = std::move(candidateAuditLog);
        nextAuditSequence_ = nextSequence;
        lastAudit_ = candidateLast;
        hasLastAudit_ = hasLastAudit != 0U;
        lastError_ = FarmCommerceError::None;
        return true;
    } catch (...) {
        lastError_ = FarmCommerceError::CorruptState;
        return false;
    }
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
