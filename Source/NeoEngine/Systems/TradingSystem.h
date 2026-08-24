#pragma once
#include <vector>
#include <string>
#include <functional>

namespace NeoEngine {
struct TradeOffer { std::string offerId, fromId, toId; std::vector<std::string> offerItems; std::vector<int> offerCounts; std::vector<std::string> requestItems; std::vector<int> requestCounts; bool accepted=false, cancelled=false; int goldOffer=0, goldRequest=0; };
class TradingSystem {
private:
    std::vector<TradeOffer> m_Offers;
    std::function<void(const TradeOffer&)> m_OnTradeProposed;
    std::function<void(const TradeOffer&)> m_OnTradeAccepted;
public:
    const std::string& ProposeTrade(const std::string& from, const std::string& to, const std::vector<std::string>& offer, const std::vector<int>& offerCount, const std::vector<std::string>& request, const std::vector<int>& requestCount, int goldOffer=0, int goldReq=0) {
        m_Offers.push_back({"trade_"+std::to_string(m_Offers.size()), from, to, offer, offerCount, request, requestCount, false, false, goldOffer, goldReq});
        if (m_OnTradeProposed) m_OnTradeProposed(m_Offers.back()); return m_Offers.back().offerId;
    }
    bool AcceptTrade(const std::string& offerId) {
        for (auto& o : m_Offers) { if (o.offerId == offerId && !o.accepted) { o.accepted = true; if (m_OnTradeAccepted) m_OnTradeAccepted(o); return true; } }
        return false;
    }
    bool CancelTrade(const std::string& offerId) { for (auto& o : m_Offers) if (o.offerId == offerId && !o.accepted) { o.cancelled = true; return true; } return false; }
    std::vector<TradeOffer> GetPendingOffers(const std::string& playerId) const {
        std::vector<TradeOffer> pending; for (auto& o : m_Offers) if ((o.toId == playerId || o.fromId == playerId) && !o.accepted && !o.cancelled) pending.push_back(o); return pending;
    }
    void SetOnTradeProposed(std::function<void(const TradeOffer&)> cb) { m_OnTradeProposed = cb; }
    void SetOnTradeAccepted(std::function<void(const TradeOffer&)> cb) { m_OnTradeAccepted = cb; }
};
}
