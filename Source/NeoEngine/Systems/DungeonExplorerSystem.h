#pragma once
#include <vector>
#include <string>
#include <cstdlib>
#include <functional>

namespace NeoEngine {
enum class RoomType { Empty, Combat, Treasure, Trap, Boss, Shop, Healing, Mystery };
struct DungeonRoom { RoomType type; int x,y; bool explored=false; bool cleared=false; std::string loot; int enemyLevel=1; };
struct Dungeon { std::string name; int width=5,height=5; std::vector<std::vector<DungeonRoom>> rooms; int currentFloor=1; int maxFloors=10; };
class DungeonExplorerSystem {
private:
    Dungeon m_Dungeon; int m_PlayerX=0,m_PlayerY=0,m_PlayerHP=100; std::function<void(DungeonRoom&)> m_OnEnterRoom;
public:
    void GenerateDungeon(int w=5,int h=5,int floors=10){
        m_Dungeon.width=w; m_Dungeon.height=h; m_Dungeon.maxFloors=floors; m_Dungeon.rooms.clear();
        m_Dungeon.rooms.resize(h,std::vector<DungeonRoom>(w));
        for(int y=0;y<h;y++)for(int x=0;x<w;x++){
            RoomType t; int r=rand()%100;
            if(x==0&&y==0)t=RoomType::Empty;
            else if(x==w-1&&y==h-1)t=RoomType::Boss;
            else if(r<30)t=RoomType::Combat;
            else if(r<50)t=RoomType::Treasure;
            else if(r<60)t=RoomType::Trap;
            else if(r<75)t=RoomType::Empty;
            else if(r<85)t=RoomType::Healing;
            else if(r<95)t=RoomType::Shop;
            else t=RoomType::Mystery;
            m_Dungeon.rooms[y][x]={t,x,y,false,false,"",1+rand()%m_Dungeon.currentFloor};
        }
        m_PlayerX=m_PlayerY=0; m_Dungeon.rooms[0][0].explored=true;
    }
    bool Move(int dx,int dy){
        int nx=m_PlayerX+dx,ny=m_PlayerY+dy;
        if(nx<0||nx>=m_Dungeon.width||ny<0||ny>=m_Dungeon.height)return false;
        m_PlayerX=nx;m_PlayerY=ny;
        auto& room=m_Dungeon.rooms[ny][nx];
        room.explored=true;
        if(m_OnEnterRoom)m_OnEnterRoom(room);
        return true;
    }
    void SetPlayerHP(int hp){m_PlayerHP=hp;}
    int GetPlayerHP()const{return m_PlayerHP;}
    int GetPlayerX()const{return m_PlayerX;}
    int GetPlayerY()const{return m_PlayerY;}
    const Dungeon& GetDungeon()const{return m_Dungeon;}
    void NextFloor(){ m_Dungeon.currentFloor++; GenerateDungeon(m_Dungeon.width,m_Dungeon.height,m_Dungeon.maxFloors); }
    void SetOnEnterRoom(std::function<void(DungeonRoom&)> cb){ m_OnEnterRoom=cb; }
};
}
