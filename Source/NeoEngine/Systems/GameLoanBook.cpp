#include "GameLoanBook.h"

#include <algorithm>
#include <cctype>
#include <limits>

namespace NeoEngine {
namespace {
bool ValidId(std::string_view id){return !id.empty()&&id.size()<=48U&&std::all_of(id.begin(),id.end(),[](unsigned char c){return std::isalnum(c)||c=='-'||c=='_';});}
bool TotalOwed(const GameLoanOffer& offer,int64_t& total){if(offer.principalCoins>1000000000LL||offer.interestBasisPointsPerTerm>5000U||offer.termCount==0U||offer.termCount>100U)return false;const int64_t factor=static_cast<int64_t>(offer.interestBasisPointsPerTerm)*offer.termCount;if(offer.principalCoins>std::numeric_limits<int64_t>::max()/factor&&factor!=0)return false;const int64_t product=offer.principalCoins*factor;const int64_t interest=(product+9999LL)/10000LL;if(offer.principalCoins>std::numeric_limits<int64_t>::max()-interest)return false;total=offer.principalCoins+interest;return true;}
}
const GameLoanSnapshot* GameLoanBook::Find(std::string_view id) const {const auto found=std::find_if(loans_.begin(),loans_.end(),[id](const GameLoanSnapshot& loan){return loan.offer.id==id;});return found==loans_.end()?nullptr:&*found;}
bool GameLoanBook::Open(const GameLoanOffer& offer){if(!ValidId(offer.id)){lastError_=GameLoanError::InvalidId;return false;}if(Find(offer.id)){lastError_=GameLoanError::DuplicateId;return false;}if(loans_.size()>=kMaxLoans){lastError_=GameLoanError::Capacity;return false;}if(offer.principalCoins<=0){lastError_=GameLoanError::InvalidPrincipal;return false;}if(offer.collateralValueCoins<offer.principalCoins){lastError_=GameLoanError::InsufficientCollateral;return false;}if(offer.termCount==0U||offer.termCount>100U){lastError_=GameLoanError::InvalidTerms;return false;}if(offer.interestBasisPointsPerTerm>5000U){lastError_=GameLoanError::InvalidRate;return false;}int64_t total=0;if(!TotalOwed(offer,total)){lastError_=GameLoanError::Overflow;return false;}loans_.push_back({offer,0U,total,false});lastError_=GameLoanError::None;return true;}
bool GameLoanBook::NextDue(std::string_view id,int64_t& dueCoins) const {const GameLoanSnapshot* loan=Find(id);if(!loan){lastError_=GameLoanError::MissingLoan;return false;}if(loan->closed){lastError_=GameLoanError::ClosedLoan;return false;}const uint8_t remainingTerms=static_cast<uint8_t>(loan->offer.termCount-loan->paidTerms);dueCoins=(loan->outstandingCoins+remainingTerms-1LL)/remainingTerms;lastError_=GameLoanError::None;return true;}
bool GameLoanBook::PayNextTerm(std::string_view id,int64_t paymentCoins){auto found=std::find_if(loans_.begin(),loans_.end(),[id](const GameLoanSnapshot& loan){return loan.offer.id==id;});if(found==loans_.end()){lastError_=GameLoanError::MissingLoan;return false;}if(found->closed){lastError_=GameLoanError::ClosedLoan;return false;}int64_t due=0;if(!NextDue(id,due)){return false;}if(paymentCoins!=due){lastError_=GameLoanError::InvalidPayment;return false;}found->outstandingCoins-=due;++found->paidTerms;if(found->paidTerms==found->offer.termCount){found->outstandingCoins=0;found->closed=true;}lastError_=GameLoanError::None;return true;}
} // namespace NeoEngine
