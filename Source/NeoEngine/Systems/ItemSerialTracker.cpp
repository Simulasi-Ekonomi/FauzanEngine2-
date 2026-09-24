#include "ItemSerialTracker.h"
#include <algorithm>
#include <limits>
#include <sstream>
namespace NeoEngine {
ItemSerialTracker::ItemSerialTracker() {
    m_RNG.seed(std::chrono::high_resolution_clock::now().time_since_epoch().count());
}
std::string ItemSerialTracker::GenerateSerialLocked(const std::string& itemType,const std::string& itemName) {
    const auto now=std::chrono::system_clock::now();
    const auto ms=std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    const char* typeCode="XX";
    if(itemType=="weapon") typeCode="WP"; else if(itemType=="armor") typeCode="AR"; else if(itemType=="potion") typeCode="PT";
    else if(itemType=="material") typeCode="MT"; else if(itemType=="currency") typeCode="CR"; else if(itemType=="reward") typeCode="RW";
    const std::uint64_t randomPart=m_RNG()%100000ULL;
    const std::uint64_t checksum=(static_cast<std::uint64_t>(ms)+randomPart+itemName.size())%10000ULL;
    char buffer[64]{};
    std::snprintf(buffer,sizeof(buffer),"FE-%s-%04llX-%05llu-%04llu",typeCode,
        static_cast<unsigned long long>(static_cast<std::uint64_t>(ms)&0xFFFFULL),
        static_cast<unsigned long long>(randomPart),static_cast<unsigned long long>(checksum));
    return buffer;
}
std::string ItemSerialTracker::GenerateSerial(const std::string& itemType,const std::string& itemName) {
    std::lock_guard lock(m_Mutex); ++m_TotalItems; std::string serial;
    do { serial=GenerateSerialLocked(itemType,itemName); } while(m_Registry.find(serial)!=m_Registry.end());
    return serial;
}
std::string ItemSerialTracker::RegisterGeneratedItemLocked(const std::string& ownerId,const std::string& itemType,
                                                           const std::string& itemName,int quantity,const std::string& source) {
    if(quantity<=0) quantity=1; ++m_TotalItems; std::string serial;
    do { serial=GenerateSerialLocked(itemType,itemName); } while(m_Registry.find(serial)!=m_Registry.end());
    SerialNumber item; item.number=serial; item.itemType=itemType; item.itemName=itemName; item.quantity=quantity;
    item.source=source; item.ownerId=ownerId; item.timestamp=std::chrono::system_clock::now();
    m_Registry.emplace(serial,std::move(item)); m_PendingVerification.push_back(serial);
    if(!ownerId.empty()) AddAuditLocked(ownerId,"REGISTER|"+serial+"|"+source);
    return serial;
}
SerialNumber* ItemSerialTracker::RegisterItem(const std::string& ownerId,const std::string& itemType,
                                              const std::string& itemName,int quantity,const std::string& source) {
    std::lock_guard lock(m_Mutex);
    return &m_Registry.at(RegisterGeneratedItemLocked(ownerId,itemType,itemName,quantity,source));
}
bool ItemSerialTracker::VerifyItemSilently(const std::string& id,const std::string& playerId) {
    std::lock_guard lock(m_Mutex); const auto it=m_Registry.find(id);
    return it!=m_Registry.end()&&it->second.ownerId==playerId&&it->second.verified&&!it->second.consumed;
}
bool ItemSerialTracker::VerifyWithServer(const std::string& serialNumber) {
    std::function<void(const std::string&)> cb;
    { std::lock_guard lock(m_Mutex); const auto it=m_Registry.find(serialNumber);
      if(it==m_Registry.end()||it->second.consumed){++m_RejectedCount;return false;}
      if(!it->second.verified){it->second.verified=true;++m_VerifiedCount;AddAuditLocked(it->second.ownerId,"VERIFY|"+serialNumber);}
      cb=m_OnVerified;
    }
    if(cb) cb(serialNumber); return true;
}
bool ItemSerialTracker::IsItemValid(const std::string& serialNumber) const {
    std::lock_guard lock(m_Mutex); const auto it=m_Registry.find(serialNumber);
    return it!=m_Registry.end()&&it->second.verified&&!it->second.consumed;
}
bool ItemSerialTracker::ConsumeItem(const std::string& serialNumber) {
    std::lock_guard lock(m_Mutex); const auto it=m_Registry.find(serialNumber);
    if(it==m_Registry.end()||it->second.consumed||!it->second.verified) return false;
    it->second.consumed=true; AddAuditLocked(it->second.ownerId,"CONSUME|"+serialNumber); return true;
}
int ItemSerialTracker::VerifyPendingBatch(int maxBatch) {
    if(maxBatch<=0) return 0; std::vector<std::string> verified; std::function<void(const std::string&)> cb;
    { std::lock_guard lock(m_Mutex); const std::size_t limit=std::min<std::size_t>(static_cast<std::size_t>(maxBatch),m_PendingVerification.size());
      std::vector<std::string> remaining; remaining.reserve(m_PendingVerification.size()-limit);
      for(std::size_t i=0;i<m_PendingVerification.size();++i){const auto& serial=m_PendingVerification[i];
        if(i>=limit){remaining.push_back(serial);continue;} const auto it=m_Registry.find(serial);
        if(it==m_Registry.end()||it->second.consumed){++m_RejectedCount;continue;}
        it->second.verified=true;++m_VerifiedCount;verified.push_back(serial);AddAuditLocked(it->second.ownerId,"VERIFY|"+serial);
      }
      m_PendingVerification.swap(remaining); cb=m_OnVerified;
    }
    if(cb) for(const auto& serial:verified) cb(serial);
    return static_cast<int>(verified.size());
}
std::string ItemSerialTracker::GenerateCurrencySerial(const std::string& currencyType,int amount,const std::string& source) {
    std::lock_guard lock(m_Mutex); return RegisterGeneratedItemLocked("","currency",currencyType,amount,source);
}
std::string ItemSerialTracker::GenerateRewardSerial(const std::string& rewardName,const std::string& source) {
    std::lock_guard lock(m_Mutex); return RegisterGeneratedItemLocked("","reward",rewardName,1,source);
}
bool ItemSerialTracker::IsSerialDuplicate(const std::string& serialNumber) const {
    std::lock_guard lock(m_Mutex); const auto it=m_Registry.find(serialNumber); return it!=m_Registry.end()&&it->second.consumed;
}
std::string ItemSerialTracker::GetAuditTrail(const std::string& playerId) const {
    std::lock_guard lock(m_Mutex); const auto it=m_AuditTrail.find(playerId);
    if(it==m_AuditTrail.end()) return "Audit for "+playerId; std::ostringstream report; report<<"Audit for "<<playerId;
    for(const auto& entry:it->second) report<<'\n'<<entry; return report.str();
}
void ItemSerialTracker::MarkAllPlayerItemsContaminated(const std::string& playerId,const std::string& playerName) {
    std::function<void(const std::string&,const std::string&)> cb; std::vector<std::string> contaminated;
    { std::lock_guard lock(m_Mutex); for(auto& [serial,item]:m_Registry) if(item.ownerId==playerId&&!item.consumed){
        item.verified=false; item.serverSignature="CONTAMINATED"; contaminated.push_back(serial);
        AddAuditLocked(playerId,"CONTAMINATE|"+serial+"|"+playerId);
      } cb=m_OnCheatDetected; if(!contaminated.empty()) ++m_RejectedCount; }
    if(cb) for(const auto& serial:contaminated) cb(playerId,"Contaminated item "+serial+" owned by "+playerName);
}
bool ItemSerialTracker::TransferOwnership(const std::string& serial,const std::string& fromId,const std::string& fromName,
                                           const std::string& toId,const std::string& toName,const std::string& method) {
    if(serial.empty()||fromId.empty()||toId.empty()||fromId==toId||method.empty()) return false;
    std::lock_guard lock(m_Mutex); const auto it=m_Registry.find(serial);
    if(it==m_Registry.end()||it->second.ownerId!=fromId||!it->second.verified||it->second.consumed) return false;
    it->second.ownerId=toId; it->second.serverSignature=GenerateTransferHash(serial,fromId,toId,method);
    const std::string entry="TRANSFER|"+serial+"|"+fromId+"|"+toId+"|"+method;
    AddAuditLocked(fromId,entry); AddAuditLocked(toId,entry);
    AddAuditLocked(toId,"OWNER|"+serial+"|"+toId+"|"+toName); AddAuditLocked(fromId,"OWNER|"+serial+"|"+fromId+"|"+fromName); return true;
}
int ItemSerialTracker::GetVerifiedCount() const { std::lock_guard lock(m_Mutex); return m_VerifiedCount; }
int ItemSerialTracker::GetRejectedCount() const { std::lock_guard lock(m_Mutex); return m_RejectedCount; }
int ItemSerialTracker::GetTotalItems() const { std::lock_guard lock(m_Mutex); return m_TotalItems; }
void ItemSerialTracker::SetServerURL(const std::string& url){std::lock_guard lock(m_Mutex);m_ServerURL=url;}
void ItemSerialTracker::SetServerPublicKey(const std::string& key){std::lock_guard lock(m_Mutex);m_ServerPublicKey=key;}
void ItemSerialTracker::SetOnCheatDetected(std::function<void(const std::string&,const std::string&)> cb){std::lock_guard lock(m_Mutex);m_OnCheatDetected=std::move(cb);}
void ItemSerialTracker::SetOnVerified(std::function<void(const std::string&)> cb){std::lock_guard lock(m_Mutex);m_OnVerified=std::move(cb);}
void ItemSerialTracker::AddAuditLocked(const std::string& playerId,const std::string& entry){if(!playerId.empty())m_AuditTrail[playerId].push_back(entry);}
std::string ItemSerialTracker::GenerateTransferHash(const std::string& serial,const std::string& from,const std::string& to,const std::string& method) const {
    return std::to_string(std::hash<std::string>{}(serial+from+to+method));
}
size_t ItemSerialTracker::WriteCallback(void* contents,size_t size,size_t nmemb,std::string* output){
    if(contents==nullptr||output==nullptr||size!=0&&nmemb>std::numeric_limits<size_t>::max()/size)return 0;
    const size_t bytes=size*nmemb; output->append(static_cast<const char*>(contents),bytes); return bytes;
}
} // namespace NeoEngine
