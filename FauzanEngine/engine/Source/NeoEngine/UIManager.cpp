#include "UIManager.h"
#include <android/log.h>

void UUIManager::InitUIStack() {
    ActiveWidgets.clear();
}

void UUIManager::RenderUI() {
    // Loop through ActiveWidgets and call Draw calls to Vulkan
}