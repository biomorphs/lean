#pragma once
#include <string>
#include <vector>
#include <stdint.h>

namespace Graphs
{
	class PinDescriptor
	{
	public:
		std::string m_name;
		std::string m_type;
		uint8_t m_id = -1;
		bool m_isOutput = false;
	};

	const PinDescriptor* GetPinHelper(const std::vector<PinDescriptor>& pins, uint8_t id);
}