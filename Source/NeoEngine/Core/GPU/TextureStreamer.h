#pragma once

#include <string>

namespace NeoEngine
{

class TextureStreamer
{
public:

    void StreamIn(const std::string& path);

    void StreamOut(const std::string& path);

};

}
