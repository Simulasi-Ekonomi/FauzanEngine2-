#include "EditorAgentAPI.h"
#include <json/json.h>
#include <memory>
#include <algorithm>

namespace NeoEngine {

std::string EditorAgentAPI::SerializeSceneToJSON(const EditorSceneSession& session) {
    const auto hierarchy = session.HierarchySnapshot();
    Json::Value root;
    root["sceneId"] = session.Document().sceneId;
    root["revision"] = static_cast<Json::UInt64>(session.Document().revision);
    root["selectedActorId"] = session.SelectedActorId();
    root["hasUnsavedChanges"] = session.HasUnsavedChanges();
    root["actorCount"] = static_cast<Json::UInt64>(hierarchy.size());

    Json::Value actorsArray(Json::arrayValue);
    for (const auto& a : hierarchy) {
        Json::Value actorVal;
        actorVal["id"] = a.id;
        actorVal["parentId"] = a.parentId;
        actorVal["kind"] = static_cast<int>(a.kind);
        actorVal["name"] = a.name;
        actorVal["assetId"] = a.assetId;
        actorVal["materialAssetId"] = a.materialAssetId;
        actorVal["textureAssetId"] = a.textureAssetId;

        Json::Value tf;
        tf["x"] = a.transform.x;
        tf["y"] = a.transform.y;
        tf["z"] = a.transform.z;
        actorVal["transform"] = tf;

        actorsArray.append(actorVal);
    }
    root["actors"] = actorsArray;

    Json::StreamWriterBuilder writerBuilder;
    writerBuilder["indentation"] = "";
    return Json::writeString(writerBuilder, root);
}

EditorAgentResponse EditorAgentAPI::ProcessRequest(EditorSceneSession& session, const AssetRegistry& assets, const EditorAgentRequest& req) {
    EditorAgentResponse resp;
    bool ok = false;

    switch (req.commandType) {
        case EditorAgentCommandType::SpawnActor: {
            EditorSceneActor actor;
            actor.id = req.actorId;
            actor.parentId = req.parentId;
            actor.kind = req.kind;
            actor.transform = req.transform;
            actor.name = req.name;
            actor.assetId = req.assetId;
            actor.materialAssetId = req.materialAssetId;
            actor.textureAssetId = req.textureAssetId;
            actor.spriteRgba = req.spriteRgba;
            ok = session.AddActor(actor, assets);
            break;
        }
        case EditorAgentCommandType::DeleteActor:
            ok = session.DeleteActor(req.actorId, assets);
            break;

        case EditorAgentCommandType::DuplicateActor:
            ok = session.DuplicateActor(req.actorId, req.newActorId, assets);
            break;

        case EditorAgentCommandType::SetTransform:
            ok = session.UpdateTransform(req.actorId, req.transform, assets);
            break;

        case EditorAgentCommandType::ReparentActor:
            ok = session.ReparentActor(req.actorId, req.parentId, assets);
            break;

        case EditorAgentCommandType::SelectActor:
            if (req.selectionIds.size() > 1) {
                ok = session.MultiSelectActors(req.selectionIds);
            } else {
                ok = session.SelectActor(req.actorId);
            }
            break;

        case EditorAgentCommandType::SetActorProperties:
            ok = session.UpdateActorProperties(req.actorId, req.name, req.materialAssetId, req.textureAssetId, req.spriteRgba, assets);
            break;

        case EditorAgentCommandType::SaveScene: {
            EditorSceneDocument doc;
            ok = session.Save(doc);
            break;
        }
        case EditorAgentCommandType::Undo:
            ok = session.Undo(assets);
            break;

        case EditorAgentCommandType::Redo:
            ok = session.Redo(assets);
            break;

        case EditorAgentCommandType::QueryScene:
            ok = true;
            break;

        default:
            resp.error = "Unknown command type";
            return resp;
    }

    resp.success = ok;
    if (!ok) {
        resp.error = "Session mutation failed";
    }
    resp.sceneRevision = session.Document().revision;
    resp.selectedActorId = session.SelectedActorId();
    resp.actorCount = session.HierarchySnapshot().size();
    resp.jsonPayload = SerializeSceneToJSON(session);
    return resp;
}

EditorAgentResponse EditorAgentAPI::ProcessJsonCommand(EditorSceneSession& session, const AssetRegistry& assets, const std::string& json) {
    EditorAgentResponse resp;
    if (json.empty()) {
        resp.success = false;
        resp.error = "Empty JSON payload";
        return resp;
    }

    Json::Value root;
    Json::CharReaderBuilder builder;
    std::string errs;
    std::unique_ptr<Json::CharReader> reader(builder.newCharReader());

    if (!reader->parse(json.c_str(), json.c_str() + json.size(), &root, &errs)) {
        resp.success = false;
        resp.error = "Malformed JSON: " + errs;
        return resp;
    }

    if (!root.isObject()) {
        resp.success = false;
        resp.error = "JSON root must be an object";
        return resp;
    }

    EditorAgentRequest req;
    std::string cmd = root.get("cmd", "").asString();

    if (cmd == "SPAWN") req.commandType = EditorAgentCommandType::SpawnActor;
    else if (cmd == "DELETE") req.commandType = EditorAgentCommandType::DeleteActor;
    else if (cmd == "DUPLICATE") req.commandType = EditorAgentCommandType::DuplicateActor;
    else if (cmd == "TRANSFORM") req.commandType = EditorAgentCommandType::SetTransform;
    else if (cmd == "REPARENT") req.commandType = EditorAgentCommandType::ReparentActor;
    else if (cmd == "SELECT") req.commandType = EditorAgentCommandType::SelectActor;
    else if (cmd == "PROPERTIES") req.commandType = EditorAgentCommandType::SetActorProperties;
    else if (cmd == "SAVE") req.commandType = EditorAgentCommandType::SaveScene;
    else if (cmd == "UNDO") req.commandType = EditorAgentCommandType::Undo;
    else if (cmd == "REDO") req.commandType = EditorAgentCommandType::Redo;
    else if (cmd == "QUERY") req.commandType = EditorAgentCommandType::QueryScene;
    else {
        resp.success = false;
        resp.error = "Unknown command: " + cmd;
        return resp;
    }

    if (root.isMember("actorId")) req.actorId = root["actorId"].asUInt();
    if (root.isMember("newActorId")) req.newActorId = root["newActorId"].asUInt();
    if (root.isMember("parentId")) req.parentId = root["parentId"].asUInt();
    if (root.isMember("kind")) req.kind = static_cast<EditorSceneActorKind>(root["kind"].asUInt());

    if (root.isMember("name")) req.name = root["name"].asString();
    if (root.isMember("assetId")) req.assetId = root["assetId"].asString();
    if (root.isMember("materialAssetId")) req.materialAssetId = root["materialAssetId"].asString();
    if (root.isMember("textureAssetId")) req.textureAssetId = root["textureAssetId"].asString();

    if (root.isMember("x")) req.transform.x = root["x"].asFloat();
    if (root.isMember("y")) req.transform.y = root["y"].asFloat();
    if (root.isMember("z")) req.transform.z = root["z"].asFloat();

    if (root.isMember("selectionIds") && root["selectionIds"].isArray()) {
        for (const auto& item : root["selectionIds"]) {
            if (item.isUInt()) {
                req.selectionIds.push_back(item.asUInt());
            }
        }
    }

    return ProcessRequest(session, assets, req);
}

} // namespace NeoEngine
