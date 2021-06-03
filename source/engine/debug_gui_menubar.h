#pragma once

#include <string>
#include <functional>
#include <vector>

// menu bar helper
namespace Engine
{
	class MenuItem
	{
	public:
		MenuItem() = default;
		~MenuItem() = default;

		std::string m_label;
		std::string m_shortcut;
		std::function<void()> m_onSelected;
		bool m_selected = false;
		bool m_enabled = true;
	};

	class SubMenu
	{
	public:
		SubMenu() = default;
		~SubMenu() = default;
		void AddItem(std::string name, std::function<void()> onSelected, std::string shortcut="");
		SubMenu& AddSubmenu(std::string label);
		SubMenu* GetSubmenu(std::string label);
		void RemoveItems();

		std::string m_label;
		bool m_enabled = true;
		std::vector<MenuItem> m_menuItems;
		std::vector<SubMenu> m_subMenus;
	};

	class MenuBar
	{
	public:
		MenuBar() = default;
		~MenuBar() = default;
		SubMenu& AddSubmenu(std::string label);
		SubMenu* GetSubmenu(std::string label);
		void RemoveItems();

		std::vector<SubMenu> m_subMenus;
	};
}