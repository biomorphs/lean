#include "debug_gui_menubar.h"

namespace Engine
{
	void SubMenu::RemoveItems()
	{
		m_menuItems.clear();
		m_subMenus.clear();
	}

	void MenuBar::RemoveItems()
	{
		m_subMenus.clear();
	}

	void SubMenu::AddItem(std::string name, std::function<void()> onSelected, std::string shortcut)
	{
		MenuItem mi;
		mi.m_label = name;
		mi.m_shortcut = shortcut;
		mi.m_onSelected = onSelected;
		m_menuItems.push_back(mi);
	}

	SubMenu* MenuBar::GetSubmenu(std::string label)
	{
		auto found = std::find_if(m_subMenus.begin(), m_subMenus.end(), [&label](const SubMenu& m) {
			return m.m_label == label;
		});
		if (found != m_subMenus.end())
		{
			return &(*found);
		}
		else
		{
			return nullptr;
		}
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

	SubMenu* SubMenu::GetSubmenu(std::string label)
	{
		auto found = std::find_if(m_subMenus.begin(), m_subMenus.end(), [&label](const SubMenu& m) {
			return m.m_label == label;
		});
		if (found != m_subMenus.end())
		{
			return &(*found);
		}
		else
		{
			return nullptr;
		}
	}
}