package com.neoengine.core

import android.content.Context
import android.util.Log
import com.google.ai.edge.litertlm.*
import kotlinx.coroutines.*
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.flowOf
import kotlinx.coroutines.flow.map

class NeoEngineToolSet : ToolSet {
    @Tool(description = "Add a new 3D actor to the scene at specified position")
    fun addActor(
        @ToolParam(description = "Type: cube, sphere, plane, cylinder, cone, light_directional, light_point, camera, static_mesh, water, foliage") type: String,
        @ToolParam(description = "Name of the actor") name: String,
        @ToolParam(description = "X position") x: Float,
        @ToolParam(description = "Y position") y: Float,
        @ToolParam(description = "Z position") z: Float
    ): String {
        return try {
            val result = NeoEngineBridgeNative.nativeAddActor(type, name, x, y, z)
            if (result >= 0) "Actor '$name' (type=$type) created at ($x,$y,$z) with id=$result"
            else "Failed to create actor '$name'"
        } catch (e: Exception) {
            notifyWebView("ADD_ACTOR", """{"type":"$type","name":"$name","x":$x,"y":$y,"z":$z}""")
            "Actor '$name' creation requested via WebView"
        }
    }

    @Tool(description = "Set transform (position, rotation, scale) of an actor")
    fun setTransform(
        @ToolParam(description = "Name or ID of the actor") actorName: String,
        @ToolParam(description = "X position") px: Float,
        @ToolParam(description = "Y position") py: Float,
        @ToolParam(description = "Z position") pz: Float,
        @ToolParam(description = "X rotation (degrees)") rx: Float,
        @ToolParam(description = "Y rotation (degrees)") ry: Float,
        @ToolParam(description = "Z rotation (degrees)") rz: Float,
        @ToolParam(description = "X scale") sx: Float,
        @ToolParam(description = "Y scale") sy: Float,
        @ToolParam(description = "Z scale") sz: Float
    ): String {
        return try {
            NeoEngineBridgeNative.nativeSetTransform(actorName, px, py, pz, rx, ry, rz, sx, sy, sz)
            "Transform of '$actorName' updated: pos=($px,$py,$pz) rot=($rx,$ry,$rz) scale=($sx,$sy,$sz)"
        } catch (e: Exception) {
            notifyWebView("SET_TRANSFORM", """{"name":"$actorName","px":$px,"py":$py,"pz":$pz,"rx":$rx,"ry":$ry,"rz":$rz,"sx":$sx,"sy":$sy,"sz":$sz}""")
            "Transform update requested via WebView"
        }
    }

    @Tool(description = "Delete an actor from the scene")
    fun deleteActor(
        @ToolParam(description = "Name of the actor to delete") actorName: String
    ): String {
        return try {
            val ok = NeoEngineBridgeNative.nativeDeleteActor(actorName)
            if (ok) "Actor '$actorName' deleted" else "Actor '$actorName' not found"
        } catch (e: Exception) {
            notifyWebView("DELETE_ACTOR", """{"name":"$actorName"}""")
            "Delete requested via WebView"
        }
    }

    @Tool(description = "Get current scene state as JSON")
    fun getSceneState(): String {
        return try {
            NeoEngineBridgeNative.nativeGetSceneJSON()
        } catch (e: Exception) {
            """{"actors":[],"status":"scene_unavailable"}"""
        }
    }

    @Tool(description = "Set material property of an actor")
    fun setMaterial(
        @ToolParam(description = "Name of the actor") actorName: String,
        @ToolParam(description = "Color in hex format e.g. #FF5500") color: String,
        @ToolParam(description = "Roughness 0.0-1.0") roughness: Float,
        @ToolParam(description = "Metalness 0.0-1.0") metalness: Float
    ): String {
        notifyWebView("SET_MATERIAL", """{"name":"$actorName","color":"$color","roughness":$roughness,"metalness":$metalness}""")
        return "Material of '$actorName' updated: color=$color roughness=$roughness metalness=$metalness"
    }

    @Tool(description = "Generate C++ gameplay code and add to engine")
    fun generateCode(
        @ToolParam(description = "Description of the gameplay feature") description: String
    ): String {
        notifyWebView("GENERATE_CODE", """{"description":"$description"}""")
        return "Code generation for '$description' requested from Aries Director"
    }

    @Tool(description = "Create a complete game world based on description")
    fun createGameWorld(
        @ToolParam(description = "Description of the game world to create") description: String
    ): String {
        notifyWebView("CREATE_WORLD", """{"description":"$description"}""")
        return "World creation for '$description' delegated to Aries+Hermes+Ruflo agents"
    }

