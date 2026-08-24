#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

namespace NeoEngine {

struct DialogueLine { std::string speaker, text, portrait; float duration=3.0f; };
struct DialogueChoice { std::string text; int nextId; std::function<void()> action; };
struct DialogueNode { int id; DialogueLine line; std::vector<DialogueChoice> choices; bool autoAdvance=false; };

class DialogueSystem {
private:
    std::unordered_map<std::string, std::vector<DialogueNode>> m_Dialogues;
    std::string m_ActiveDialogue;
    int m_CurrentNode=0;
    std::function<void(const DialogueLine&)> m_OnLine;
    std::function<void(const std::vector<DialogueChoice>&)> m_OnChoices;
    
public:
    void CreateDialogue(const std::string& name, const std::vector<DialogueNode>& nodes){ m_Dialogues[name] = nodes; }
    void StartDialogue(const std::string& name){ if(m_Dialogues.find(name)==m_Dialogues.end())return; m_ActiveDialogue=name; m_CurrentNode=0; ShowCurrentNode(); }
    void SelectChoice(int index){
        if(m_ActiveDialogue.empty())return;
        auto& nodes = m_Dialogues[m_ActiveDialogue];
        if(m_CurrentNode >= nodes.size())return;
        auto& choices = nodes[m_CurrentNode].choices;
        if(index < 0 || index >= choices.size())return;
        if(choices[index].action) choices[index].action();
        if(choices[index].nextId >= 0){ m_CurrentNode = choices[index].nextId; ShowCurrentNode(); }
        else EndDialogue();
    }
    void ShowCurrentNode(){
        auto& nodes = m_Dialogues[m_ActiveDialogue];
        if(m_CurrentNode >= nodes.size()){ EndDialogue(); return; }
        auto& node = nodes[m_CurrentNode];
        if(m_OnLine) m_OnLine(node.line);
        if(!node.choices.empty() && m_OnChoices) m_OnChoices(node.choices);
        if(node.autoAdvance) m_CurrentNode++;
    }
    void EndDialogue(){ m_ActiveDialogue=""; m_CurrentNode=0; }
    void SetOnLine(std::function<void(const DialogueLine&)> cb){ m_OnLine=cb; }
    void SetOnChoices(std::function<void(const std::vector<DialogueChoice>&)> cb){ m_OnChoices=cb; }
};

} // namespace NeoEngine
