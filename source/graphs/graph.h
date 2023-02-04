#pragma once

#include "pin_descriptor.h"
#include <stdint.h>
#include <string_view>
#include <vector>
#include <robin_hood.h>

namespace Graphs
{
	class Connection
	{
	public:
		uint16_t m_nodeID0;	// node0 output -> node1 input
		uint16_t m_nodeID1;
		uint8_t m_pinID0;
		uint8_t m_pinID1;
	};

	class Node;
	class Graph
	{
	public:
		Graph();
		~Graph();

		bool AddInputPin(uint8_t id, std::string_view name, std::string_view dataType);
		bool AddOutputPin(uint8_t id, std::string_view name, std::string_view dataType);
		const PinDescriptor* GetInputPin(uint8_t id) const { return GetPinHelper(m_inputs, id); }
		const PinDescriptor* GetOutputPin(uint8_t id) const { return GetPinHelper(m_outputs, id); }

		uint16_t AddNode(Node* ptr);	// takes ownership of the ptr
		Node* GetNode(uint16_t nodeID);
		std::vector<Node*>& GetNodes() { return m_nodes; }

		enum AddConnectionResult
		{
			OK,
			BadNodeID,
			BadPinID,
			TypesIncompatible
		};
		// pass Node::InvalidID to reference graph inputs/outputs
		// only accepts output -> input
		AddConnectionResult AddConnection(uint16_t node0, uint8_t pinID0, uint16_t node1, uint8_t pinID1);
		const Connection* FindFirstConnection(uint16_t node0, uint8_t pinID0) const;
		const std::vector<Connection>& GetConnections() const { return m_connections; }

	private:
		std::vector<PinDescriptor> m_inputs;
		std::vector<PinDescriptor> m_outputs;
		uint16_t m_nextNodeID = 0;
		std::vector<Node*> m_nodes;
		robin_hood::unordered_map<uint16_t, int> m_nodeIdToIndex;
		std::vector<Connection> m_connections;
	};
}