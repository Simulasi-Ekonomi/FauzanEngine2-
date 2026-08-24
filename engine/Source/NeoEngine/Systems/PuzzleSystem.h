#pragma once
#include <vector>
#include <string>
#include <cstdlib>
#include <functional>

namespace NeoEngine {
struct PuzzlePiece { int id, currentX, currentY, targetX, targetY; bool placed=false; };
struct Puzzle { std::string name; std::vector<PuzzlePiece> pieces; int size=3; bool solved=false; };
class PuzzleSystem {
private:
    std::vector<Puzzle> m_Puzzles; Puzzle* m_ActivePuzzle=nullptr;
    std::function<void(const Puzzle&)> m_OnSolved;
public:
    Puzzle* CreatePuzzle(const std::string& name, int size=3) {
        m_Puzzles.push_back({name}); auto& p = m_Puzzles.back(); p.size = size;
        int count = size * size;
        for (int i = 0; i < count; i++) { int tx = i % size, ty = i / size; int cx = rand() % size, cy = rand() % size; p.pieces.push_back({i, cx, cy, tx, ty, (cx==tx&&cy==ty)}); }
        return &p;
    }
    bool SwapPieces(Puzzle* p, int x1, int y1, int x2, int y2) {
        if (!p || p->solved) return false;
        PuzzlePiece *a=nullptr, *b=nullptr;
        for (auto& piece : p->pieces) { if (piece.currentX == x1 && piece.currentY == y1) a = &piece; if (piece.currentX == x2 && piece.currentY == y2) b = &piece; }
        if (!a || !b) return false;
        std::swap(a->currentX, b->currentX); std::swap(a->currentY, b->currentY);
        a->placed = (a->currentX == a->targetX && a->currentY == a->targetY);
        b->placed = (b->currentX == b->targetX && b->currentY == b->targetY);
        p->solved = true;
        for (auto& piece : p->pieces) if (!piece.placed) { p->solved = false; break; }
        if (p->solved && m_OnSolved) m_OnSolved(*p);
        return true;
    }
    void SetOnSolved(std::function<void(const Puzzle&)> cb) { m_OnSolved = cb; }
};
}
