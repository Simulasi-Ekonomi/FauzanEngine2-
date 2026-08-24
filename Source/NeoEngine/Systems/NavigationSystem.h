#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <queue>

namespace NeoEngine {

struct NavNode {
    int id;
    float x, y, z;
    std::vector<int> neighbors;
};

struct NavPath {
    std::vector<NavNode> nodes;
    float totalDistance = 0;
    bool valid = false;
};

class NavigationSystem {
private:
    std::vector<NavNode> m_Nodes;
    std::unordered_map<int, int> m_NodeMap;
    int m_NextId = 1;
    
public:
    int AddNode(float x, float y, float z) {
        NavNode node{m_NextId, x, y, z, {}};
        m_Nodes.push_back(node);
        m_NodeMap[m_NextId] = m_Nodes.size() - 1;
        return m_NextId++;
    }
    
    void ConnectNodes(int id1, int id2) {
        auto* n1 = GetNode(id1);
        auto* n2 = GetNode(id2);
        if (n1 && n2) {
            n1->neighbors.push_back(id2);
            n2->neighbors.push_back(id1);
        }
    }
    
    NavNode* GetNode(int id) {
        auto it = m_NodeMap.find(id);
        if (it != m_NodeMap.end() && it->second < m_Nodes.size()) return &m_Nodes[it->second];
        return nullptr;
    }
    
    NavPath FindPath(int startId, int endId) {
        NavPath path;
        if (startId == endId) { path.valid = true; return path; }
        
        std::unordered_map<int, int> cameFrom;
        std::unordered_map<int, float> costSoFar;
        std::priority_queue<std::pair<float, int>, std::vector<std::pair<float, int>>, std::greater<>> frontier;
        
        frontier.push({0, startId});
        costSoFar[startId] = 0;
        
        while (!frontier.empty()) {
            auto current = frontier.top().second;
            frontier.pop();
            
            if (current == endId) {
                // Reconstruct path
                path.valid = true;
                std::vector<int> pathIds;
                for (int id = endId; id != startId; id = cameFrom[id]) {
                    pathIds.push_back(id);
                }
                pathIds.push_back(startId);
                std::reverse(pathIds.begin(), pathIds.end());
                for (int id : pathIds) {
                    auto* node = GetNode(id);
                    if (node) path.nodes.push_back(*node);
                }
                for (size_t i = 1; i < path.nodes.size(); i++) {
                    float dx = path.nodes[i].x - path.nodes[i-1].x;
                    float dy = path.nodes[i].y - path.nodes[i-1].y;
                    float dz = path.nodes[i].z - path.nodes[i-1].z;
                    path.totalDistance += sqrt(dx*dx + dy*dy + dz*dz);
                }
                return path;
            }
            
            auto* node = GetNode(current);
            if (!node) continue;
            
            for (int neighbor : node->neighbors) {
                auto* n = GetNode(neighbor);
                if (!n) continue;
                float dx = n->x - node->x, dy = n->y - node->y, dz = n->z - node->z;
                float newCost = costSoFar[current] + sqrt(dx*dx + dy*dy + dz*dz);
                if (costSoFar.find(neighbor) == costSoFar.end() || newCost < costSoFar[neighbor]) {
                    costSoFar[neighbor] = newCost;
                    cameFrom[neighbor] = current;
                    frontier.push({newCost, neighbor});
                }
            }
        }
        return path;
    }
    
    const std::vector<NavNode>& GetNodes() const { return m_Nodes; }
    void Clear() { m_Nodes.clear(); m_NodeMap.clear(); m_NextId = 1; }
};

} // namespace NeoEngine
