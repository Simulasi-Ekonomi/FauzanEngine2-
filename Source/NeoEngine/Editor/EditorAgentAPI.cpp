#include "EditorAgentAPI.h"
#include <sstream>
#include <algorithm>

namespace NeoEngine {

std::string EditorAgentAPI::SerializeSceneToJSON(const EditorSceneSession& session) {
    std::stringstream ss;
    const auto hierarchy = session.HierarchySnapshot();
    ss << "{\"sceneId\":\"" << session.Document().sceneId << "\",";
    ss << "\"revision\":" << session.Document().revision << ",";
    ss << "\"selectedActorId\":" << session.SelectedActorId() << ",";
    ss << "\"hasUnsavedChanges\":" << (session.HasUnsavedChanges() ? "true" : "false") << ",";
    ss << "\"actorCount\":" << hierarchy.size() << ",";
    ss << "\"actors\":[";

    for (size_t i = 0; i < hierarchy.size(); ++i) {
        const auto& a = hierarchy[i];
        if (i > 0) ss << ",";
        ss << "{\"id\":" << a.id << ",";
        ss << "\"parentId\":" << a.parentId << ",";
        ss << "\"kind\":" << static_cast<int>(a.kind) << ",";
        ss << "\"name\":\"" << a.name << "\",";
        ss << "\"assetId\":\"" << a.assetId << "\",";
        ss << "\"materialAssetId\":\"" << a.materialAssetId << "\",";
        ss << "\"textureAssetId\":\"" << a.textureAssetId << "\",";
        ss << "\"transform\":{\"x\":" << a.transform.x << ",\"y\":" << a.transform.y << ",\"z\":" << a.transform.z << "}}";
    }
    ss << "]}";
    return ss.str();
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
    EditorAgentRequest req;

    auto findVal = [&](const std::string& key) -> std::string {
        size_t pos = json.find("\"" + key + "\"");
        if (pos == std::string::npos) return "";
        size_t colon = json.find(":", pos);
        if (colon == std::string::npos) return "";
        size_t start = json.find_first_not_of(" \t\n\r", colon + 1);
        if (start == std::string::npos) return "";
        if (json[start] == '\"') {
            size_t end = json.find('\"', start + 1);
            if (end == std::string::npos) return "";
            return json.substr(start + 1, end - start - 1);
        } else {
            size_t end = json.find_first_of(" \t\n\r,}", start);
            if (end == std::string::npos) end = json.size();
            return json.substr(start, end - start);
        }
    };

    std::string cmd = findVal("cmd");
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

    std::string idStr = findVal("actorId");
    if (!idStr.empty()) req.actorId = static_cast<uint32_t>(std::stoul(idStr));

    std::string newIdStr = findVal("newActorId");
    if (!newIdStr.empty()) req.newActorId = static_cast<uint32_t>(std::stoul(newIdStr));

    std::string parentStr = findVal("parentId");
    if (!parentStr.empty()) req.parentId = static_cast<uint32_t>(std::stoul(parentStr));

    std::string kindStr = findVal("kind");
    if (!kindStr.empty()) req.kind = static_cast<EditorSceneActorKind>(std::stoul(kindStr));

    req.name = findVal("name");
    req.assetId = findVal("assetId");
    req.materialAssetId = findVal("materialAssetId");
    req.textureAssetId = findVal("textureAssetId");

    std::string xStr = findVal("x"); if (!xStr.empty()) req.transform.x = std::stof(xStr);
    std::string yStr = findVal("y"); if (!yStr.empty()) req.transform.y = std::stof(yStr);
    std::string zStr = findVal("z"); if (!zStr.empty()) req.transform.z = std::stof(zStr);

    return ProcessRequest(session, assets, req);
}

} // namespace NeoEngine
