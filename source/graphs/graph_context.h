#pragma once
#include "node.h"
#include <stdint.h>
#include <robin_hood.h>

namespace Graphs
{
	class Graph;
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
		robin_hood::unordered_flat_map<uint32_t, uint32_t> m_pinToStorageLookup;
		// std::unordered_map<uint32_t, uint32_t> m_pinToStorageLookup;	// useful for debugging
	};
}