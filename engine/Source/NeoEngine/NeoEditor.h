#pragma once
#include "ActorCore.h"
#include "World/NeoWorld.h"

namespace NeoEngine {

enum class EditorMode { Select, Move, Rotate, Scale };
enum class EditorSnap { None, Grid1, Grid5, Grid10 };

class NeoEditor {
public:
    NeoEditor() = default;

    void SetWorld(NeoWorld* world) { m_World = world; }
    void SelectActor(EntityID id) { m_SelectedId = id; }
    EntityID GetSelectedId() const { return m_SelectedId; }

    void SetEditorMode(EditorMode mode) { m_Mode = mode; }
    EditorMode GetEditorMode() const { return m_Mode; }

    void SetSnap(EditorSnap snap) { m_Snap = snap; }
    EditorSnap GetSnap() const { return m_Snap; }

    void MoveSelected(float dx, float dy, float dz) {
        if (m_World && m_SelectedId != INVALID_ENTITY) {
            auto* actor = m_World->GetActor(m_SelectedId);
            if (actor) {
                actor->posX += dx; actor->posY += dy; actor->posZ += dz;
            }
        }
    }

    void DeleteSelected() {
        if (m_World && m_SelectedId != INVALID_ENTITY) {
            m_World->DestroyActor(m_SelectedId);
            m_SelectedId = INVALID_ENTITY;
        }
    }

    void DuplicateSelected() {
        if (m_World && m_SelectedId != INVALID_ENTITY) {
            auto* src = m_World->GetActor(m_SelectedId);
            if (src) {
                m_SelectedId = m_World->SpawnActor(
                    src->name + "_Copy", src->type,
                    src->posX + 1, src->posY, src->posZ + 1);
            }
        }
    }

private:
    NeoWorld* m_World = nullptr;
    EntityID m_SelectedId = INVALID_ENTITY;
    EditorMode m_Mode = EditorMode::Select;
    EditorSnap m_Snap = EditorSnap::None;
};

} // namespace NeoEngine
