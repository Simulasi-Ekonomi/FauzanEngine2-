#pragma once
#include <vector>
#include <string>

class PlatformFS {
public:
    // Abstraksi pembacaan file agar Core tidak tahu apakah file di APK atau Folder
    static std::vector<char> ReadFile(const std::string& Path);
    static bool FileExists(const std::string& Path);
};
