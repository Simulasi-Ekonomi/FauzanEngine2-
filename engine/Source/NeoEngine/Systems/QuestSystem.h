#pragma once
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
namespace NeoEngine {
enum class QuestStatus{Available,Active,Completed,Failed};
enum class QuestType{Main,Side,Daily,Event,Achievement};
struct Quest{std::string id,title,description;QuestType type;QuestStatus status=QuestStatus::Available;std::vector<std::string> objectives;std::vector<int> objProgress,objTargets;std::string rewardItem;int rewardXP=0,rewardGold=0;};
class QuestSystem{
    std::unordered_map<std::string,Quest> m_Quests;
    std::function<void(const Quest&)> m_OnComplete;
    int m_NextId=1;
public:
    std::string AddQuest(const std::string& t,const std::string& d,QuestType ty){std::string id="q_"+std::to_string(m_NextId++);m_Quests[id]={id,t,d,ty};return id;}
    void AddObjective(const std::string& qid,const std::string& desc,int target){auto it=m_Quests.find(qid);if(it!=m_Quests.end()){it->second.objectives.push_back(desc);it->second.objProgress.push_back(0);it->second.objTargets.push_back(target);}}
    void SetReward(const std::string& qid,int xp,int gold,const std::string& item=""){auto it=m_Quests.find(qid);if(it!=m_Quests.end()){it->second.rewardXP=xp;it->second.rewardGold=gold;it->second.rewardItem=item;}}
    bool AcceptQuest(const std::string& qid){auto it=m_Quests.find(qid);if(it!=m_Quests.end()&&it->second.status==QuestStatus::Available){it->second.status=QuestStatus::Active;return true;}return false;}
    bool UpdateProgress(const std::string& qid,int idx,int amount){auto it=m_Quests.find(qid);if(it==m_Quests.end())return false;it->second.objProgress[idx]+=amount;bool done=true;for(size_t i=0;i<it->second.objProgress.size();i++)if(it->second.objProgress[i]<it->second.objTargets[i]){done=false;break;}if(done){it->second.status=QuestStatus::Completed;if(m_OnComplete)m_OnComplete(it->second);}return true;}
    void SetOnComplete(std::function<void(const Quest&)> cb){m_OnComplete=cb;}
};
}
