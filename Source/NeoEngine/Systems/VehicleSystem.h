#pragma once
#include <string>

namespace NeoEngine {
struct Vehicle {
    std::string type;
    float posX=0, posY=0, posZ=0, rotX=0, rotY=0, rotZ=0;
    float speed=0, maxSpeed=50, acceleration=10, braking=15, handling=2;
    float health=100, fuel=100;
    bool engineOn=false;
    int passengers=0, maxPassengers=4;
};
class VehicleSystem {
public:
    Vehicle CreateVehicle(const std::string& t, float x, float y, float z) {
        Vehicle v{t, x, y, z};
        if(t=="sportscar"){v.maxSpeed=80;v.acceleration=15;v.handling=3;}
        else if(t=="truck"){v.maxSpeed=30;v.acceleration=5;v.handling=1;}
        else if(t=="helicopter"){v.maxSpeed=40;v.acceleration=8;}
        return v;
    }
    void Update(Vehicle& v, float throttle, float steer, float brake, float dt) {
        if(!v.engineOn)return;
        if(throttle>0)v.speed+=v.acceleration*throttle*dt;
        else if(brake>0)v.speed-=v.braking*brake*dt;
        else v.speed*=0.99f;
        if(v.speed>v.maxSpeed)v.speed=v.maxSpeed;
        if(v.speed<0)v.speed=0;
        v.rotY+=steer*v.handling*dt*(v.speed/v.maxSpeed);
        v.posX+=sin(v.rotY*3.14159f/180.f)*v.speed*dt;
        v.posZ+=cos(v.rotY*3.14159f/180.f)*v.speed*dt;
    }
    void StartEngine(Vehicle& v){v.engineOn=true;}
    void StopEngine(Vehicle& v){v.engineOn=false;v.speed=0;}
};
}
