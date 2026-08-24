#include "Core/Math/NeoMath.h"
#pragma once
#include <vector>
#include <string>
#include <functional>

namespace NeoEngine {

enum class NoteType { Tap, Hold, Slide, Flick };
enum class JudgeResult { Perfect, Great, Good, Miss };

struct Note {
    NoteType type;
    float beatTime; // waktu dalam beat
    float duration = 0; // untuk hold notes
    int lane = 0; // 0-3 untuk 4 lane
    bool processed = false;
};

struct RhythmResult {
    JudgeResult judge;
    int score;
    int combo;
    bool feverActive;
};

class RhythmGameSystem {
private:
    std::vector<Note> m_Notes;
    std::vector<Note> m_HoldNotes; // hold notes yang sedang aktif
    float m_BeatPosition = 0;
    float m_BeatSpeed = 120.0f; // BPM
    int m_Score = 0;
    int m_Combo = 0;
    int m_MaxCombo = 0;
    int m_PerfectCount = 0;
    int m_GreatCount = 0;
    int m_GoodCount = 0;
    int m_MissCount = 0;
    int m_FeverGauge = 0;
    bool m_FeverActive = false;
    float m_FeverTimer = 0;
    float m_Health = 100;
    float m_MaxHealth = 100;
    std::function<void(const RhythmResult&)> m_OnHit;

public:
    void SetBPM(float bpm) { m_BeatSpeed = bpm; }
    void AddNote(NoteType type, float beatTime, int lane, float duration = 0) {
        m_Notes.push_back({type, beatTime, duration, lane, false});
    }

    void LoadChart(const std::vector<std::tuple<NoteType, float, int, float>>& chart) {
        m_Notes.clear();
        for (auto& [type, time, lane, dur] : chart) {
            m_Notes.push_back({type, time, dur, lane, false});
        }
    }

    void Update(float dt) {
        // Advance beat position
        m_BeatPosition += m_BeatSpeed / 60.0f * dt;

        // Auto-miss notes that passed the hit line
        for (auto& note : m_Notes) {
            if (!note.processed && note.beatTime < m_BeatPosition - 0.15f) {
                note.processed = true;
                m_MissCount++;
                m_Combo = 0;
                m_FeverGauge = std::max(0, m_FeverGauge - 10);
                m_Health -= 5;
            }
        }

        // Fever timer
        if (m_FeverActive) {
            m_FeverTimer -= dt;
            if (m_FeverTimer <= 0) m_FeverActive = false;
        }
    }

    RhythmResult Hit(int lane, float hitTime) {
        RhythmResult result{JudgeResult::Miss, 0, 0, m_FeverActive};

        // Temukan note terdekat di lane yang sama
        Note* closestNote = nullptr;
        float closestDist = 0.5f;
        for (auto& note : m_Notes) {
            if (note.processed || note.lane != lane) continue;
            float dist = NeoEngine::Math::Fabs(note.beatTime - hitTime);
            if (dist < closestDist) {
                closestDist = dist;
                closestNote = &note;
            }
        }

        if (closestNote) {
            closestNote->processed = true;
            if (closestDist < 0.03f) {
                result.judge = JudgeResult::Perfect;
                result.score = 100;
                m_PerfectCount++;
                m_FeverGauge += 5;
            } else if (closestDist < 0.07f) {
                result.judge = JudgeResult::Great;
                result.score = 80;
                m_GreatCount++;
                m_FeverGauge += 3;
            } else if (closestDist < 0.15f) {
                result.judge = JudgeResult::Good;
                result.score = 50;
                m_GoodCount++;
                m_FeverGauge += 1;
            } else {
                result.judge = JudgeResult::Miss;
                m_MissCount++;
                m_FeverGauge = std::max(0, m_FeverGauge - 10);
            }

            if (result.judge != JudgeResult::Miss) {
                m_Combo++;
                if (m_Combo > m_MaxCombo) m_MaxCombo = m_Combo;
                result.combo = m_Combo;
                int comboMul = 1 + (m_Combo / 50);
                result.score *= comboMul;
                m_Score += result.score;
                m_Health = std::min(m_MaxHealth, m_Health + 1);
            } else {
                m_Combo = 0;
                result.combo = 0;
                m_Health -= 5;
            }

            // Cek fever
            if (m_FeverGauge >= 100) {
                m_FeverActive = true;
                m_FeverGauge = 0;
                m_FeverTimer = 10.0f;
                result.feverActive = true;
            }
        } else {
            m_MissCount++;
            m_Combo = 0;
        }

        if (m_OnHit) m_OnHit(result);
        return result;
    }

    int GetScore() const { return m_Score; }
    int GetCombo() const { return m_Combo; }
    int GetMaxCombo() const { return m_MaxCombo; }
    float GetHealth() const { return m_Health; }
    bool IsFeverActive() const { return m_FeverActive; }
    int GetPerfectCount() const { return m_PerfectCount; }
    int GetGreatCount() const { return m_GreatCount; }
    int GetGoodCount() const { return m_GoodCount; }
    int GetMissCount() const { return m_MissCount; }

    void SetOnHit(std::function<void(const RhythmResult&)> cb) { m_OnHit = cb; }
};

} // namespace NeoEngine
