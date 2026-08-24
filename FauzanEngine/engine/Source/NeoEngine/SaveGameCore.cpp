#include "SaveGameCore.h"
#include <fstream>

void USaveGameCore::SaveToSlot(std::string SlotName) {
    std::ofstream saveFile("/sdcard/NeoEngine/Saves/" + SlotName + ".sav");
    saveFile << "PlayerPos_X: " << 0.0f << "\n"; // Contoh placeholder data
    saveFile.close();
}