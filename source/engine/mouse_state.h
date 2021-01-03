#pragma once

namespace Engine
{
	enum MouseButtons
	{
		LeftButton = (1 << 0),
		MiddleButton = (1 << 1),
		RightButton = (1 << 2)
	};

	struct MouseRawState
	{
		int32_t m_cursorX = 0;	// position relative to window/viewport in pixels
		int32_t m_cursorY = 0;
		uint32_t m_buttonState = 0;	// mask of buttons
		int32_t m_wheelScroll = 0;
	};
}