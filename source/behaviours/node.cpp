#include "node.h"

namespace Behaviours
{
	SERIALISE_BEGIN(PinProperties)
		SERIALISE_PROPERTY("Colour", m_colour)
		SERIALISE_PROPERTY("Annotation", m_annotation)
		SERIALISE_PROPERTY("ConnectedNodeID", m_connectedNodeID)
	SERIALISE_END()

	SERIALISE_BEGIN(Node)
		if (op == Engine::SerialiseType::Read)
			m_outputPins.clear();
		SERIALISE_PROPERTY("LocalID", m_localID)
		SERIALISE_PROPERTY("Name", m_name)
		SERIALISE_PROPERTY("BgColour", m_bgColour)
		SERIALISE_PROPERTY("TextColour", m_textColour)
		SERIALISE_PROPERTY("EditorPosition", m_editorPosition)
		SERIALISE_PROPERTY("EditorDimensions", m_editorDimensions)
		SERIALISE_PROPERTY("OutputPins", m_outputPins)
	SERIALISE_END();

	
}