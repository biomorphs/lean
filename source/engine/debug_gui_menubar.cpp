#include "debug_gui_menubar.h"

namespace Engine
{
	void SubMenu::AddItem(std::string name, std::function<void()> onSelected, std::string shortcut)
	{
		MenuItem mi;
		mi.m_label = name;
		mi.m_shortcut = shortcut;
		mi.m_onSelected = onSelected;
		m_menuItems.push_back(mi);
	}

	SubMenu& MenuBar::AddSubmenu(std::string label)
	{
		SubMenu sm;
		sm.m_label = label;
		m_subMenus.push_back(sm);
		return m_subMenus[m_subMenus.size() - 1];
	}

	SubMenu& SubMenu::AddSubmenu(std::string label)
	{
		SubMenu sm;
		sm.m_label = label;
		m_subMenus.push_back(sm);
		return m_subMenus[m_subMenus.size() - 1];
	}
}