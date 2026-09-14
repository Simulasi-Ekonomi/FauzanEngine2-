#include "TowerDefenseGame.h"
#include <new>

namespace NeoEngine {

bool TowerDefenseGame::PlaceTower(uint8_t lane) {
    if (lane >= 3) return Fail(TowerDefenseError::InvalidLane);
    if (m_Gold < 25) return Fail(TowerDefenseError::InsufficientGold);
    if (m_Towers.size() >= 16) return Fail(TowerDefenseError::TowerLimit);

    try {
        m_Towers.push_back({lane});
    } catch (const std::bad_alloc&) {
        return Fail(TowerDefenseError::Capacity);
    }

    m_Gold -= 25;
    m_LastError = TowerDefenseError::None;
    return true;
}

bool TowerDefenseGame::StartWave(uint8_t count) {
    if (!m_Enemies.empty()) return Fail(TowerDefenseError::WaveActive);
    if (count == 0 || count > 32) return Fail(TowerDefenseError::InvalidWave);

    try {
        m_Enemies.reserve(m_Enemies.size() + count);
        for (uint8_t i = 0; i < count; ++i) {
            m_Enemies.push_back({static_cast<uint8_t>(i % 3), 6, 0});
        }
    } catch (const std::bad_alloc&) {
        return Fail(TowerDefenseError::Capacity);
    }

    ++m_Wave;
    m_LastError = TowerDefenseError::None;
    return true;
}

bool TowerDefenseGame::Tick() {
    if (m_Lives <= 0) return Fail(TowerDefenseError::InvalidState);

    try {
        std::vector<Enemy> damaged = m_Enemies;
        for (const Tower& tower : m_Towers) {
            for (Enemy& enemy : damaged) {
                if (enemy.lane == tower.lane && enemy.health > 0) {
                    enemy.health -= 2;
                    break;
                }
            }
        }

        std::vector<Enemy> survivors;
        survivors.reserve(damaged.size());
        int32_t nextGold = m_Gold;
        int32_t nextLives = m_Lives;

        for (Enemy enemy : damaged) {
            if (enemy.health <= 0) {
                nextGold += 5;
                continue;
            }
            if (++enemy.progress >= 10) {
                --nextLives;
                continue;
            }
            survivors.push_back(enemy);
        }

        m_Enemies = std::move(survivors);
        m_Gold = nextGold;
        m_Lives = nextLives;
    } catch (const std::bad_alloc&) {
        return Fail(TowerDefenseError::Capacity);
    }

    m_LastError = TowerDefenseError::None;
    return true;
}

TowerDefenseSnapshot TowerDefenseGame::Snapshot() const {
    return {m_Gold, m_Lives, static_cast<uint32_t>(m_Towers.size()), static_cast<uint32_t>(m_Enemies.size()), m_Wave, !m_Enemies.empty()};
}

uint64_t TowerDefenseGame::DeterministicState() const {
    const auto s = Snapshot();
    uint64_t hash = 1469598103934665603ULL;
    auto add = [&hash](uint64_t v) { hash ^= v; hash *= 1099511628211ULL; };
    add(static_cast<uint32_t>(s.gold));
    add(static_cast<uint32_t>(s.lives));
    add(s.towers);
    add(s.enemies);
    add(s.wave);
    for (const auto& t : m_Towers) add(t.lane);
    for (const auto& e : m_Enemies) {
        add(e.lane);
        add(static_cast<uint16_t>(e.health));
        add(e.progress);
    }
    return hash;
}

} // namespace NeoEngine
