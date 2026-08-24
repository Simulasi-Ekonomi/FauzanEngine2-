#include "Object.h"
#include <atomic>

static std::atomic<uint64_t> GlobalIDCounter(0);

Object::Object() {
    ID = ++GlobalIDCounter;
    Name = "Object_" + std::to_string(ID);
}

Object::~Object() {}

uint64_t Object::GetID() const { return ID; }
std::string Object::GetName() const { return Name; }
