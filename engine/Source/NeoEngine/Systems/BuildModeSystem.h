#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>

namespace NeoEngine {

struct BuildObject {
    std::string id, type, name;
    float x,y,z, rx,ry,rz;
    int gridX=0, gridY=0, gridZ=0;
    std::string material;
    bool snapToGrid=true;
    int cost=0;
};

class BuildModeSystem {
private:
    std::vector<BuildObject> m_Objects;
    bool m_BuildModeActive=false;
    int m_GridSize=1;
    std::string m_SelectedType="wall";

public:
    void EnterBuildMode() { m_BuildModeActive=true; }
    void ExitBuildMode() { m_BuildModeActive=false; }
    bool IsInBuildMode() const { return m_BuildModeActive; }
    BuildObject* PlaceObject(const std::string& type, float x, float y, float z) {
        int gx=(int)(x/m_GridSize)*m_GridSize, gy=(int)(y/m_GridSize)*m_GridSize, gz=(int)(z/m_GridSize)*m_GridSize;
        m_Objects.push_back({"bld_"+std::to_string(m_Objects.size()), type, type, (float)gx, (float)gy, (float)gz,0,0,0,gx,gy,gz,"default",true});
        return &m_Objects.back();
    }
    bool RemoveObject(const std::string& id) {
        for(auto it=m_Objects.begin();it!=m_Objects.end();++it) if(it->id==id) { m_Objects.erase(it); return true; }
        return false;
    }
    void SetGridSize(int g) { m_GridSize=g; }
    const std::vector<BuildObject>& GetObjects() const { return m_Objects; }
};

} // namespace NeoEngine
