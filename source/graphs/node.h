#pragma once

#include "pin_descriptor.h"
#include <vector>

namespace Graphs
{
	class GraphContext;
	class Node
	{
	public:
		static constexpr uint16_t InvalidID = -1;
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
		uint16_t GetID() const { return m_id; }
		bool IsConstant() const { return m_isConstant; }
		std::string_view GetName() const { return m_name; }
		const std::vector<PinDescriptor>& GetPins() const { return m_pins; }

		void SetID(uint16_t id) { m_id = id; }	// this should ONLY be called by a graph
	protected:
		std::string m_name;
		bool m_isConstant = false;
	private:
		bool AddPin(uint8_t id, std::string_view name, std::string_view dataType, bool isOutput);
		uint16_t m_id = InvalidID;
		std::vector<PinDescriptor> m_pins;
	};
}