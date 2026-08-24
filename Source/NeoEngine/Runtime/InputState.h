#pragma once
#include <cstdint>
#include <string>
#include <vector>
namespace NeoEngine {
enum class InputError:uint8_t{None,InvalidAction,DuplicateAction,MissingAction,Capacity,QueueFull};
enum class InputDeviceType:uint8_t{Keyboard=1,Mouse=2,Touch=3,Gamepad=4};
constexpr int32_t MakeInputCode(InputDeviceType device,uint16_t control){return static_cast<int32_t>((static_cast<uint32_t>(device)<<16U)|control);}
struct InputSnapshot{bool pressed=false,justPressed=false,justReleased=false;};
class InputState{public:static constexpr size_t kMaxActions=128,kMaxEvents=512;bool Bind(std::string action,int32_t code);bool Rebind(const std::string&action,int32_t code);bool Push(int32_t code,bool pressed);void BeginFrame();InputSnapshot Query(const std::string&action)const;bool HasAction(const std::string&action)const;InputError LastError()const{return m_Error;}private:struct Action{std::string id;int32_t code;InputSnapshot state;};struct Event{int32_t code;bool pressed;};std::vector<Action>m_Actions;std::vector<Event>m_Events;InputError m_Error=InputError::None;};}
