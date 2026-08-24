#include "NeoEditor.h"
#include "Core/Math/Vector3.h"
#include <android/log.h>

namespace NeoEngine {

// Gizmo rendering helper (menggambar sumbu X/Y/Z berwarna)
void DrawTranslationGizmo(float px, float py, float pz, float scale) {
    // Sumbu X (Merah)
    // glColor3f(1,0,0); glBegin(GL_LINES); glVertex3f(px,py,pz); glVertex3f(px+scale,py,pz); glEnd();
    // Sumbu Y (Hijau)
    // glColor3f(0,1,0); glBegin(GL_LINES); glVertex3f(px,py,pz); glVertex3f(px,py+scale,pz); glEnd();
    // Sumbu Z (Biru)
    // glColor3f(0,0,1); glBegin(GL_LINES); glVertex3f(px,py,pz); glVertex3f(px,py,pz+scale); glEnd();
    __android_log_print(ANDROID_LOG_DEBUG, "NeoEditor", "Gizmo at (%.1f, %.1f, %.1f)", px, py, pz);
}

} // namespace NeoEngine
