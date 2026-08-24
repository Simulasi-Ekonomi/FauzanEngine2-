#pragma once
#include <vector>
#include <string>
#include <cstdlib>
#include <functional>

namespace NeoEngine {

enum class GemType { Red, Blue, Green, Yellow, Purple, Orange };
enum class Match3Action { None, Swap, Match, Cascade, Special };

struct Gem {
    GemType type;
    int x, y;
    bool matched = false;
    bool special = false;
    GemType specialType;
};

struct Match3Result {
    Match3Action action;
    int score = 0;
    int combos = 0;
    std::vector<std::string> effects;
};

class Match3System {
private:
    std::vector<std::vector<Gem>> m_Board;
    int m_Width = 8, m_Height = 8;
    int m_Score = 0;
    int m_MovesLeft = 30;
    int m_TargetScore = 5000;
    bool m_GameOver = false;
    std::function<void(const Match3Result&)> m_OnMatch;

public:
    Match3System(int w = 8, int h = 8) : m_Width(w), m_Height(h) { GenerateBoard(); }

    void GenerateBoard() {
        m_Board.resize(m_Height, std::vector<Gem>(m_Width));
        for (int y = 0; y < m_Height; y++) {
            for (int x = 0; x < m_Width; x++) {
                m_Board[y][x].type = GemType(rand() % 6);
                m_Board[y][x].x = x; m_Board[y][x].y = y;
            }
        }
        // Remove initial matches
        while (FindMatches()) { FillBoard(); }
    }

    bool SwapGems(int x1, int y1, int x2, int y2) {
        if (abs(x1 - x2) + abs(y1 - y2) != 1) return false; // must be adjacent
        if (m_MovesLeft <= 0) return false;
        std::swap(m_Board[y1][x1], m_Board[y2][x2]);
        std::swap(m_Board[y1][x1].x, m_Board[y2][x2].x);
        std::swap(m_Board[y1][x1].y, m_Board[y2][x2].y);
        if (!FindMatches()) { // Invalid swap, revert
            std::swap(m_Board[y1][x1], m_Board[y2][x2]);
            std::swap(m_Board[y1][x1].x, m_Board[y2][x2].x);
            std::swap(m_Board[y1][x1].y, m_Board[y2][x2].y);
            return false;
        }
        m_MovesLeft--;
        ProcessMatches();
        return true;
    }

    bool FindMatches() {
        bool found = false;
        // Horizontal
        for (int y = 0; y < m_Height; y++) {
            for (int x = 0; x < m_Width - 2; x++) {
                if (m_Board[y][x].type == m_Board[y][x+1].type && m_Board[y][x].type == m_Board[y][x+2].type) {
                    m_Board[y][x].matched = m_Board[y][x+1].matched = m_Board[y][x+2].matched = true;
                    found = true;
                }
            }
        }
        // Vertical
        for (int x = 0; x < m_Width; x++) {
            for (int y = 0; y < m_Height - 2; y++) {
                if (m_Board[y][x].type == m_Board[y+1][x].type && m_Board[y][x].type == m_Board[y+2][x].type) {
                    m_Board[y][x].matched = m_Board[y+1][x].matched = m_Board[y+2][x].matched = true;
                    found = true;
                }
            }
        }
        return found;
    }

    void ProcessMatches() {
        int combo = 0;
        while (FindMatches()) {
            combo++;
            int matchScore = 0;
            for (int y = 0; y < m_Height; y++) {
                for (int x = 0; x < m_Width; x++) {
                    if (m_Board[y][x].matched) {
                        matchScore += 10;
                        m_Board[y][x].type = GemType(rand() % 6);
                        m_Board[y][x].matched = false;
                    }
                }
            }
            m_Score += matchScore * combo;
            if (m_OnMatch) m_OnMatch({Match3Action::Match, m_Score, combo, {}});
        }
        FillBoard();
        if (m_MovesLeft <= 0) m_GameOver = true;
    }

    void FillBoard() {
        for (int x = 0; x < m_Width; x++) {
            for (int y = m_Height - 1; y >= 0; y--) {
                if (m_Board[y][x].matched || m_Board[y][x].type == GemType(0)) {
                    // Shift down
                    for (int k = y; k > 0; k--) {
                        m_Board[k][x] = m_Board[k-1][x];
                    }
                    m_Board[0][x].type = GemType(rand() % 6);
                    m_Board[0][x].matched = false;
                }
            }
        }
    }

    int GetScore() const { return m_Score; }
    int GetMovesLeft() const { return m_MovesLeft; }
    bool IsGameOver() const { return m_GameOver; }
    const auto& GetBoard() const { return m_Board; }
    void SetOnMatch(std::function<void(const Match3Result&)> cb) { m_OnMatch = cb; }
};

} // namespace NeoEngine
