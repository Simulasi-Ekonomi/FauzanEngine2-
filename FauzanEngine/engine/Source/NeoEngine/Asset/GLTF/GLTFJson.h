#include <cassert>
#pragma once

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <fstream>
#include <string>

namespace NeoEngine
{

class GLTFJson
{
public:

    static rapidjson::Document ParseFile(const std::string& path)
    {
        std::ifstream ifs(path);

        if(!ifs.is_open())
        {
            throw std::runtime_error("GLTFJson: cannot open file");
        }

        rapidjson::IStreamWrapper isw(ifs);

        rapidjson::Document doc;
        doc.ParseStream(isw);

        if(doc.HasParseError())
        {
            throw std::runtime_error("GLTFJson: parse error");
        }

        return doc;
    }

};

}
