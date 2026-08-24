#pragma once
#include <string>
#include <functional>
struct StreamRequest{
    std::string asset;
    std::function<void(void*)> onLoaded;
};
