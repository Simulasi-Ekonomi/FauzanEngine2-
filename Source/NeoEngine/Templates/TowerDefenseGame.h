#pragma once
#include <cstdint>
#include <vector>
namespace NeoEngine {
enum class TowerDefenseError : uint8_t { None, InvalidLane, InsufficientGold, TowerLimit, WaveActive, InvalidWave, InvalidState };
struct TowerDefenseSnapshot { int32_t gold; int32_t lives; uint32_t towers; uint32_t enemies; uint32_t wave; bool waveActive; };
class TowerDefenseGame { public: bool PlaceTower(uint8_t lane); bool StartWave(uint8_t enemies); bool Tick(); TowerDefenseSnapshot Snapshot() const; TowerDefenseError LastError()const{return m_LastError;} uint64_t DeterministicState()const; private: struct Tower{uint8_t lane;}; struct Enemy{uint8_t lane; int16_t health; uint8_t progress;}; bool Fail(TowerDefenseError error){m_LastError=error;return false;} std::vector<Tower> m_Towers;std::vector<Enemy> m_Enemies;int32_t m_Gold=100,m_Lives=10;uint32_t m_Wave=0;TowerDefenseError m_LastError=TowerDefenseError::None;};
} // namespace NeoEngine
