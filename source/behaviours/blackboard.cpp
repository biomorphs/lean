#include "blackboard.h"

namespace Behaviours
{
	void Blackboard::Reset()
	{
		m_ints.clear();
		m_floats.clear();
		m_entities.clear();
		m_vectors.clear();
	}
}