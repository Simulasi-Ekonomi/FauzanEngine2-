#include "Rendering/Rendering/RenderGraph/RenderGraph.h"
#include <cstdio>
#include <string>
#include <vector>

int main() {
    using namespace NeoEngine;
    std::vector<std::string> executed;
    RenderGraph graph;
    RenderPass post("post", [&] { executed.emplace_back("post"); });
    post.reads.push_back("lit");
    post.writes.push_back("present");
    RenderPass lit("lit", [&] { executed.emplace_back("lit"); });
    lit.reads.push_back("gbuffer");
    lit.writes.push_back("lit");
    RenderPass gbuffer("gbuffer", [&] { executed.emplace_back("gbuffer"); });
    gbuffer.writes.push_back("gbuffer");
    graph.add(post); graph.add(lit); graph.add(gbuffer);
    if (!graph.executeChecked() || executed != std::vector<std::string>{"gbuffer", "lit", "post"}) return 1;
    graph.clear();
    RenderPass a("a", [] {}); RenderPass b("b", [] {});
    a.reads.push_back("b"); a.writes.push_back("a");
    b.reads.push_back("a"); b.writes.push_back("b");
    graph.add(a); graph.add(b);
    if (graph.executeChecked() || !graph.executionOrder().empty()) return 2;
    std::puts("RENDER_GRAPH_SMOKE_OK dependency_order=1 cycle_reject=1 clear=1");
    return 0;
}
