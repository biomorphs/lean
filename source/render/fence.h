#pragma once

namespace Render
{
	class Fence
	{
		friend class Device;
	public:
		Fence() = default;
		bool IsValid() { return m_data != nullptr; }
	private:
		Fence(void* ptr) : m_data(ptr) {}
		void* m_data = nullptr;
	};
}