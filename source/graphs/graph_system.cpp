#include "graph_system.h"
#include "core/log.h"
#include "core/profiler.h"
#include <robin_hood.h>
#include <string>
#include <string_view>
#include <vector>
#include <memory>

namespace Graphs
{

	class PinDescriptor
	{
	public:
		std::string m_name;
		std::string m_type;
		uint8_t m_id = -1;
	};

	class GraphContext;
	class Node
	{
	public:
		static constexpr uint16_t InvalidID = -1;
		uint16_t m_id = InvalidID;
		std::string m_name;
		bool m_isConstant = false;
		std::vector<PinDescriptor> m_inputs;
		std::vector<PinDescriptor> m_outputs;
		enum ExecResult
		{
			Completed,
			Continue,		// node will be executed again
			Error
		};
		virtual ExecResult Execute(GraphContext&) = 0;
		bool AddInputPin(uint8_t id, std::string_view name, std::string_view dataType);
		bool AddOutputPin(uint8_t id, std::string_view name, std::string_view dataType);
		const PinDescriptor* GetInputPin(uint8_t id) const;
		const PinDescriptor* GetOutputPin(uint8_t id) const;
	};

	bool Node::AddInputPin(uint8_t id, std::string_view name, std::string_view dataType)
	{
		if (GetInputPin(id) != nullptr)
		{
			return false;
		}
		PinDescriptor pd;
		pd.m_id = id;
		pd.m_name = name;
		pd.m_type = dataType;
		m_inputs.push_back(pd);
		return true;
	}

	bool Node::AddOutputPin(uint8_t id, std::string_view name, std::string_view dataType)
	{
		if (GetOutputPin(id) != nullptr)
		{
			return false;
		}
		PinDescriptor pd;
		pd.m_id = id;
		pd.m_name = name;
		pd.m_type = dataType;
		m_outputs.push_back(pd);
		return true;
	}

	const PinDescriptor* GetPinHelper(const std::vector<PinDescriptor>& pins, uint8_t id)
	{
		const PinDescriptor* pd = nullptr;
		auto foundExisting = std::find_if(pins.begin(), pins.end(), [id](const PinDescriptor& pd) {
			return pd.m_id == id;
		});
		if (foundExisting != pins.end())
		{
			pd = &(*foundExisting);
		}
		return pd;
	}

	const PinDescriptor* Node::GetInputPin(uint8_t id) const
	{
		return GetPinHelper(m_inputs, id);
	}

	const PinDescriptor* Node::GetOutputPin(uint8_t id) const
	{
		return GetPinHelper(m_outputs, id);
	}

	class Connection
	{
	public:
		uint16_t m_nodeID0;	// node0 output -> node1 input
		uint16_t m_nodeID1;
		uint8_t m_pinID0;
		uint8_t m_pinID1;
	};

	class Graph
	{
	public:
		Graph() {}
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
			TypesIncompatible,
			Duplicate	// connection already exists
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
		std::vector<Connection> m_connections;
	};

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

		ptr->m_id = newNodeID;
		m_nodes.push_back(ptr);

