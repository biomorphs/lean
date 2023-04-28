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

	float Blackboard::GetOrParseFloat(const Engine::Tag& tag)
	{
		float r = 0.0f;
		if (IsKey(tag.c_str()))
		{
			r = TryGetFloat(tag, 0.0f);
		}
		else
		{
			sscanf(tag.c_str(), "%f", &r);
		}
		return r;
	}
}