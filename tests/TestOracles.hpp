#pragma once

#include "core/Bitset.hpp"
#include "core/Types.hpp"
#include "cut/BfsEngine.hpp"
#include "graph/Graph.hpp"

#include <algorithm>
#include <limits>
#include <queue>
#include <vector>

namespace apdfs
{
namespace test
{

inline size_t maxFlowEdmondsKarp(const Graph& graph, VertexIndex source, VertexIndex sink)
{
    size_t n = graph.vertexCount();
    size_t m = graph.edgeCount();

    std::vector<int64_t> cap(m, 1);
    std::vector<int64_t> flow(m, 0);
    std::vector<EdgeIndex> parent(n);
    std::vector<EdgeIndex> parentEdge(n);

    int64_t maxFlow = 0;

    while (true)
    {
        std::fill(parent.begin(), parent.end(), static_cast<EdgeIndex>(-1));
        std::queue<VertexIndex> q;
        q.push(source);
        parent[source] = source;

        while (!q.empty())
        {
            VertexIndex u = q.front();
            q.pop();

            for (EdgeIndex e : graph.outgoingEdges(u))
            {
                VertexIndex v = graph.edgeTarget(e);
                if (parent[v] == static_cast<EdgeIndex>(-1) && cap[e] - flow[e] > 0)
                {
                    parent[v] = u;
                    parentEdge[v] = e;
                    q.push(v);
                    if (v == sink)
                    {
                        break;
                    }
                }
            }
        }

        if (parent[sink] == static_cast<EdgeIndex>(-1))
        {
            break;
        }

        int64_t push = std::numeric_limits<int64_t>::max();
        for (VertexIndex v = sink; v != source; v = parent[v])
        {
            EdgeIndex e = parentEdge[v];
            push = std::min(push, cap[e] - flow[e]);
        }

        for (VertexIndex v = sink; v != source; v = parent[v])
        {
            EdgeIndex e = parentEdge[v];
            flow[e] += push;
        }

        maxFlow += push;
    }

    return static_cast<size_t>(maxFlow);
}

inline bool isValidCut(const Graph& graph, VertexIndex source, VertexIndex sink, const std::vector<EdgeIndex>& cut)
{
    DynamicBitset forbidden(graph.edgeCount());
    for (EdgeIndex e : cut)
    {
        forbidden.set(e);
    }

    BfsEngine bfs(graph.vertexCount(), graph.edgeCount());
    std::vector<EdgeIndex> dummy;
    bool reachedSink = bfs.forwardBfs(source, sink, graph, forbidden, dummy);
    return !reachedSink;
}

inline bool allCutsUnique(std::vector<std::vector<EdgeIndex>> cuts)
{
    for (auto& cut : cuts)
    {
        std::sort(cut.begin(), cut.end());
    }
    std::sort(cuts.begin(), cuts.end());
    for (size_t i = 1; i < cuts.size(); ++i)
    {
        if (cuts[i - 1] == cuts[i])
        {
            return false;
        }
    }
    return true;
}

} // namespace test
} // namespace apdfs
