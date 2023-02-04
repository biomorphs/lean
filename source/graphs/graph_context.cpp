#include "graph_context.h"
#include "graph.h"
#include "pin_descriptor.h"
#include "core/profiler.h"
#include "core/log.h"

namespace Graphs
{
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
		WriteGraphPin(m_runningNode->GetID(), myOutputPinId, value);
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
		return ReadGraphPin(m_runningNode->GetID(), myInputPinId, defaultValue);
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
			if (n->IsConstant())
			{
				m_runningNode = n;	// for Read/WritePin
				Node::ExecResult r = n->Execute(*this);
				m_runningNode = nullptr;
				if (r == Node::ExecResult::Error)
				{
					SDE_LOG("Error occured in constant node '%s'", n->GetName().data());
					return Node::ExecResult::Error;
				}
				else if (r == Node::ExecResult::Continue)
				{
					SDE_LOG("Error - constant node '%s' trying to run multiple times!", n->GetName().data());
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
				SDE_LOG("Error with node '%s'", m_runningNode->GetName().data());
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