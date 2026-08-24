#include "Systems/GameLoanBook.h"

#include <cstdio>

int main() {
    using namespace NeoEngine; GameLoanBook loans; int64_t due=0;
    if(!loans.Open({"barn-loan",1000,1200,2,100})||!loans.NextDue("barn-loan",due)||due!=510||loans.PayNextTerm("barn-loan",509)||loans.LastError()!=GameLoanError::InvalidPayment||!loans.PayNextTerm("barn-loan",510)||!loans.NextDue("barn-loan",due)||due!=510||!loans.PayNextTerm("barn-loan",510)) return 1;
    const GameLoanSnapshot* closed=loans.Find("barn-loan");if(!closed||!closed->closed||closed->outstandingCoins!=0||loans.PayNextTerm("barn-loan",1)||loans.LastError()!=GameLoanError::ClosedLoan||loans.Open({"underwater",1000,999,2,0})||loans.LastError()!=GameLoanError::InsufficientCollateral||loans.Open({"long",1000,1000,101,0})||loans.LastError()!=GameLoanError::InvalidTerms) return 1;
    std::printf("GAME_LOAN_BOOK_SMOKE_OK terms=2 due=510 collateral=1 closed=1 bounds=1\n");return 0;
}
