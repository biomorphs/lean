#include "node.h"

namespace Graphs
{
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

	const PinDescriptor* Node::GetInputPin(uint8_t id) const
	{
		return GetPinHelper(m_inputs, id);
	}

	const PinDescriptor* Node::GetOutputPin(uint8_t id) const
	{
		return GetPinHelper(m_outputs, id);
	}
}