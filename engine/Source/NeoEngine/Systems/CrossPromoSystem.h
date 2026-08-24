#pragma once
#include <vector>
#include <string>
#include <functional>

namespace NeoEngine {

struct CrossPromoOffer {
    std::string offerId;
    std::string targetGameName;
    std::string targetGameId;
    std::string description;
    std::string rewardInTarget; // hadiah di game tujuan
    std::string rewardInCurrent; // hadiah di game ini
    int durationDays = 7;
    bool claimed = false;
};

class CrossPromoSystem {
private:
    std::vector<CrossPromoOffer> m_Offers;
    std::function<void(const CrossPromoOffer&)> m_OnClaim;
    
public:
    void AddOffer(const std::string& gameName, const std::string& gameId, 
                  const std::string& rewardHere, const std::string& rewardThere) {
        m_Offers.push_back({"cp_"+std::to_string(m_Offers.size()), gameName, gameId, 
                           "Play " + gameName + " and get rewards!", rewardThere, rewardHere, 7, false});
    }
    
    bool ClaimOffer(const std::string& offerId) {
        for (auto& o : m_Offers) {
            if (o.offerId == offerId && !o.claimed) {
                o.claimed = true;
                if (m_OnClaim) m_OnClaim(o);
                return true;
            }
        }
        return false;
    }
    
    std::vector<CrossPromoOffer> GetActiveOffers() const {
        std::vector<CrossPromoOffer> active;
        for (auto& o : m_Offers) if (!o.claimed) active.push_back(o);
        return active;
    }
    
    void SetOnClaim(std::function<void(const CrossPromoOffer&)> cb) { m_OnClaim = cb; }
};

} // namespace NeoEngine
