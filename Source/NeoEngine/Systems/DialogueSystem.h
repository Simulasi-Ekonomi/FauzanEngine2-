#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

namespace NeoEngine {

struct DialogueLine { std::string speaker, text; };
struct DialogueChoice { std::string text; int nextNode = -1; std::function<void()> action; };
struct DialogueNode { int id; DialogueLine line; std::vector<DialogueChoice> choices; };

class DialogueSystem {
public:
    void CreateDialogue(const std::string& name, const std::vector<DialogueNode>& nodes) {
        m_Dialogues[name] = nodes;
    }

    void StartDialogue(const std::string& name) {
        auto it = m_Dialogues.find(name);
        if (it != m_Dialogues.end()) {
            m_Active = name;
            m_Current = 0;
        }
    }

    const DialogueNode* GetCurrentNode() const {
        auto it = m_Dialogues.find(m_Active);
        if (it != m_Dialogues.end() && m_Current < (int)it->second.size())
            return &it->second[m_Current];
        return nullptr;
    }

    bool SelectChoice(int index) {
        auto* node = GetCurrentNode();
        if (!node || index >= (int)node->choices.size()) return false;
        auto& choice = node->choices[index];
        if (choice.action) choice.action();
        if (choice.nextNode >= 0) m_Current = choice.nextNode;
        else EndDialogue();
        return true;
    }

    void EndDialogue() { m_Active.clear(); m_Current = 0; }
    bool IsInDialogue() const { return !m_Active.empty(); }

private:
    std::unordered_map<std::string, std::vector<DialogueNode>> m_Dialogues;
    std::string m_Active;
    int m_Current = 0;
};

} // namespace NeoEngine
