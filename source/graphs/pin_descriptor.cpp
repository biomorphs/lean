#include "pin_descriptor.h"

namespace Graphs
{
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
}