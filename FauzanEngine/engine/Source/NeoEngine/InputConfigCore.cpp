#include "InputConfigCore.h"

void UInputConfigCore::BindAction(std::string ActionName, int KeyCode) {
    ActionMappings[ActionName] = KeyCode;
}