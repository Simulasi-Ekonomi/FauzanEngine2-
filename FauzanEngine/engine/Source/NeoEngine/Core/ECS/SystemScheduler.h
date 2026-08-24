#pragma once

#include <vector>
#include <functional>
#include <unordered_map>
#include <string>
#include <queue>
#include <atomic>

#include "../Threading/ThreadPool.h"

namespace NeoEngine {

class SystemScheduler {
public:

    int RegisterSystem(const std::function<void()>& func)
    {
        int id = nextID++;
        systems[id] = func;
        return id;
    }

    void AddDependency(int system, int dependsOn)
    {
        graph[dependsOn].push_back(system);
        indegree[system]++;
    }

    void Execute()
    {
        std::queue<int> ready;

        for (auto& [id, func] : systems)
        {
            if (indegree[id] == 0)
                ready.push(id);
        }

        std::atomic<int> activeJobs = 0;

        while (!ready.empty())
        {
            int current = ready.front();
            ready.pop();

            activeJobs++;

            threadPool.Enqueue([&, current]()
            {
                systems[current]();

                for (int next : graph[current])
                {
                    if (--indegree[next] == 0)
                    {
                        std::lock_guard<std::mutex> lock(queueMutex);
                        ready.push(next);
                    }
                }

                activeJobs--;
            });
        }

        // Wait until all jobs selesai
        while (activeJobs > 0) {}

        threadPool.Wait();
    }

private:

    std::unordered_map<int, std::function<void()>> systems;
    std::unordered_map<int, std::vector<int>> graph;
    std::unordered_map<int, int> indegree;

    int nextID = 0;

    ThreadPool threadPool;

    std::mutex queueMutex;
};

} // namespace NeoEngine
