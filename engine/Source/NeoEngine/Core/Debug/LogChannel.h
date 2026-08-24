#pragma once

enum class LogLevel {
    Info,
    Warning,
    Error,
    Fatal
};

enum class LogChannel {
    Core,
    Memory,
    Platform,
    ECS,
    Rendering
};
