#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace NeoEngine {
enum class GameLoanError : uint8_t { None, InvalidId, DuplicateId, InvalidTerms, InvalidPrincipal, InsufficientCollateral, InvalidRate, Capacity, MissingLoan, ClosedLoan, InvalidPayment, Overflow };
struct GameLoanOffer { std::string id; int64_t principalCoins = 0; int64_t collateralValueCoins = 0; uint8_t termCount = 0; uint16_t interestBasisPointsPerTerm = 0; };
struct GameLoanSnapshot { GameLoanOffer offer{}; uint8_t paidTerms = 0; int64_t outstandingCoins = 0; bool closed = false; };
class GameLoanBook {
public:
    static constexpr uint8_t kMaxLoans = 128;
    bool Open(const GameLoanOffer& offer);
    bool NextDue(std::string_view id, int64_t& dueCoins) const;
    bool PayNextTerm(std::string_view id, int64_t paymentCoins);
    [[nodiscard]] const GameLoanSnapshot* Find(std::string_view id) const;
    [[nodiscard]] GameLoanError LastError() const { return lastError_; }
private:
    std::vector<GameLoanSnapshot> loans_;
    mutable GameLoanError lastError_ = GameLoanError::None;
};
} // namespace NeoEngine
