#!/bin/bash
# Audit Stubs for FauzanEngine
# Identifikasi file stub, placeholder, TODO, dan fungsi kosong

TARGET_DIR="${1:-.}"
REPORT_FILE="audit_stubs_report_$(date +%Y%m%d_%H%M%S).txt"

echo "🔍 Memulai audit stub di: $TARGET_DIR"
echo "Laporan disimpan ke: $REPORT_FILE"
echo "============================================" | tee "$REPORT_FILE"

# Fungsi cek stub
check_stub() {
    local file="$1"
    local basename=$(basename "$file")
    
    # Abaikan folder build dan dependency
    if echo "$file" | grep -qE "(build|node_modules|\.git|__pycache__|cereal)"; then
        return
    fi
    
    local size=$(wc -l < "$file")
    local todo_count=$(grep -ciE "(TODO|FIXME|HACK|XXX|PLACEHOLDER|stub|not implemented|implement later)" "$file")
    local stub_keywords=$(grep -ciE "(return \{\};|return \"\";|return false;|return nullptr;|return 0;|throw std::runtime_error|// TODO|# TODO|pass\s*$)" "$file")
    local empty_functions=$(grep -cE "^\s*(void|int|bool|float|double|string|auto)\s+\w+\s*\(.*\)\s*\{\s*\}" "$file")
    
    local reason=""
    if [ $size -lt 10 ] && [ $size -gt 0 ]; then
        reason="File terlalu pendek ($size baris)"
    fi
    if [ $todo_count -gt 0 ]; then
        reason="$reason | Ada $todo_count TODO/stub marker"
    fi
    if [ $stub_keywords -gt 1 ]; then
        reason="$reason | Banyak stub keyword ($stub_keywords)"
    fi
    if [ $empty_functions -gt 0 ]; then
        reason="$reason | Fungsi kosong: $empty_functions"
    fi
    
    if [ -n "$reason" ]; then
        # Kategorikan
        local category="Unknown"
        if echo "$file" | grep -qE "(AI|BehaviorTree|Navigation|AIController)"; then
            category="AI System"
        elif echo "$file" | grep -qE "(ECS|ECSManager|Entity|Component|System)"; then
            category="ECS Core"
        elif echo "$file" | grep -qE "(Render|Shader|RHI|Vulkan|OpenGL|GPU|Viewport)"; then
            category="Rendering"
        elif echo "$file" | grep -qE "(Physics|RigidBody|Collision|Collider)"; then
            category="Physics"
        elif echo "$file" | grep -qE "(World|Scene|Level|Streaming|Chunk|Biome)"; then
            category="World Generation"
        elif echo "$file" | grep -qE "(LiteRT|Gemma|Ruflo|Hermes|Aries|Brain|LLM)"; then
            category="AI Integration"
        elif echo "$file" | grep -qE "(Android|JNI|Bridge|Activity)"; then
            category="Android/JNI"
        elif echo "$file" | grep -qE "(Editor|WebView|AIConsole|GameRuntime)"; then
            category="Editor Frontend"
        fi
        
        echo "[STUB] $file | Kategori: $category | $reason" | tee -a "$REPORT_FILE"
    fi
}

# Pencarian file kode sumber
find "$TARGET_DIR" -type f \( \
    -name "*.cpp" -o -name "*.h" -o -name "*.hpp" -o -name "*.java" \
    -o -name "*.kt" -o -name "*.py" -o -name "*.ts" -o -name "*.tsx" \
    -o -name "*.js" -o -name "*.jsx" \
\) ! -path "*/build/*" ! -path "*/node_modules/*" ! -path "*/.git/*" ! -path "*/__pycache__/*" \
| sort | while read file; do
    check_stub "$file"
done

echo "============================================" | tee -a "$REPORT_FILE"
total_stubs=$(grep -c "^\[STUB\]" "$REPORT_FILE")
echo "Total file stub ditemukan: $total_stubs" | tee -a "$REPORT_FILE"
echo "Laporan lengkap: $REPORT_FILE"
