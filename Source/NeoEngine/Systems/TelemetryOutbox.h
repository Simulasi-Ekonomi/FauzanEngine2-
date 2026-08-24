#pragma once
#include <cstdint>
#include <string>
#include <vector>
namespace NeoEngine { struct TelemetryEnvelope{std::string id;std::string json;}; class TelemetryOutbox{public: static constexpr size_t kMaxEnvelopes=128,kMaxEnvelopeBytes=16384; bool Enqueue(std::string id,std::string json);bool Acknowledge(const std::string& id);const std::vector<TelemetryEnvelope>& Pending()const{return m_Pending;}std::vector<uint8_t> Serialize()const;bool Deserialize(const std::vector<uint8_t>& bytes);private:static bool ValidId(const std::string& id);std::vector<TelemetryEnvelope>m_Pending;}; }
