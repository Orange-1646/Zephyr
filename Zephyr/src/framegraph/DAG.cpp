#include "DAG.h"

namespace Zephyr
{
    std::vector<Edge*> DAG::GetIncomingEdges(Node* node)
    {
        std::vector<Edge*> result;

        for (auto& edge : m_Edges)
        {
            if (edge->to == node)
            {
                result.push_back(edge);
            }
        }

        return result;
    }

    std::vector<Edge*> DAG::GetOutgoingEdges(Node* node) { 
        std::vector<Edge*> result;

        for (auto& edge : m_Edges)
        {
            if (edge->from == node)
            {
                result.push_back(edge);
            }
        }

        return result;
    }

    void DAG::Cull()
    {
        // first iterate through all the edges and add 1 refcount to all the "from" node
        for (auto& edge : m_Edges)
        {
            auto& from = edge->from;
            from->refcount++;
        }

        // second iterate through the nodes and push all nodes with 0 refcount to the cull stack
        std::stack<Node*> cullStack;
        for (auto& node : m_Nodes)
        {
            if (node->refcount == 0)
            {
                cullStack.push(node);
            }
        }

        // then check for each node in the stack, remove a refcount from its parents(check all the edges with "to"
        // equals current node, the "from" of that edge is considered its parent)
        while (cullStack.size() != 0)
        {
            auto node = cullStack.top();
            cullStack.pop();

            auto incomingEdges = GetIncomingEdges(node);

            for (auto& edge : incomingEdges)
            {
                auto& from = edge->from;
                from->refcount--;

                if (from->refcount == 0)
                {
                    cullStack.push(from);
                }
            }
        }
    }
} // namespace Zephyr