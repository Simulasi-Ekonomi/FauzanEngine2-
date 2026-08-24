#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>
#include <json/json.h>

namespace NeoEngine {

enum class TerrainType { Grass, Dirt, Sand, Rock, Water, Lava, Ice, Road, Bridge, Cliff };
enum class EditorTool { Paint, Erase, Fill, Select, Move, Rotate, Scale };

struct MapTile { TerrainType terrain=TerrainType::Grass; int height=0; std::string objectId; };
struct MapObject { std::string id, type; float x,y,z; float rx,ry,rz; float sx=1,sy=1,sz=1; std::unordered_map<std::string,std::string> props; };

class MapEditorSystem {
private:
    std::vector<std::vector<MapTile>> m_Tiles;
    std::vector<MapObject> m_Objects;
    int m_MapWidth=64, m_MapHeight=64;
    int m_CursorX=0, m_CursorY=0;
    EditorTool m_CurrentTool=EditorTool::Paint;
    TerrainType m_BrushTerrain=TerrainType::Grass;
    std::string m_MapName="New Map";
    int m_ObjectCount=0;

public:
    MapEditorSystem(int w=64, int h=64):m_MapWidth(w),m_MapHeight(h) { m_Tiles.resize(h, std::vector<MapTile>(w)); }

    void PaintTile(int x, int y) { if(x>=0&&x<m_MapWidth&&y>=0&&y<m_MapHeight) m_Tiles[y][x].terrain=m_BrushTerrain; }
    void SetHeight(int x, int y, int h) { if(x>=0&&x<m_MapWidth&&y>=0&&y<m_MapHeight) m_Tiles[y][x].height=h; }
    MapObject* AddObject(const std::string& type, float x, float y, float z) {
        m_Objects.push_back({"obj_"+std::to_string(++m_ObjectCount), type, x, y, z}); return &m_Objects.back();
    }
    bool RemoveObject(const std::string& id) {
        for(auto it=m_Objects.begin();it!=m_Objects.end();++it) if(it->id==id) { m_Objects.erase(it); return true; }
        return false;
    }

    std::string ExportToJSON() const {
        Json::Value root; root["name"]=m_MapName; root["width"]=m_MapWidth; root["height"]=m_MapHeight;
        Json::Value tiles(Json::arrayValue);
        for(int y=0;y<m_MapHeight;y++) for(int x=0;x<m_MapWidth;x++) {
            Json::Value t; t["x"]=x;t["y"]=y;t["terrain"]=(int)m_Tiles[y][x].terrain;t["height"]=m_Tiles[y][x].height;
            tiles.append(t);
        }
        root["tiles"]=tiles;
        Json::Value objs(Json::arrayValue);
        for(auto& o:m_Objects) {
            Json::Value obj; obj["id"]=o.id;obj["type"]=o.type;obj["x"]=o.x;obj["y"]=o.y;obj["z"]=o.z;
            obj["rx"]=o.rx;obj["ry"]=o.ry;obj["rz"]=o.rz;obj["sx"]=o.sx;obj["sy"]=o.sy;obj["sz"]=o.sz;
            objs.append(obj);
        }
        root["objects"]=objs;
        Json::FastWriter w; return w.write(root);
    }

    void SetTool(EditorTool t) { m_CurrentTool=t; }
    void SetBrushTerrain(TerrainType t) { m_BrushTerrain=t; }
    void SetMapName(const std::string& n) { m_MapName=n; }
};

} // namespace NeoEngine
