#pragma once
#include <array>
#include <cstdint>
#include <cstring>

namespace NeoEngine::Networking {

enum class PacketType : uint8_t { Hello=1, Challenge=2, Input=3, Snapshot=4, Ack=5, Spawn=6, Destroy=7, Disconnect=8 };
struct PacketHeader { uint32_t magic{0x464E4554}; uint8_t version{1}; PacketType type{}; uint16_t payloadBytes{}; uint64_t sequence{}; uint64_t ack{}; };

class PacketFrame {
public:
    static constexpr uint16_t MaxPayload=1200;
    static constexpr uint16_t HeaderBytes=24;
    static constexpr uint16_t MaxBytes=HeaderBytes+MaxPayload;
    bool encode(const PacketHeader& header,const uint8_t* payload,uint16_t bytes){if(header.magic!=Magic||header.version!=Version||bytes>MaxPayload||(bytes&&!payload)||header.payloadBytes!=bytes)return false;std::memcpy(buffer_.data(),&header,HeaderBytes);if(bytes)std::memcpy(buffer_.data()+HeaderBytes,payload,bytes);size_=HeaderBytes+bytes;return true;}
    bool decode(PacketHeader& header,uint8_t* payload,uint16_t capacity,uint16_t& bytes)const{if(size_<HeaderBytes)return false;std::memcpy(&header,buffer_.data(),HeaderBytes);if(header.magic!=Magic||header.version!=Version||header.payloadBytes>MaxPayload||size_!=HeaderBytes+header.payloadBytes||header.payloadBytes>capacity)return false;bytes=header.payloadBytes;if(bytes&&!payload)return false;if(bytes)std::memcpy(payload,buffer_.data()+HeaderBytes,bytes);return true;}
    const uint8_t* data()const{return buffer_.data();} uint16_t size()const{return size_;}
private:
    static constexpr uint32_t Magic=0x464E4554;static constexpr uint8_t Version=1;std::array<uint8_t,MaxBytes> buffer_{};uint16_t size_{};
};
}
