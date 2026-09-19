#include "Runtime/EditorSceneAgentAPI.h"

#include <json/json.h>

#include <cmath>
#include <set>
#include <sstream>
#include <vector>

namespace NeoEngine {
namespace {

bool ReadFinite(const Json::Value& value, float& out) {
    if (!value.isNumeric()) return false;
    out = value.asFloat();
    return std::isfinite(out);
}

bool HasOnly(const Json::Value& object, std::initializer_list<const char*> allowed) {
    std::set<std::string> names;
    for (const char* name : allowed) names.emplace(name);
    for (const auto& name : object.getMemberNames()) if (!names.contains(name)) return false;
    return true;
}

bool ReadTransform(const Json::Value& value, Transform3& out) {
    if (!value.isObject() || !HasOnly(value, {"x", "y", "z", "rx", "ry", "rz", "sx", "sy", "sz"})) return false;
    return ReadFinite(value["x"], out.x) && ReadFinite(value["y"], out.y) &&
           ReadFinite(value["z"], out.z) && ReadFinite(value["rx"], out.rx) &&
           ReadFinite(value["ry"], out.ry) && ReadFinite(value["rz"], out.rz) &&
           ReadFinite(value["sx"], out.sx) && ReadFinite(value["sy"], out.sy) &&
           ReadFinite(value["sz"], out.sz);
}

std::string SceneResult(const char* operation, const EditorSceneSession& session) {
    Json::Value root(Json::objectValue);
    root["ok"] = true;
    root["operation"] = operation;
    root["revision"] = Json::UInt64(session.HierarchySnapshot().empty() ? 0U : session.HierarchySnapshot().size());
    root["actorCount"] = Json::UInt64(session.HierarchySnapshot().size());
    root["selectedActorId"] = session.SelectedActorId();
    Json::Value actors(Json::arrayValue);
    for (const auto& actor : session.HierarchySnapshot()) {
        Json::Value item(Json::objectValue);
        item["id"] = actor.id; item["parentId"] = actor.parentId; item["name"] = actor.name;
        item["assetId"] = actor.assetId; item["materialAssetId"] = actor.materialAssetId; item["textureAssetId"] = actor.textureAssetId;
        actors.append(item);
    }
    root["actors"] = actors;
    Json::StreamWriterBuilder builder; builder["indentation"] = "";
    return Json::writeString(builder, root);
}

std::string Result(const char* operation) {
    Json::Value root(Json::objectValue);
    root["ok"] = true;
    root["operation"] = operation;
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    return Json::writeString(builder, root);
}

} // namespace

bool EditorSceneAgentAPI::Fail(EditorAgentError error, std::string& response) const {
    lastError_ = error;
    Json::Value root(Json::objectValue);
    root["ok"] = false;
    root["error"] = static_cast<unsigned>(error);
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    response = Json::writeString(builder, root);
    return false;
}

bool EditorSceneAgentAPI::Execute(std::string_view request, EditorSceneSession& session, const AssetRegistry& assets, std::string& response) const {
    response.clear();
    if (request.empty() || request.size() > 64U * 1024U) return Fail(EditorAgentError::EmptyRequest, response);

    Json::CharReaderBuilder readerBuilder;
    Json::Value root;
    std::string errors;
    std::istringstream input{std::string(request)};
    if (!Json::parseFromStream(readerBuilder, input, &root, &errors)) return Fail(EditorAgentError::InvalidJson, response);
    if (!root.isObject()) return Fail(EditorAgentError::RootNotObject, response);
    if (!root.isMember("operation") || !root["operation"].isString()) return Fail(EditorAgentError::MissingOperation, response);

    const std::string operation = root["operation"].asString();
    if (operation == "spawn") {
        if (!HasOnly(root, {"operation", "actorId", "parentId", "kind", "name", "assetId", "materialAssetId", "textureAssetId", "x", "y", "z"}) ||
            !root["actorId"].isUInt() || !root["parentId"].isUInt() || !root["kind"].isUInt() ||
            !root["name"].isString() || !root["assetId"].isString() || !root["materialAssetId"].isString() || !root["textureAssetId"].isString()) return Fail(EditorAgentError::InvalidArgument, response);
        EditorSceneActor actor{};
        actor.id = root["actorId"].asUInt(); actor.parentId = root["parentId"].asUInt();
        actor.kind = static_cast<EditorSceneActorKind>(root["kind"].asUInt()); actor.name = root["name"].asString();
        actor.assetId = root["assetId"].asString(); actor.materialAssetId = root["materialAssetId"].asString(); actor.textureAssetId = root["textureAssetId"].asString();
        if (root.isMember("x")) actor.transform.x = root["x"].asFloat();
        if (root.isMember("y")) actor.transform.y = root["y"].asFloat();
        if (root.isMember("z")) actor.transform.z = root["z"].asFloat();
        if (!session.AddActor(actor, assets)) return Fail(EditorAgentError::OperationFailed, response);
        lastError_ = EditorAgentError::None; response = SceneResult("spawn", session); return true;
    }
    if (operation == "duplicate") {
        if (!HasOnly(root, {"operation", "actorId", "newActorId"}) || !root["actorId"].isUInt() || !root["newActorId"].isUInt()) return Fail(EditorAgentError::InvalidArgument, response);
        if (!session.DuplicateActor(root["actorId"].asUInt(), root["newActorId"].asUInt(), assets)) return Fail(EditorAgentError::OperationFailed, response);
        lastError_ = EditorAgentError::None; response = SceneResult("duplicate", session); return true;
    }
    if (operation == "properties") {
        if (!HasOnly(root, {"operation", "actorId", "name", "materialAssetId", "textureAssetId", "spriteRgba"}) ||
            !root["actorId"].isUInt() || !root["name"].isString() || !root["materialAssetId"].isString() || !root["textureAssetId"].isString() || !root["spriteRgba"].isUInt()) return Fail(EditorAgentError::InvalidArgument, response);
        if (!session.UpdateActorProperties(root["actorId"].asUInt(), root["name"].asString(), root["materialAssetId"].asString(), root["textureAssetId"].asString(), root["spriteRgba"].asUInt(), assets)) return Fail(EditorAgentError::OperationFailed, response);
        lastError_ = EditorAgentError::None; response = SceneResult("properties", session); return true;
    }
    if (operation == "selectMany") {
        if (!HasOnly(root, {"operation", "actorIds"}) || !root["actorIds"].isArray()) return Fail(EditorAgentError::InvalidArgument, response);
        std::vector<uint32_t> ids;
        for (const auto& item : root["actorIds"]) { if (!item.isUInt()) return Fail(EditorAgentError::InvalidArgument, response); ids.push_back(item.asUInt()); }
        if (!session.MultiSelectActors(ids)) return Fail(EditorAgentError::OperationFailed, response);
        lastError_ = EditorAgentError::None; response = SceneResult("selectMany", session); return true;
    }
    if (operation == "query") {
        if (!HasOnly(root, {"operation"})) return Fail(EditorAgentError::UnknownField, response);
        lastError_ = EditorAgentError::None; response = SceneResult("query", session); return true;
    }
    if (operation == "select") {
        if (!HasOnly(root, {"operation", "actorId"}) || !root["actorId"].isUInt()) return Fail(EditorAgentError::InvalidArgument, response);
        if (!session.SelectActor(root["actorId"].asUInt())) return Fail(EditorAgentError::UnknownActor, response);
        lastError_ = EditorAgentError::None; response = Result("select"); return true;
    }
    if (operation == "transform") {
        if (!HasOnly(root, {"operation", "actorId", "transform"}) || !root["actorId"].isUInt()) return Fail(EditorAgentError::InvalidArgument, response);
        Transform3 transform;
        if (!ReadTransform(root["transform"], transform)) return Fail(EditorAgentError::InvalidArgument, response);
        if (!session.UpdateTransform(root["actorId"].asUInt(), transform, assets)) return Fail(EditorAgentError::OperationFailed, response);
        lastError_ = EditorAgentError::None; response = Result("transform"); return true;
    }
    if (operation == "reparent") {
        if (!HasOnly(root, {"operation", "actorId", "parentId"}) || !root["actorId"].isUInt() || !root["parentId"].isUInt()) return Fail(EditorAgentError::InvalidArgument, response);
        if (!session.ReparentActor(root["actorId"].asUInt(), root["parentId"].asUInt(), assets)) return Fail(EditorAgentError::OperationFailed, response);
        lastError_ = EditorAgentError::None; response = Result("reparent"); return true;
    }
    if (operation == "delete") {
        if (!HasOnly(root, {"operation", "actorId"}) || !root["actorId"].isUInt()) return Fail(EditorAgentError::InvalidArgument, response);
        if (!session.DeleteActor(root["actorId"].asUInt(), assets)) return Fail(EditorAgentError::OperationFailed, response);
        lastError_ = EditorAgentError::None; response = Result("delete"); return true;
    }
    if (operation == "undo") {
        if (!HasOnly(root, {"operation"}) || !session.Undo(assets)) return Fail(EditorAgentError::OperationFailed, response);
        lastError_ = EditorAgentError::None; response = Result("undo"); return true;
    }
    if (operation == "redo") {
        if (!HasOnly(root, {"operation"}) || !session.Redo(assets)) return Fail(EditorAgentError::OperationFailed, response);
        lastError_ = EditorAgentError::None; response = Result("redo"); return true;
    }
    if (operation == "save") {
        if (!HasOnly(root, {"operation"})) return Fail(EditorAgentError::UnknownField, response);
        EditorSceneDocument document;
        if (!session.Save(document)) return Fail(EditorAgentError::OperationFailed, response);
        lastError_ = EditorAgentError::None; response = Result("save"); return true;
    }
    return Fail(EditorAgentError::InvalidOperation, response);
}

} // namespace NeoEngine