    @Tool(description = "Play or stop the game simulation")
    fun setPlayMode(
        @ToolParam(description = "true to play, false to stop") playing: Boolean
    ): String {
        notifyWebView("SET_PLAY_MODE", """{"playing":$playing}""")
        return if (playing) "Game simulation started" else "Game simulation stopped"
    }

    @Tool(description = "Get engine performance metrics")
    fun getEngineMetrics(): String {
        return try {
            NeoEngineBridgeNative.nativeGetTelemetryJSON()
        } catch (e: Exception) {
            """{"fps":0,"status":"unavailable"}"""
        }
    }

    @Tool(description = "Generate a detailed 3D asset from text description")
    fun generate3DAsset(
        @ToolParam(description = "Description of the 3D model") description: String,
        @ToolParam(description = "Type: character, building, terrain, prop, vehicle") assetType: String
    ): String {
        notifyWebView("GENERATE_3D", """{"description":"$description","type":"$assetType"}""")
        return "Generating $assetType: '$description' via AI pipeline..."
    }

    @Tool(description = "Generate a 2D sprite or texture from text prompt")
    fun generateSprite(
        @ToolParam(description = "Description of the sprite/texture") description: String,
        @ToolParam(description = "Width in pixels") width: Int,
        @ToolParam(description = "Height in pixels") height: Int,
        @ToolParam(description = "Style (e.g., pixel art, realistic, cartoon)") style: String
    ): String {
        notifyWebView("GENERATE_SPRITE", """{"description":"$description","width":$width,"height":$height,"style":"$style"}""")
        return "Sprite generation requested: '$description' (${width}x${height}, $style)"
    }

    @Tool(description = "Generate an animated character with skeleton and animations")
    fun generateAnimatedCharacter(
        @ToolParam(description = "Description of the character") description: String,
        @ToolParam(description = "Animation set (idle, walk, run, attack, jump)") animations: String
    ): String {
        notifyWebView("GENERATE_ANIMATED_CHAR", """{"description":"$description","animations":"$animations"}""")
        return "Animated character generation started: '$description' with animations [$animations]"
    }

    @Tool(description = "Create an NPC with AI behavior tree")
    fun createNPC(
        @ToolParam(description = "NPC name") name: String,
        @ToolParam(description = "NPC role (merchant, guard, quest_giver, enemy)") role: String,
        @ToolParam(description = "Initial dialogue or behavior description") behavior: String,
        @ToolParam(description = "X position") x: Float,
        @ToolParam(description = "Y position") y: Float,
        @ToolParam(description = "Z position") z: Float
    ): String {
        notifyWebView("CREATE_NPC", """{"name":"$name","role":"$role","behavior":"$behavior","x":$x,"y":$y,"z":$z}""")
        return "NPC '$name' ($role) created at ($x,$y,$z) with behavior: $behavior"
    }

    @Tool(description = "Generate a massive open world map with thousands of objects")
    fun generateWorldMap(
        @ToolParam(description = "Description of the world") description: String,
        @ToolParam(description = "World size in kilometers") sizeKm: Float,
        @ToolParam(description = "Object density: low, medium, high, ultra") density: String
    ): String {
        notifyWebView("GENERATE_WORLD_MAP", """{"description":"$description","sizeKm":$sizeKm,"density":"$density"}""")
        return "Generating ${sizeKm}x${sizeKm} km world: '$description' (density: $density)..."
    }

    @Tool(description = "Generate a massive 100x100 km open world")
    fun generateMassiveWorld(
        @ToolParam(description = "World theme (e.g., medieval fantasy, sci-fi, realistic)") theme: String,
        @ToolParam(description = "Size in kilometers (default 100)") sizeKm: Int
    ): String {
        notifyWebView("GENERATE_MASSIVE_WORLD", """{"theme":"$theme","sizeKm":$sizeKm}""")
        return "Generating ${sizeKm}x${sizeKm} km world with theme '$theme'..."
    }

    private fun notifyWebView(action: String, data: String) {
        LiteRTManager.webViewRef?.get()?.let { wv ->
            wv.post {
                val escaped = data.replace("'", "\\'").replace("\n", "\\n")
                wv.evaluateJavascript(
                    "window.onAriesCommand && window.onAriesCommand('$action', '$escaped')",
                    null
                )
            }
        }
    }
}

