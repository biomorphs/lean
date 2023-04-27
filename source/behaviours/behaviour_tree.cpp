#include "behaviour_tree.h"
#include "basic_nodes.h"

namespace Behaviours
{
	BehaviourTree::BehaviourTree()
	{
		AddNode(std::make_unique<RootNode>());
	}

	Node* BehaviourTree::GetRoot() const
	{
		return m_allNodes[0].get();
	}

	uint16_t BehaviourTree::AddNode(std::unique_ptr<Node>&& n)
	{
		uint16_t newID = m_nextLocalID++;
		n->m_localID = newID;
		m_allNodes.emplace_back(std::move(n));
		return newID;
	}

	Node* BehaviourTree::FindNode(uint16_t localID) const
	{
		auto found = std::find_if(m_allNodes.begin(), m_allNodes.end(), [localID](const std::unique_ptr<Node>& n) {
			return n->m_localID == localID;
		});
		if (found != m_allNodes.end())
		{
			return (*found).get();
		}
		else
		{
			return nullptr;
		}
	}

	Node* BehaviourTree::FindConnectedNode(uint16_t localID) const
	{
		// find a node with a output connection to this one
		Node* result = nullptr;
		auto found = std::find_if(m_allNodes.begin(), m_allNodes.end(), [localID](const std::unique_ptr<Node>& n) {
			auto foundConnection = std::find_if(n->m_outputPins.begin(), n->m_outputPins.end(), [localID](const PinProperties& p) {
				return p.m_connectedNodeID == localID;
			});
			return foundConnection != n->m_outputPins.end();
		});
		if (found != m_allNodes.end())
		{
			return found->get();
		}
		else
		{
			return nullptr;
		}
	}

	BehaviourTree::AddConnectionResult BehaviourTree::AddConnection(uint16_t outNodeID, uint16_t outPinIndex, uint16_t inNodeID)
	{
		Node* inNode = FindNode(inNodeID);
		Node* outNode = FindNode(outNodeID);
		if (inNode == nullptr || outNode == nullptr)
		{
			return AddConnectionResult::BadNodeID;
		}
		if (outNode->m_outputPins.size() <= outPinIndex)
		{
			return AddConnectionResult::BadPinID;
		}

		if (outNode->m_outputPins[outPinIndex].m_connectedNodeID != -1)
		{
			return AddConnectionResult::OutputPinAlreadyConnected;
		}

		if (FindConnectedNode(inNodeID) != nullptr)
		{
			return AddConnectionResult::InputAlreadyConnected;
		}

		outNode->m_outputPins[outPinIndex].m_connectedNodeID = inNodeID;

		return AddConnectionResult::OK;
	}
}