#pragma once
#include <cstdint>
#include <deque>
#include <string>
#include <string_view>
#include <vector>
namespace NeoEngine {
enum class AssetKind : uint8_t { Texture, Mesh, Material, Prefab, Scene, Audio };
enum class AssetState : uint8_t { Declared, Ready };
enum class AssetRegistryError : uint8_t { None, InvalidIdentifier, DuplicateId, MissingDependency, CapacityReached, InvalidState, ByteLimitExceeded, MissingAsset };
struct AssetDefinition { std::string id; AssetKind kind; std::vector<std::string> dependencies; AssetState state = AssetState::Declared; uint64_t contentHash=0; uint32_t byteSize=0; };
class AssetRegistry { public: static constexpr size_t kMaxAssets=4096, kMaxDependencies=16,kMaxAssetBytes=16*1024*1024,kMaxStoredBytes=64*1024*1024; bool Declare(std::string id,AssetKind kind,std::vector<std::string> dependencies); bool ImportBytes(std::string id,AssetKind kind,std::vector<std::string> dependencies,std::vector<uint8_t> bytes); bool ReplaceBytes(std::string_view id,std::vector<uint8_t> bytes); bool MarkReady(std::string_view id); const AssetDefinition* Find(std::string_view id) const; const std::vector<uint8_t>* Data(std::string_view id)const; std::vector<AssetDefinition> All() const { return {m_Assets.begin(),m_Assets.end()}; } AssetRegistryError LastError() const{return m_LastError;} private: static bool ValidId(std::string_view id); static uint64_t Hash(const std::vector<uint8_t>&bytes); std::deque<AssetDefinition> m_Assets; std::deque<std::vector<uint8_t>>m_Bytes; size_t m_StoredBytes=0; AssetRegistryError m_LastError=AssetRegistryError::None; };
} // namespace NeoEngine