object NeoEngineBridgeNative {
    @JvmStatic external fun nativeAddActor(type: String, name: String, x: Float, y: Float, z: Float): Int
    @JvmStatic external fun nativeDeleteActor(name: String): Boolean
    @JvmStatic external fun nativeSetTransform(name: String, px: Float, py: Float, pz: Float, rx: Float, ry: Float, rz: Float, sx: Float, sy: Float, sz: Float)
    @JvmStatic external fun nativeGetSceneJSON(): String
    @JvmStatic external fun nativeGetTelemetryJSON(): String
}

class LiteRTManager(private val context: Context) {
    private var engine: Engine? = null
    private var conversation: Conversation? = null
    private val scope = CoroutineScope(Dispatchers.IO + SupervisorJob())

    companion object {
        var webViewRef: java.lang.ref.WeakReference<android.webkit.WebView>? = null

        val MODEL_PATHS = listOf(
            "/sdcard/Gemma4/gemma4_2b_v09_obfus_fix_all_modalities_thinking.litertlm",
            "/sdcard/Gemma4/",
            "/sdcard/Android/data/com.google.ai.edge.gallery/files/Gemma_4_E2B_it/"
        )

        val SYSTEM_PROMPT = """
            Kamu adalah ARIES, AI brain dari NeoEngine buatan Fauzan.
            KEMAMPUAN:
            - addActor: tambah object 3D ke scene
            - setTransform: pindahkan/rotate/scale object
            - deleteActor: hapus object dari scene
            - setMaterial: ubah warna/material object
            - generateCode: buat kode C++ gameplay
            - createGameWorld: buat dunia game lengkap
            - generate3DAsset: buat model 3D realistis
            - generateSprite: buat sprite 2D / tekstur
            - generateAnimatedCharacter: buat karakter dengan animasi
            - createNPC: buat NPC dengan behavior
            - generateWorldMap: buat dunia dengan ribuan objek
            - generateMassiveWorld: buat dunia 100x100 km
            Cara kerja:
            - Dengarkan perintah user dalam Bahasa Indonesia
            - Gunakan tools untuk manipulasi scene secara langsung
            - Selalu konfirmasi action yang sudah dilakukan
        """.trimIndent()
    }

    suspend fun initialize(modelPath: String = "", enableTools: Boolean = true): Boolean = withContext(Dispatchers.IO) {
        try {
            val actualPath = when {
                modelPath.isNotEmpty() && java.io.File(modelPath).exists() -> modelPath
                else -> MODEL_PATHS.firstOrNull { p ->
                    java.io.File(p).let { f ->
                        f.exists() && (f.isFile || f.list()?.any { it.endsWith(".litertlm") || it.endsWith(".bin") } == true)
                    }
                }
            }
            if (actualPath == null) {
                Log.e("LiteRTManager", "Model not found in any path")
                return@withContext false
            }
            Log.i("LiteRTManager", "Loading model from: $actualPath")

            val convConfigBuilder = ConversationConfig.Builder()
                .setSystemInstruction(Contents.of(SYSTEM_PROMPT))
                .setSamplerConfig(SamplerConfig.Builder().setTopK(40).setTopP(0.95f).setTemperature(0.7f).build())
            if (enableTools) {
                convConfigBuilder.addToolSet(NeoEngineToolSet())
            }
            val engineConfig = EngineConfig.Builder()
                .setModelPath(actualPath)
                .setBackend(Backend.CPU())
                .setCacheDir(context.cacheDir.absolutePath)
                .setConversationConfig(convConfigBuilder.build())
                .build()

            engine = Engine(engineConfig).also { it.initialize() }
            conversation = engine?.createConversation()
            Log.i("LiteRTManager", "Aries Brain ready with Gemma4!")
            true
        } catch (e: Exception) {
            Log.e("LiteRTManager", "Init failed: ${e.message}", e)
            false
        }
    }

    suspend fun sendMessageSync(prompt: String): String = withContext(Dispatchers.IO) {
        try { conversation?.sendMessage(prompt)?.text ?: "Aries belum siap" }
        catch (e: Exception) { "Error: ${e.message}" }
    }

    fun sendMessageStream(prompt: String): Flow<String> {
        return try { conversation?.sendMessageAsync(prompt)?.map { it.text } ?: flowOf("Aries belum siap") }
        catch (e: Exception) { flowOf("Error: ${e.message}") }
    }

    fun resetConversation() {
        scope.launch {
            try { conversation?.close(); conversation = engine?.createConversation() }
            catch (e: Exception) { Log.e("LiteRTManager", "Reset failed: ${e.message}") }
        }
    }

    fun shutdown() {
        scope.cancel()
        try { conversation?.close(); engine?.close() }
        catch (e: Exception) { Log.w("LiteRTManager", "Shutdown error: ${e.message}") }
        conversation = null; engine = null
    }
}