		return newNodeID;
	}

	Node* Graph::GetNode(uint16_t nodeID)
	{
		auto foundIt = std::find_if(m_nodes.begin(), m_nodes.end(), [nodeID](const Node* n0) {
			return n0->m_id == nodeID;
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
			return nodeID0 == c.m_nodeID0 &&  pinID0 == c.m_pinID0;
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

	class GraphContext
	{
	public:
		GraphContext(Graph* graphToRun);
		void WriteGraphPin(uint16_t nodeId, uint8_t myOutputPinId, int value);
		int ReadGraphPin(uint16_t nodeId, uint8_t myInputPinId, int defaultValue = 0) const;
		void ExecuteNext(uint16_t nodeId, uint8_t execPinId);			// set next execution pin
		enum RunResult
		{
			Completed,
			StillRunning,
			ErrorInvalidExecPin,
			ErrorWithNode
		};
		RunResult RunSingleStep();

		// only call these from node execute, they reference the current running node
		void WritePin(uint8_t myOutputPinId, int value);
		int ReadPin(uint8_t myInputPinId, int defaultValue = 0) const;
		
	private:
		struct ExecPin {
			uint16_t m_nodeId = Node::InvalidID;
			uint8_t m_pinId = -1;
		};
		Node::ExecResult RunConstantNodes();
		uint32_t MakePinDataLookup(uint16_t nodeId, uint8_t pinId) const;
		
		ExecPin m_nextPinToRun;
		bool m_hasRunConstants = false;
		Graph* m_runningGraph = nullptr;
		Node* m_runningNode = nullptr;

		void PreparePinStorage();
		std::vector<int> m_allIntPinValues;
		//robin_hood::unordered_flat_map<uint32_t, uint32_t> m_pinToStorageLookup;
		std::unordered_map<uint32_t, uint32_t> m_pinToStorageLookup;
	};

	GraphContext::GraphContext(Graph* graphToRun)
		: m_runningGraph(graphToRun)
	{
		PreparePinStorage();
	}

	uint32_t GraphContext::MakePinDataLookup(uint16_t nodeId, uint8_t pinId) const
	{
		return static_cast<uint32_t>(pinId) | static_cast<uint32_t>(nodeId) << 8;
	}

	void GraphContext::PreparePinStorage()
	{
		SDE_PROF_EVENT();

		m_allIntPinValues.clear();
		m_allIntPinValues.reserve(4096);
		m_pinToStorageLookup.clear();

		const auto& connections = m_runningGraph->GetConnections();
		for (const auto& c : connections)
		{
			bool isExecutePin = false;
			const PinDescriptor* inputPinDesc = nullptr;
			if (c.m_nodeID0 == Node::InvalidID)	// graph pin input
			{
				inputPinDesc = m_runningGraph->GetInputPin(c.m_pinID0);
			}
			else    // node pin output
			{
				Node* n = m_runningGraph->GetNode(c.m_nodeID0);
				inputPinDesc = n->GetOutputPin(c.m_pinID0);
			}
			isExecutePin = inputPinDesc ? inputPinDesc->m_type == "Execution" : false;
			if (!isExecutePin)
			{
				const uint32_t outputKey = MakePinDataLookup(c.m_nodeID0, c.m_pinID0);
				const uint32_t inputKey = MakePinDataLookup(c.m_nodeID1, c.m_pinID1);
				uint32_t storageIndex = -1;
				auto foundStorage = m_pinToStorageLookup.find(outputKey);
				if (foundStorage == m_pinToStorageLookup.end())		// allocate storage once for each connected output pin
				{
					storageIndex = static_cast<uint32_t>(m_allIntPinValues.size());
					m_allIntPinValues.push_back(-1);
					m_pinToStorageLookup[outputKey] = storageIndex;
				}
				else
				{
					storageIndex = foundStorage->second;
				}
				auto foundInput = m_pinToStorageLookup.find(inputKey);
				if (foundInput != m_pinToStorageLookup.end())
				{
					SDE_LOG("Error - input pin should only have 1 incoming connection! Node '%d', Pin %d", c.m_nodeID1, c.m_pinID1);
				}
				m_pinToStorageLookup[inputKey] = storageIndex;
			}
		}
	}

	void GraphContext::WriteGraphPin(uint16_t nodeId, uint8_t myOutputPinId, int value)
	{
		uint32_t dataLookup = MakePinDataLookup(nodeId, myOutputPinId);
		auto foundIt = m_pinToStorageLookup.find(dataLookup);
		if (foundIt == m_pinToStorageLookup.end())
		{
			SDE_LOG("Woah, no storage for pin %d", myOutputPinId);
		}
		else if (foundIt->second < m_allIntPinValues.size())
		{
			m_allIntPinValues[foundIt->second] = value;
		}
		else
		{
			SDE_LOG("Bad lookup!");
		}
	}

	void GraphContext::WritePin(uint8_t myOutputPinId, int value)
	{
		WriteGraphPin(m_runningNode->m_id, myOutputPinId, value);
	}

	int GraphContext::ReadGraphPin(uint16_t nodeId, uint8_t myInputPinId, int defaultValue) const
	{
		int valueToReturn = 0;
		uint32_t dataLookup = MakePinDataLookup(nodeId, myInputPinId);
		auto foundIt = m_pinToStorageLookup.find(dataLookup);
		if (foundIt == m_pinToStorageLookup.end())
		{
			SDE_LOG("Woah, no storage for pin %d", myInputPinId);
			valueToReturn = defaultValue;
		}
		else if (foundIt->second < m_allIntPinValues.size())
		{
			valueToReturn = m_allIntPinValues[foundIt->second];
		}
		else
		{
			SDE_LOG("Bad lookup!");
		}
		return valueToReturn;
	}

	int GraphContext::ReadPin(uint8_t myInputPinId, int defaultValue) const
	{
		return ReadGraphPin(m_runningNode->m_id, myInputPinId, defaultValue);
	}

	void GraphContext::ExecuteNext(uint16_t nodeId, uint8_t execPinId)
	{
		m_nextPinToRun = { nodeId, execPinId };
	}

	Node::ExecResult GraphContext::RunConstantNodes()
	{
		SDE_PROF_EVENT();

		std::vector<Node*>& nodes = m_runningGraph->GetNodes();
		for (Node* n : nodes)
		{
			if (n->m_isConstant)
			{
				m_runningNode = n;	// for Read/WritePin
				Node::ExecResult r = n->Execute(*this);
				m_runningNode = nullptr;
				if (r == Node::ExecResult::Error)
				{
					SDE_LOG("Error occured in constant node '%s'", n->m_name.c_str());
					return Node::ExecResult::Error;
				}
				else if (r == Node::ExecResult::Continue)
				{
					SDE_LOG("Error - constant node '%s' trying to run multiple times!", n->m_name.c_str());
					return Node::ExecResult::Error;
				}
			}
		}
		m_hasRunConstants = true;
		return Node::ExecResult::Completed;
	}

	GraphContext::RunResult GraphContext::RunSingleStep()
	{
		SDE_PROF_EVENT();
		if (m_hasRunConstants == false)
		{
			RunConstantNodes();
		}
		if (m_runningNode == nullptr && m_nextPinToRun.m_pinId != -1)
		{
			SDE_PROF_EVENT("FindNextRunningNode");
			const Connection* execConnection = m_runningGraph->FindFirstConnection(m_nextPinToRun.m_nodeId, m_nextPinToRun.m_pinId);
			m_nextPinToRun = { Node::InvalidID,(uint8_t)-1 };	// regardless of the result, we are done with this pin
			if (execConnection == nullptr)
			{
				SDE_LOG("Exec Pin or connection not found (node %llu, pin %d)", m_nextPinToRun.m_nodeId, m_nextPinToRun.m_pinId);
				return RunResult::ErrorInvalidExecPin;
			}
			if (execConnection->m_nodeID1 == Node::InvalidID && execConnection->m_pinID1 != -1)	// next connection is graph exec output, we are done
			{
				m_nextPinToRun = { execConnection->m_nodeID1, execConnection->m_pinID1 };	// store the next pin to run
				return RunResult::Completed;
			}
			Graphs::Node* nodeToRun = m_runningGraph->GetNode(execConnection->m_nodeID1);
			if (nodeToRun->GetInputPin(execConnection->m_pinID1)->m_type != "Execution")
			{
				SDE_LOG("Current exec pin is not an execution pin! (node %llu, pin %d)", execConnection->m_nodeID0, execConnection->m_pinID0);
				return RunResult::ErrorInvalidExecPin;
			}
			m_runningNode = nodeToRun;
		}
		if (m_runningNode != nullptr)
		{
			SDE_PROF_EVENT("RunNode");
			Node::ExecResult result = m_runningNode->Execute(*this);
			if (result == Node::ExecResult::Error)
			{
				SDE_LOG("Error with node '%s'", m_runningNode->m_name.c_str());
				return RunResult::ErrorWithNode;
			}
			else if (result == Node::ExecResult::Completed)
			{
				m_runningNode = nullptr;
			}
			return RunResult::StillRunning;
		}
		return RunResult::Completed;
	}
	
}

class ConstantNode : public Graphs::Node
{
public:
	ConstantNode(int value = 0)
		: m_value(value)
	{
		m_name = "Constant<int>";
		m_isConstant = true;
		AddOutputPin(0, "Value", "int");
	}
	ExecResult Execute(Graphs::GraphContext& ctx) override
	{
		SDE_PROF_EVENT();
		ctx.WritePin(0, m_value);
		return ExecResult::Completed;
	}
	int m_value;
};

class AddIntsNode : public Graphs::Node
{
public:
	AddIntsNode()
	{
		m_name = "AddNumbers<int>";
		AddInputPin(0, "Execute", "Execution");
		AddInputPin(1, "Value1", "int");
		AddInputPin(2, "Value2", "int");
		AddOutputPin(3, "RunNext", "Execution");
		AddOutputPin(4, "Result", "int");
	}
	ExecResult Execute(Graphs::GraphContext& ctx) override
	{
		SDE_PROF_EVENT();
		int v1 = ctx.ReadPin(1);
		int v2 = ctx.ReadPin(2);
		ctx.WritePin(4, v1 + v2);
		ctx.ExecuteNext(m_id, 3);	// run the next execution pin 
		return ExecResult::Completed;
	}
};

class IntGreaterThan : public Graphs::Node
{
public:
	IntGreaterThan()
	{
		m_name = "GreaterThan<int>";
		AddInputPin(0, "Execute", "Execution");
		AddInputPin(1, "Value1", "int");
		AddInputPin(2, "Value2", "int");
		AddOutputPin(3, "RunIfTrue", "Execution");
		AddOutputPin(4, "RunIfFalse", "Execution");
	}

	ExecResult Execute(Graphs::GraphContext& ctx) override
	{
		SDE_PROF_EVENT();
		int v1 = ctx.ReadPin(1);
		int v2 = ctx.ReadPin(2);
		if (v1 > v2)
		{
			ctx.ExecuteNext(m_id, 3);
		}
		else
		{
			ctx.ExecuteNext(m_id, 4);
		}
		return ExecResult::Completed;
	}
};

class PrintIntNode : public Graphs::Node
{
public:
	PrintIntNode(std::string_view prefix = "Value = ")
		: m_prefix(prefix)
	{
		m_name = "PrintValue<int>";
		AddInputPin(0, "Execute", "Execution");
		AddInputPin(1, "Value", "int");
		AddOutputPin(2, "RunNext", "Execution");
	}
	ExecResult Execute(Graphs::GraphContext& ctx) override
	{
		SDE_PROF_EVENT();
		int value = ctx.ReadPin(1);
		SDE_LOG("%s%d", m_prefix.c_str(), value);
		ctx.ExecuteNext(m_id, 2);	// run the next execution pin 
		return ExecResult::Completed;
	}
	std::string m_prefix;
};

Graphs::Graph g_testGraph;

bool GraphSystem::Initialise()
{
	SDE_PROF_EVENT();

	g_testGraph.AddInputPin(0, "Execute", "Execution");
	g_testGraph.AddInputPin(1, "TestValue", "int");
	g_testGraph.AddOutputPin(2, "RunNext", "Execution");
	g_testGraph.AddOutputPin(3, "Result", "int");

	uint16_t c0id = g_testGraph.AddNode(new ConstantNode(3));
	uint16_t printC0Id = g_testGraph.AddNode(new PrintIntNode(""));
	uint16_t c1id = g_testGraph.AddNode(new ConstantNode(5));
	uint16_t printC1Id = g_testGraph.AddNode(new PrintIntNode(" + "));
	uint16_t addIntId = g_testGraph.AddNode(new AddIntsNode);
	uint16_t printResultId = g_testGraph.AddNode(new PrintIntNode(" = "));
	uint16_t igtId = g_testGraph.AddNode(new IntGreaterThan());
	uint16_t printGtId = g_testGraph.AddNode(new PrintIntNode("which is greater than "));
	uint16_t printNotGtId = g_testGraph.AddNode(new PrintIntNode("which is not greater than "));

	// exec flow
	g_testGraph.AddConnection(-1, 0, printC0Id, 0);				// graph execute -> c0 print
	g_testGraph.AddConnection(printC0Id, 2, printC1Id, 0);		// c0 print -> c1 print
	g_testGraph.AddConnection(printC1Id, 2, addIntId, 0);		// c1 print -> addInt execute
	g_testGraph.AddConnection(addIntId, 3, printResultId, 0);	// addInt exec -> print reult
	g_testGraph.AddConnection(printResultId, 2, igtId, 0);		// print result -> isIntGreater
	g_testGraph.AddConnection(igtId, 3, printGtId, 0);			// int greater true -> print greater
	g_testGraph.AddConnection(igtId, 4, printNotGtId, 0);		// int greater false -> print not greater
	g_testGraph.AddConnection(printGtId, 2, -1, 2);				// print greater -> graph out
	g_testGraph.AddConnection(printNotGtId, 2, -1, 2);			// print not greater -> graph out

	// data flow
	g_testGraph.AddConnection(c0id, 0, addIntId, 1);			// c0 value -> addInt v1
	g_testGraph.AddConnection(c1id, 0, addIntId, 2);			// c1 value -> addInt v2
	g_testGraph.AddConnection(c0id, 0, printC0Id, 1);			// c0 value -> printC0
	g_testGraph.AddConnection(c1id, 0, printC1Id, 1);			// c1 value -> printC1
	g_testGraph.AddConnection(addIntId, 4, printResultId, 1);	// addInt result -> print value
	g_testGraph.AddConnection(addIntId, 4, -1, 3);				// addInt result -> graph result
	g_testGraph.AddConnection(addIntId, 4, igtId, 1);			// addInt result -> igt v1
	g_testGraph.AddConnection(-1, 1, igtId, 2);					// graph input test value -> igt v2
	g_testGraph.AddConnection(-1, 1, printGtId, 1);				// graph input test value -> print greater
	g_testGraph.AddConnection(-1, 1, printNotGtId, 1);			// graph input test value -> print not greater
	
	return true;
}

bool GraphSystem::Tick(float timeDelta)
{
	SDE_PROF_EVENT();
	{
		Graphs::GraphContext runContext(&g_testGraph);
		runContext.WriteGraphPin(Graphs::Node::InvalidID, 1, 7);		// test value
		runContext.ExecuteNext(Graphs::Node::InvalidID, 0);				// execute graph pin 0
		bool graphRunning = true;
		while (graphRunning)
		{
			auto stepResult = runContext.RunSingleStep();
			if (stepResult == Graphs::GraphContext::Completed)
			{
				SDE_LOG("Graph finished ok");
				int resultValue = runContext.ReadGraphPin(Graphs::Node::InvalidID, 3);
				SDE_LOG("\tresult = %d", resultValue);
				graphRunning = false;
			}
		}
	}

	return true;
}

void GraphSystem::Shutdown()
{
	SDE_PROF_EVENT();
}