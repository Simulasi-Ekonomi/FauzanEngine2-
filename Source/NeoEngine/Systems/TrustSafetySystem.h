#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
namespace NeoEngine { enum class FraudSignal:uint8_t{DuplicateReceipt,LedgerMismatch,ImpossibleInventory}; struct BanRecord{std::string playerId;std::string eventId;FraudSignal signal;uint8_t score;}; class TrustSafetySystem{public: bool Report(std::string playerId,std::string eventId,FraudSignal signal);bool IsBanned(const std::string& playerId)const;uint8_t Score(const std::string& playerId)const;const std::vector<BanRecord>& Audit()const{return m_Audit;}uint64_t DeterministicState()const;private:static bool Valid(const std::string& id);static uint8_t Weight(FraudSignal signal);std::unordered_set<std::string>m_Events,m_Banned;std::unordered_map<std::string,uint8_t>m_Scores;std::vector<BanRecord>m_Audit;}; }
