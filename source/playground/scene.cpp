#include "scene.h"

SERIALISE_BEGIN(Scene)
SERIALISE_PROPERTY("Name", m_name)
SERIALISE_PROPERTY("Scripts", m_scripts)
SERIALISE_END()