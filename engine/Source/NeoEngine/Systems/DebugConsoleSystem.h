#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

namespace NeoEngine {

struct ConsoleCommand { std::string name, description; std::function<std::string(const std::vector<std::string>&)> execute; };
struct ConsoleLog { std::string text, color; float timestamp; };

class DebugConsoleSystem {
private:
    std::unordered_map<std::string, ConsoleCommand> m_Commands;
    std::vector<ConsoleLog> m_Logs;
    std::string m_InputBuffer;
    int m_MaxLogs=200;

public:
    DebugConsoleSystem() {
        RegisterCommand("help", "Show all commands", [this](auto&)->std::string{
            std::string list="Commands:\n";
            for(auto& [k,v]:m_Commands) list+="  "+k+" - "+v.description+"\n";
            return list;
        });
        RegisterCommand("clear", "Clear console", [this](auto&){m_Logs.clear();return"Console cleared";});
        RegisterCommand("stats", "Show engine stats", [&](auto&){return"Entities: 0 | FPS: 60 | Memory: 0MB";});
        RegisterCommand("echo", "Print message", [](auto& args){std::string r;for(auto& a:args)r+=a+" ";return r;});
    }

    void RegisterCommand(const std::string& name, const std::string& desc, std::function<std::string(const std::vector<std::string>&)> exec) {
        m_Commands[name]=ConsoleCommand{name, desc, exec};
    }

    std::string Execute(const std::string& input) {
        Log("> " + input, "#00ff00");
        auto parts = SplitString(input);
        if(parts.empty()) return "";
        auto it=m_Commands.find(parts[0]);
        if(it!=m_Commands.end()) {
            std::vector<std::string> args(parts.begin()+1, parts.end());
            std::string result = it->second.execute(args);
            Log(result, "#ffffff");
            return result;
        }
        Log("Unknown command: " + parts[0], "#ff0000");
        return "Unknown command";
    }

    void Log(const std::string& text, const std::string& color="#ffffff") {
        m_Logs.push_back({text, color, 0});
        if(m_Logs.size() > m_MaxLogs) m_Logs.erase(m_Logs.begin());
    }

    std::vector<std::string> SplitString(const std::string& s) {
        std::vector<std::string> res; std::string cur;
        for(char c:s){if(c==' '){if(!cur.empty()){res.push_back(cur);cur.clear();}}else{cur+=c;}}
        if(!cur.empty())res.push_back(cur);
        return res;
    }

    const std::vector<ConsoleLog>& GetLogs() const { return m_Logs; }
};

} // namespace NeoEngine
