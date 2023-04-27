#pragma once
#include "node.h"
#include <memory>

namespace Behaviours
{
	class BehaviourTree
	{
	public:
		BehaviourTree();
		~BehaviourTree() = default;
		SERIALISED_CLASS();
		std::vector<std::unique_ptr<Node>> m_allNodes;
		uint16_t m_nextLocalID = 0;

		uint16_t AddNode(std::unique_ptr<Node>&& n);
		Node* FindNode(uint16_t localID) const;
		Node* FindConnectedNode(uint16_t localID) const;	// find a node with a output connection to this one
		Node* GetRoot() const;

		enum class AddConnectionResult : int
		{
			OK,
			BadNodeID,
			BadPinID,
			OutputPinAlreadyConnected,
			InputAlreadyConnected
		};
		AddConnectionResult AddConnection(uint16_t outNodeID, uint16_t outPinIndex, uint16_t inNodeID);
	};
}