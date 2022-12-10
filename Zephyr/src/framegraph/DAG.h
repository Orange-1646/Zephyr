#pragma once
#include "pch.h"

namespace Zephyr
{
    static const constexpr uint32_t TARGET = 0x80000000u;

    class Node
    {
    public:
        uint32_t    refcount = 0;
        std::string name;

        Node() {}

        Node(const std::string& name) : name(name)
        {
        }

        inline bool IsCulled() { return refcount == 0; }

        virtual ~Node() = default;

        void SideEffect() { refcount = TARGET; }
    };

    struct Edge
    {
        Edge(Node* to, Node* from) : to(to), from(from) {}

        Node* from;
        Node* to;
    };

    class DAG
    {
    public:
        void Add(Node* node) { m_Nodes.push_back(node); }
        void Add(Edge* edge) { m_Edges.push_back(edge); }

        std::vector<Edge*> GetIncomingEdges(Node* node);
        std::vector<Edge*> GetOutgoingEdges(Node* node);
        void               Cull();

    public:
        std::vector<Node*> m_Nodes;
        std::vector<Edge*> m_Edges;
    };

} // namespace Zephyr