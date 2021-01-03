#pragma once

#include "engine/serialisation.h"

class Scene
{
public:
	Scene() = default;
	~Scene() = default;
	std::string Name() const { return m_name; }
	std::string& Name() { return m_name; }
	std::vector<std::string>& Scripts() { return m_scripts; }
	const std::vector<std::string>& Scripts() const { return m_scripts; }
	SERIALISED_CLASS();
private:
	std::string m_name;
	std::vector<std::string> m_scripts;
};