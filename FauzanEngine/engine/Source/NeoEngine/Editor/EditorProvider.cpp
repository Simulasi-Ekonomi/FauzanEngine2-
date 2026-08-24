#include "EditorProvider.h"

EditorProvider& EditorProvider::Get() {
    static EditorProvider Instance;
    return Instance;
}
