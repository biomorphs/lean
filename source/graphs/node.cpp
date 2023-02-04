#include "node.h"

namespace Graphs
{
	bool Node::AddPin(uint8_t id, std::string_view name, std::string_view dataType, bool isOutput)
	{
		if (GetPinHelper(m_pins, id) != nullptr)
		{
			return false;
		}
		PinDescriptor pd;
		pd.m_id = id;
		pd.m_name = name;
		pd.m_type = dataType;
		pd.m_isOutput = isOutput;
		m_pins.push_back(pd);
		return true;
	}

	bool Node::AddInputPin(uint8_t id, std::string_view name, std::string_view dataType)
	{
		return AddPin(id, name, dataType, false);
	}

	bool Node::AddOutputPin(uint8_t id, std::string_view name, std::string_view dataType)
	{
		return AddPin(id, name, dataType, true);
	}

	const PinDescriptor* Node::GetInputPin(uint8_t id) const
	{
		return GetPinHelper(m_pins, id);
	}

	const PinDescriptor* Node::GetOutputPin(uint8_t id) const
	{
		return GetPinHelper(m_pins, id);
	}
}