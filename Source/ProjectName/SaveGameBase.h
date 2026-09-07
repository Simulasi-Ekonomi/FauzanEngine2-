#pragma once
#include <cstdint>
#include <string>
#include <fstream>

class USaveGameBase {
public:
    std::string SaveSlotName;
    uint32_t UserIndex = 0;
    int32_t PlayerLevel = 1;
    float PlayTimeSeconds = 0.0f;

    bool SaveToFile(const std::string& filePath) const {
        std::ofstream outFile(filePath, std::ios::binary | std::ios::trunc);
        if (!outFile.is_open()) return false;
        outFile.write(reinterpret_cast<const char*>(&UserIndex), sizeof(UserIndex));
        outFile.write(reinterpret_cast<const char*>(&PlayerLevel), sizeof(PlayerLevel));
        outFile.write(reinterpret_cast<const char*>(&PlayTimeSeconds), sizeof(PlayTimeSeconds));
        return static_cast<bool>(outFile);
    }

    bool LoadFromFile(const std::string& filePath) {
        std::ifstream inFile(filePath, std::ios::binary);
        if (!inFile.is_open()) return false;
        inFile.read(reinterpret_cast<char*>(&UserIndex), sizeof(UserIndex));
        inFile.read(reinterpret_cast<char*>(&PlayerLevel), sizeof(PlayerLevel));
        inFile.read(reinterpret_cast<char*>(&PlayTimeSeconds), sizeof(PlayTimeSeconds));
        return static_cast<bool>(inFile);
    }
};
