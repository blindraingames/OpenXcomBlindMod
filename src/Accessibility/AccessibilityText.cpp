#include "AccessibilityText.h"
#include "AccessibleNode.h"
#include <cctype>

namespace OpenXcom
{

std::string AccessibilityText::clean(const std::string &text)
{
	std::string result;
	bool space = false;
	for (std::string::const_iterator i = text.begin(); i != text.end(); ++i)
	{
		const unsigned char c = static_cast<unsigned char>(*i);
		if (c < 32 || c == 127)
		{
			space = !result.empty();
			continue;
		}
		if (std::isspace(c))
		{
			space = !result.empty();
			continue;
		}
		if (space)
		{
			result += ' ';
			space = false;
		}
		result += static_cast<char>(c);
	}
	return result;
}

std::string AccessibilityText::describe(const AccessibleNode &node)
{
	std::string result = clean(node.name);
	if (!node.value.empty())
	{
		if (!result.empty()) result += ", ";
		result += clean(node.value);
	}
	if (!node.state.empty())
	{
		if (!result.empty()) result += ", ";
		result += clean(node.state);
	}

	std::string role = clean(node.roleName);
	if (role.empty()) switch (node.role)
	{
	case ACCESSIBLE_BUTTON: role = "button"; break;
	case ACCESSIBLE_TOGGLE_BUTTON: role = "toggle button"; break;
	case ACCESSIBLE_LIST: role = "list"; break;
	case ACCESSIBLE_EDIT: role = "edit"; break;
	case ACCESSIBLE_COMBO_BOX: role = "combo box"; break;
	case ACCESSIBLE_SLIDER: role = "slider"; break;
	case ACCESSIBLE_SPIN_BUTTON: role = "spin button"; break;
	case ACCESSIBLE_GRID: role = "grid"; break;
	case ACCESSIBLE_GRID_CELL: role = "grid cell"; break;
	case ACCESSIBLE_INVENTORY_SLOT: role = "inventory slot"; break;
	default: break;
	}
	if (!role.empty())
	{
		if (!result.empty()) result += ", ";
		result += role;
	}
	if (!node.shortcut.empty())
	{
		result += ", ";
		result += clean(node.shortcut);
	}
	return result;
}

}
