#include "AndroidPlatform.h"
#include <iostream>
void AndroidPlatform::Init() { std::cout << "[Platform] Android Init" << std::endl; }
void AndroidPlatform::PumpEvents() { /* Placeholder */ }
void AndroidPlatform::Shutdown() { std::cout << "[Platform] Android Shutdown" << std::endl; }
