#include "Rendering/Rendering/FrameGraph/FrameGraph.h"
#include <cstdio>
#include <string>
#include <vector>

int main() {
    using namespace NeoEngine;
    std::vector<std::string> executed;
    FrameGraph graph;
    FrameGraphPass lighting("lighting", [&] { executed.emplace_back("lighting"); });
    lighting.addInput("gbuffer");
    lighting.addOutput("lit");
    FrameGraphPass gbuffer("gbuffer", [&] { executed.emplace_back("gbuffer"); });
    gbuffer.addOutput("gbuffer");
    FrameGraphPass present("present", [&] { executed.emplace_back("present"); });
    present.addInput("lit");
    graph.addPass(lighting);
    graph.addPass(present);
    graph.addPass(gbuffer);
    if (!graph.executeChecked() || executed.size() != 3 || executed[0] != "gbuffer" || executed[1] != "lighting" || executed[2] != "present") return 1;
    if (graph.executionOrder().size() != 3 || graph.executionOrder()[0] != "gbuffer") return 2;
    graph.clear();
    FrameGraphPass a("a", [] {});
    FrameGraphPass b("b", [] {});
    a.addInput("b_out"); a.addOutput("a_out");
    b.addInput("a_out"); b.addOutput("b_out");
    graph.addPass(a); graph.addPass(b);
    if (graph.executeChecked() || !graph.executionOrder().empty()) return 3;
    std::puts("FRAME_GRAPH_SMOKE_OK dependency_order=1 deterministic=1 cycle_reject=1 clear=1");
    return 0;
}
