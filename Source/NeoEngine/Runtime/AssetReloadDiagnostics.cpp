#include "AssetReloadDiagnostics.h"

#include <algorithm>

namespace NeoEngine {
bool AssetReloadDiagnostics::BuildPlan(const AssetRegistry& registry,std::string_view changedId){const std::vector<AssetDefinition> assets=registry.All();if(!registry.Find(changedId)){lastError_=AssetReloadDiagnosticsError::MissingAsset;return false;}affectedIds_.clear();affectedIds_.push_back(std::string(changedId));for(size_t cursor=0;cursor<affectedIds_.size();++cursor){const std::string current=affectedIds_[cursor];for(const AssetDefinition& asset:assets){if(std::find(asset.dependencies.begin(),asset.dependencies.end(),current)==asset.dependencies.end())continue;if(std::find(affectedIds_.begin(),affectedIds_.end(),asset.id)!=affectedIds_.end())continue;if(affectedIds_.size()>=AssetRegistry::kMaxAssets){affectedIds_.clear();lastError_=AssetReloadDiagnosticsError::Capacity;return false;}affectedIds_.push_back(asset.id);}}for(const std::string& id:affectedIds_){const AssetDefinition* entry=registry.Find(id);if(!entry){affectedIds_.clear();lastError_=AssetReloadDiagnosticsError::DependencyGraphInvalid;return false;}for(const std::string& dependency:entry->dependencies)if(!registry.Find(dependency)){affectedIds_.clear();lastError_=AssetReloadDiagnosticsError::DependencyGraphInvalid;return false;}}lastError_=AssetReloadDiagnosticsError::None;return true;}
} // namespace NeoEngine
