#pragma once

#include "AccessibleNode.h"
#include <cstddef>
#include <string>
#include <vector>

namespace OpenXcom
{

class Action;
class BattleItem;
class InteractiveSurface;
class InventoryState;
class State;

/** Keyboard and speech adapter for the tactical equipment screen. */
class InventoryAccessibility
{
private:
	InventoryState *_state;
	BattleItem *_current;
	std::size_t _index;
	bool _initialized;

	std::vector<BattleItem*> items(InventoryState *state) const;
	void normalize(InventoryState *state);
	std::string describe(InventoryState *state) const;
	void move(InventoryState *state, int delta, std::vector<std::string> &messages);

public:
	InventoryAccessibility();
	void reset();
	static bool supports(State *state);
	InteractiveSurface *getPreferredControl(InventoryState *state) const;
	std::vector<AccessibleNode> getNodes(InventoryState *state);
	std::string getSummary(InventoryState *state);
	void activate(InventoryState *state, std::vector<std::string> &messages);
	bool handle(Action *action, InventoryState *state, InteractiveSurface *focused, std::vector<std::string> &messages);
};

}
