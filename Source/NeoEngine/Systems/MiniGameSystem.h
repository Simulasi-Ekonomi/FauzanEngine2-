#pragma once
#include <vector>
#include <string>
#include <cstdlib>
#include <functional>

namespace NeoEngine {

enum class MiniGameType { WhackMole, QuickTap, MemoryMatch, WordGuess, ColorSort, SpeedMath, DodgeObstacle, CoinCollect };

struct MiniGameResult {
    MiniGameType type;
    int score = 0;
    int highScore = 0;
    bool newRecord = false;
    float timeTaken = 0;
    int reward = 0;
};

class MiniGameSystem {
private:
    std::vector<int> m_HighScores;
    std::function<void(const MiniGameResult&)> m_OnComplete;
    
public:
    MiniGameSystem() { m_HighScores.resize(10, 0); }
    
    MiniGameResult PlayWhackMole(float timeLimit = 10.0f) {
        MiniGameResult res{MiniGameType::WhackMole, 0, m_HighScores[(int)MiniGameType::WhackMole], false, 0, 0};
        res.score = rand() % 30 + 10; // simulasi
        if (res.score > res.highScore) {
            res.newRecord = true;
            m_HighScores[(int)MiniGameType::WhackMole] = res.score;
        }
        res.reward = res.score * 2;
        if (m_OnComplete) m_OnComplete(res);
        return res;
    }
    
    MiniGameResult PlayDodgeObstacle() {
        MiniGameResult res{MiniGameType::DodgeObstacle, 0, m_HighScores[(int)MiniGameType::DodgeObstacle], false, 0, 0};
        res.score = rand() % 500 + 100;
        if (res.score > res.highScore) {
            res.newRecord = true;
            m_HighScores[(int)MiniGameType::DodgeObstacle] = res.score;
        }
        res.reward = res.score;
        if (m_OnComplete) m_OnComplete(res);
        return res;
    }
    
    MiniGameResult PlayCoinCollect(float timeLimit = 30.0f) {
        MiniGameResult res{MiniGameType::CoinCollect, 0, m_HighScores[(int)MiniGameType::CoinCollect], false, 0, 0};
        res.score = rand() % 100 + 50;
        if (res.score > res.highScore) {
            res.newRecord = true;
            m_HighScores[(int)MiniGameType::CoinCollect] = res.score;
        }
        res.reward = res.score * 3;
        if (m_OnComplete) m_OnComplete(res);
        return res;
    }
    
    int GetHighScore(MiniGameType type) const { return m_HighScores[(int)type]; }
    void SetOnComplete(std::function<void(const MiniGameResult&)> cb) { m_OnComplete = cb; }
};

} // namespace NeoEngine
