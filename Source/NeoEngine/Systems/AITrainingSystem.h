#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>
#include <cstdlib>
#include <chrono>

namespace NeoEngine {

struct AITrainingData {
    std::string input;
    std::string output;
    float weight = 1.0f;
    int timesUsed = 0;
    bool validated = false;
};

struct AIModel {
    std::string name, purpose;
    std::unordered_map<std::string, std::string> patterns;
    int trainingIterations = 0;
    float confidence = 0;
    bool trained = false;
};

class AITrainingSystem {
private:
    std::vector<AITrainingData> m_TrainingData;
    std::vector<AIModel> m_Models;
    std::function<void(const AIModel&)> m_OnModelTrained;

public:
    AIModel* CreateModel(const std::string& name, const std::string& purpose) {
        m_Models.push_back({name, purpose}); return &m_Models.back();
    }

    void AddTrainingData(const std::string& input, const std::string& output, float weight=1.0f) {
        m_TrainingData.push_back({input, output, weight, 0, false});
    }

    void TrainModel(const std::string& modelName, int iterations=100) {
        for(auto& model: m_Models) {
            if(model.name != modelName) continue;
            for(int i=0; i<iterations; i++) {
                for(auto& data: m_TrainingData) {
                    if(!data.validated) continue;
                    model.patterns[data.input] = data.output;
                    model.trainingIterations++;
                    data.timesUsed++;
                }
            }
            model.confidence = std::min(1.0f, model.trainingIterations / 1000.0f);
            model.trained = true;
            if(m_OnModelTrained) m_OnModelTrained(model);
        }
    }

    std::string Predict(const std::string& modelName, const std::string& input) {
        for(auto& model: m_Models) {
            if(model.name == modelName) {
                auto it = model.patterns.find(input);
                if(it != model.patterns.end()) return it->second;
                // Cari yang paling mirip (simple string matching)
                int bestScore=0; std::string best;
                for(auto& [k,v]: model.patterns) {
                    int score=0; for(size_t i=0;i<std::min(k.size(),input.size());i++) if(k[i]==input[i])score++;
                    if(score>bestScore){bestScore=score;best=v;}
                }
                return best;
            }
        }
        return "";
    }

    bool ValidateData(int index) { if(index>=0&&index<m_TrainingData.size()){m_TrainingData[index].validated=true;return true;} return false; }
    void SetOnModelTrained(std::function<void(const AIModel&)> cb) { m_OnModelTrained = cb; }
};

} // namespace NeoEngine
