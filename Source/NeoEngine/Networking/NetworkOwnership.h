#pragma once
#include <cstdint>

namespace NeoEngine::Networking {

enum class AuthorityRole : uint8_t { None, Server, Owner, Observer };

class OwnershipPolicy {
public:
    static bool canWrite(AuthorityRole role,uint32_t actorOwner,uint32_t peerId){
        if(!peerId)return false;
        if(role==AuthorityRole::Server)return true;
        return role==AuthorityRole::Owner&&actorOwner==peerId;
    }
    static bool canRead(AuthorityRole role){return role!=AuthorityRole::None;}
};
}
