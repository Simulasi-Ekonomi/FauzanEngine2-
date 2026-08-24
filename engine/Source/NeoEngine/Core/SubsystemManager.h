#pragma once

class Subsystem;

class SubsystemManager {
public:
    static void Register(Subsystem* system);
    static void InitAll();
    static void TickAll();
    static void ShutdownAll();
};
