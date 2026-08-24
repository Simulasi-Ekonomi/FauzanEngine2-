#pragma once

class EditorProvider {
public:
    static EditorProvider& Get();
    bool IsPaused() const { return bIsPaused; }
    
private:
    bool bIsPaused = false;
};
