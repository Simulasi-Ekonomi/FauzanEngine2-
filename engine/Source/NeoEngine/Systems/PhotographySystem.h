#pragma once
#include <vector>
#include <string>
#include <functional>

namespace NeoEngine {
struct Photo { std::string id; float posX, posY, posZ; float rotX, rotY, rotZ; int score=0; std::string tags; bool shared=false; };
class PhotographySystem {
private:
    std::vector<Photo> m_Photos; int m_FilmCount=30;
    std::function<void(const Photo&)> m_OnPhotoTaken;
public:
    Photo* TakePhoto(float x, float y, float z, float rx, float ry, float rz) {
        if (m_FilmCount <= 0) return nullptr; m_FilmCount--;
        m_Photos.push_back({"p_"+std::to_string(m_Photos.size()), x, y, z, rx, ry, rz}); if (m_OnPhotoTaken) m_OnPhotoTaken(m_Photos.back()); return &m_Photos.back();
    }
    int RatePhoto(const std::string& id, const std::string& tags) {
        for (auto& p : m_Photos) { if (p.id == id) { p.tags = tags; p.score = (rand() % 5) + 1; return p.score; } } return 0;
    }
    int GetFilmCount() const { return m_FilmCount; }
    void BuyFilm(int count) { m_FilmCount += count; }
    const auto& GetPhotos() const { return m_Photos; }
    void SetOnPhotoTaken(std::function<void(const Photo&)> cb) { m_OnPhotoTaken = cb; }
};
}
