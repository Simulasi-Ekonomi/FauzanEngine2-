#pragma once
#include <string>
#include <cstdlib>
#include <vector>

namespace NeoEngine {

class CasinoSystem {
private:
    int m_PlayerGold = 1000;
    int m_Jackpot = 100000;
    int m_TotalBets = 0;
    int m_TotalWins = 0;
    
public:
    int PlaySlots(int bet){
        if(bet > m_PlayerGold || bet < 1) return -1;
        m_PlayerGold -= bet; m_TotalBets++;
        int r = rand() % 100;
        if(r < 1){ int win = m_Jackpot; m_PlayerGold += win; m_Jackpot = 50000; m_TotalWins++; return win; } // Jackpot 1%
        else if(r < 10){ int win = bet * 5; m_PlayerGold += win; m_Jackpot += bet; m_TotalWins++; return win; } // Big win 9%
        else if(r < 30){ int win = bet * 2; m_PlayerGold += win; m_Jackpot += bet; m_TotalWins++; return win; } // Small win 20%
        else if(r < 45){ m_PlayerGold += bet; return 0; } // Push 15%
        else { m_Jackpot += bet; return 0; } // Lose 55%
    }
    
    int PlayRoulette(int bet, int number){
        if(bet > m_PlayerGold || bet < 1 || number < 0 || number > 36) return -1;
        m_PlayerGold -= bet; m_TotalBets++;
        int result = rand() % 37;
        if(result == number){ int win = bet * 35; m_PlayerGold += win; m_TotalWins++; return win; }
        return 0;
    }
    
    int PlayBlackjack(int bet){
        if(bet > m_PlayerGold || bet < 1) return -1;
        m_PlayerGold -= bet; m_TotalBets++;
        int player = (rand() % 11) + 12; int dealer = (rand() % 11) + 12;
        if(player > 21){ return 0; }
        else if(dealer > 21 || player > dealer){ m_PlayerGold += bet*2; m_TotalWins++; return bet*2; }
        else if(player == dealer){ m_PlayerGold += bet; return 0; }
        return 0;
    }
    
    int GetGold() const { return m_PlayerGold; }
    int GetJackpot() const { return m_Jackpot; }
    float GetWinRate() const { return m_TotalBets > 0 ? (float)m_TotalWins/m_TotalBets*100 : 0; }
    void AddGold(int amt){ m_PlayerGold += amt; }
};

} // namespace NeoEngine
