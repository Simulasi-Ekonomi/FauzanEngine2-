#pragma once

#include <cstdint>
#include <unordered_map>

namespace NeoEngine {

struct TransactionRecord final {
    uint64_t id = 0U;
    int64_t delta = 0;
    int64_t balanceAfter = 0;
};

class TransactionJournal final {
public:
    explicit TransactionJournal(int64_t initialBalance = 0, size_t capacity = 8192U) : m_Balance(initialBalance), m_Capacity(capacity) {}

    bool Apply(uint64_t transactionId, int64_t delta, TransactionRecord& record) {
        if (transactionId == 0U || m_Capacity == 0U) return false;
        auto existing = m_Records.find(transactionId);
        if (existing != m_Records.end()) { record = existing->second; return existing->second.delta == delta; }
        if (m_Records.size() >= m_Capacity) return false;
        if ((delta > 0 && m_Balance > INT64_MAX - delta) || (delta < 0 && m_Balance < INT64_MIN - delta)) return false;
        m_Balance += delta;
        record = {transactionId, delta, m_Balance};
        m_Records.emplace(transactionId, record);
        return true;
    }

    int64_t Balance() const { return m_Balance; }
    size_t Size() const { return m_Records.size(); }
    void Clear() { m_Records.clear(); }

private:
    int64_t m_Balance;
    size_t m_Capacity;
    std::unordered_map<uint64_t, TransactionRecord> m_Records;
};

} // namespace NeoEngine
