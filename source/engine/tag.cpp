#include "tag.h"

namespace Engine
{
	SERIALISE_BEGIN(Tag)
		if (op == Engine::SerialiseType::Write)
		{
			Engine::ToJson("String", m_string, json);
		}
		else
		{
			std::string strValue;
			Engine::FromJson("String", strValue, json);
			*this = strValue.c_str();
		}
	SERIALISE_END()
}