#pragma once
#include <vector>
#include <string>
#include <functional>

namespace NeoEngine {

enum class CutsceneAction { MoveCamera, ShowDialogue, PlayAnimation, PlaySound, ShowImage, FadeIn, FadeOut, Wait, SpawnActor };

struct CutsceneStep {
    CutsceneAction action;
    std::string param1, param2;
    float duration = 1.0f;
    float delay = 0;
};

struct Cutscene {
    std::string id, name;
    std::vector<CutsceneStep> steps;
    bool skippable = true;
    bool replayable = true;
    float totalDuration = 0;
};

class CutsceneSystem {
private:
    std::vector<Cutscene> m_Cutscenes;
    Cutscene* m_ActiveCutscene = nullptr;
    int m_CurrentStep = -1;
    float m_StepTimer = 0;
    bool m_Playing = false;
    std::function<void(const Cutscene&)> m_OnStart;
    std::function<void(const Cutscene&)> m_OnStep;
    std::function<void(const Cutscene&)> m_OnComplete;
    
public:
    Cutscene* CreateCutscene(const std::string& id, const std::string& name, bool skippable = true) {
        m_Cutscenes.push_back({id, name, {}, skippable, true, 0});
        return &m_Cutscenes.back();
    }
    
    void AddStep(Cutscene* c, CutsceneAction action, const std::string& p1 = "", const std::string& p2 = "", float duration = 1.0f, float delay = 0) {
        if (!c) return;
        c->steps.push_back({action, p1, p2, duration, delay});
        c->totalDuration += duration + delay;
    }
    
    void Play(const std::string& cutsceneId) {
        for (auto& c : m_Cutscenes) {
            if (c.id == cutsceneId) {
                m_ActiveCutscene = &c;
                m_CurrentStep = -1;
                m_StepTimer = 0;
                m_Playing = true;
                if (m_OnStart) m_OnStart(c);
                return;
            }
        }
    }
    
    void Update(float dt) {
        if (!m_Playing || !m_ActiveCutscene) return;
        if (m_CurrentStep < 0) { m_CurrentStep = 0; m_StepTimer = 0; return; }
        if (m_CurrentStep >= m_ActiveCutscene->steps.size()) { m_Playing = false; if (m_OnComplete) m_OnComplete(*m_ActiveCutscene); return; }
        auto& step = m_ActiveCutscene->steps[m_CurrentStep];
        if (m_StepTimer < step.delay) { m_StepTimer += dt; return; }
        if (m_StepTimer >= step.delay && m_StepTimer < step.delay + step.duration) {
            if (m_StepTimer == 0 || m_StepTimer < dt) { if (m_OnStep) m_OnStep(*m_ActiveCutscene); }
            m_StepTimer += dt;
            if (m_StepTimer >= step.delay + step.duration) { m_CurrentStep++; m_StepTimer = 0; }
        }
    }
    
    void Skip() {
        if (!m_ActiveCutscene || !m_ActiveCutscene->skippable) return;
        m_Playing = false;
        if (m_OnComplete) m_OnComplete(*m_ActiveCutscene);
    }
    
    bool IsPlaying() const { return m_Playing; }
    void SetOnStart(std::function<void(const Cutscene&)> cb) { m_OnStart = cb; }
    void SetOnStep(std::function<void(const Cutscene&)> cb) { m_OnStep = cb; }
    void SetOnComplete(std::function<void(const Cutscene&)> cb) { m_OnComplete = cb; }
};

} // namespace NeoEngine
