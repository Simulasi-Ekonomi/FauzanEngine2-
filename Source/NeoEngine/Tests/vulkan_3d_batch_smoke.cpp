#include "../Runtime/Vulkan3DBatch.h"

#include <cassert>

int main() {
    using namespace NeoEngine;

    Vulkan3DBatchBuilder builder;
    assert(builder.Append(4, 6, 3));
    assert(builder.Append(8, 12, 5));

    assert(builder.VertexCount() == 12);
    assert(builder.IndexCount() == 18);
    assert(builder.InstanceCount() == 8);
    assert(builder.Commands().size() == 2);
    assert(builder.Ranges().size() == 2);

    const auto first = builder.Commands()[0];
    assert(first.indexCount == 6);
    assert(first.instanceCount == 3);
    assert(first.firstIndex == 0);
    assert(first.vertexOffset == 0);
    assert(first.firstInstance == 0);

    const auto second = builder.Commands()[1];
    assert(second.indexCount == 12);
    assert(second.instanceCount == 5);
    assert(second.firstIndex == 6);
    assert(second.vertexOffset == 4);
    assert(second.firstInstance == 3);

    assert(!builder.Append(0, 1, 1));
    assert(!builder.Append(1, 0, 1));
    assert(!builder.Append(1, 1, 0));

    builder.Reset();
    assert(builder.Commands().empty());
    assert(builder.Ranges().empty());
    assert(builder.VertexCount() == 0);
    assert(builder.IndexCount() == 0);
    assert(builder.InstanceCount() == 0);

    return 0;
}
