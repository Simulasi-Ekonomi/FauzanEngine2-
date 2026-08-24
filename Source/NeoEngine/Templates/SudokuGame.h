#pragma once
#include <array>
#include <cstdint>
#include <string>

namespace NeoEngine {
enum class SudokuError : uint8_t { None, InvalidCoordinate, InvalidValue, GivenCell, Conflict, CorruptState };
class SudokuGame {
public:
    SudokuGame();
    bool Place(uint8_t row, uint8_t column, uint8_t value);
    bool Clear(uint8_t row, uint8_t column);
    uint8_t Cell(uint8_t row, uint8_t column) const;
    bool IsGiven(uint8_t row, uint8_t column) const;
    bool IsComplete() const;
    SudokuError LastError() const { return m_LastError; }
    std::string Serialize() const;
    bool Deserialize(const std::string& state);
private:
    bool Valid(uint8_t row, uint8_t column) const { return row < 9 && column < 9; }
    bool Conflicts(uint8_t row, uint8_t column, uint8_t value) const;
    std::array<uint8_t, 81> m_Board{};
    std::array<uint8_t, 81> m_Givens{};
    std::array<uint8_t, 81> m_Solution{};
    SudokuError m_LastError = SudokuError::None;
};
} // namespace NeoEngine
