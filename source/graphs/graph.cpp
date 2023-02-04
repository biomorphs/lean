#include "graph.h"
#include "core/log.h"
#include "node.h"

namespace Graphs
{
	Graph::~Graph()
	{
		for (auto it : m_nodes)
		{
			delete it;
		}
	}

	bool Graph::AddInputPin(uint8_t id, std::string_view name, std::string_view dataType)
	{
		if (GetInputPin(id) != nullptr)
		{
			return false;
		}
		PinDescriptor newPin;
		newPin.m_id = id;
		newPin.m_name = name;
		newPin.m_type = dataType;
		m_inputs.push_back(newPin);
		return true;
	}

	bool Graph::AddOutputPin(uint8_t id, std::string_view name, std::string_view dataType)
	{
		if (GetOutputPin(id) != nullptr)
		{
			return false;
		}
		PinDescriptor newPin;
		newPin.m_id = id;
		newPin.m_name = name;
		newPin.m_type = dataType;
		m_outputs.push_back(newPin);
		return true;
	}

	uint16_t Graph::AddNode(Node* ptr)
	{
		uint16_t newNodeID = m_nextNodeID++;

		bool foundExisting = GetNode(newNodeID) != nullptr;
		if (foundExisting)
		{
			SDE_LOG("Duplicate node ID %llu!", newNodeID);
			return Node::InvalidID;
		}

		ptr->SetID(newNodeID);
		m_nodes.push_back(ptr);

		return newNodeID;
	}

	Node* Graph::GetNode(uint16_t nodeID)
	{
		auto foundIt = std::find_if(m_nodes.begin(), m_nodes.end(), [nodeID](const Node* n0) {
			return n0->GetID() == nodeID;
		});
		Node* foundNode = nullptr;
		if (foundIt != m_nodes.end())
		{
			foundNode = *foundIt;
		}
		return foundNode;
	}

	const Connection* Graph::FindFirstConnection(uint16_t nodeID0, uint8_t pinID0) const
	{
		const Connection* foundConnection = nullptr;
		auto foundExisting = std::find_if(m_connections.begin(), m_connections.end(), [&](const Connection& c) {
			return nodeID0 == c.m_nodeID0 && pinID0 == c.m_pinID0;
		});
		if (foundExisting != m_connections.end())
		{
			foundConnection = &(*foundExisting);
		}
		return foundConnection;
	}

	Graph::AddConnectionResult Graph::AddConnection(uint16_t node0, uint8_t pinID0, uint16_t node1, uint8_t pinID1)
	{
		const Node* n0 = node0 != Node::InvalidID ? GetNode(node0) : nullptr;
		const Node* n1 = node1 != Node::InvalidID ? GetNode(node1) : nullptr;
		if (node0 != Node::InvalidID && !n0)
		{
			return AddConnectionResult::BadNodeID;
		}
		if (node1 != Node::InvalidID && !n1)
		{
			return AddConnectionResult::BadNodeID;
		}

		const PinDescriptor* p0 = node0 != Node::InvalidID ? n0->GetOutputPin(pinID0) : GetInputPin(pinID0);
		const PinDescriptor* p1 = node1 != Node::InvalidID ? n1->GetInputPin(pinID1) : GetOutputPin(pinID1);
		if (p0 == nullptr || p1 == nullptr)
		{
			return AddConnectionResult::BadPinID;
		}

		if (p0->m_type != p1->m_type)
		{
			return AddConnectionResult::TypesIncompatible;
		}

		Connection nc;
		nc.m_nodeID0 = node0;
		nc.m_pinID0 = pinID0;
		nc.m_nodeID1 = node1;
		nc.m_pinID1 = pinID1;
		auto foundExisting = std::find_if(m_connections.begin(), m_connections.end(), [&nc](Connection& c) {
			return c.m_nodeID0 == nc.m_nodeID0 && c.m_nodeID1 == nc.m_nodeID1 && c.m_pinID0 == nc.m_pinID0 && c.m_pinID1 == nc.m_pinID1;
		});
		if (foundExisting != m_connections.end())
		{
			return AddConnectionResult::Duplicate;
		}
		m_connections.push_back(nc);
		return AddConnectionResult::OK;
	}
}