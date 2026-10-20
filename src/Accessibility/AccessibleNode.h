#pragma once

#include <string>

namespace OpenXcom
{

class InteractiveSurface;

enum AccessibleRole
{
	ACCESSIBLE_BUTTON,
	ACCESSIBLE_TOGGLE_BUTTON,
	ACCESSIBLE_LIST,
	ACCESSIBLE_EDIT,
	ACCESSIBLE_COMBO_BOX,
	ACCESSIBLE_SLIDER,
	ACCESSIBLE_SPIN_BUTTON,
	ACCESSIBLE_GRID,
	ACCESSIBLE_GRID_CELL,
	ACCESSIBLE_INVENTORY_SLOT,
	ACCESSIBLE_UNKNOWN
};

struct AccessibleNode
{
	AccessibleNode();

	InteractiveSurface *control;
	AccessibleRole role;
	std::string roleName;
	std::string name;
	std::string value;
	std::string state;
	std::string description;
	std::string shortcut;
	bool enabled;
	bool selected;
	bool focusable;
};

}
