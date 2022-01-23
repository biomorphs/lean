#include "entity_handle.h"

EntityHandle::OnLoaded EntityHandle::m_onLoadedFn;

SERIALISE_BEGIN(EntityHandle)
	SERIALISE_PROPERTY("ID", m_id)
	if (op == Engine::SerialiseType::Read && m_onLoadedFn != nullptr)
	{
		m_onLoadedFn(*this);
	}
SERIALISE_END()