#pragma once
#include <array>
#include <cstdint>
namespace NeoEngine { enum class MatchThreeError:uint8_t{None,InvalidCoordinate,NotAdjacent,NoMatch}; class MatchThreeGame { public: MatchThreeGame(); bool Swap(uint8_t r1,uint8_t c1,uint8_t r2,uint8_t c2); uint32_t Resolve(); uint32_t Score()const{return m_Score;} MatchThreeError LastError()const{return m_Error;} uint64_t DeterministicState()const; private: bool Valid(uint8_t r,uint8_t c)const{return r<5&&c<5;} bool HasMatch()const; std::array<uint8_t,25> m_Board{};uint32_t m_Score=0,m_Turn=0;MatchThreeError m_Error=MatchThreeError::None;}; }
