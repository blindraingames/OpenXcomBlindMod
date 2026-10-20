#pragma once

#include "AccessibleNode.h"
#include <string>
#include <vector>

namespace OpenXcom
{

class Action;
class BaseView;
class InteractiveSurface;
class MiniBaseView;
class RuleBaseFacility;
class State;

/**
 * Non-visual navigation for the six by six base layout.
 */
class BasescapeAccessibility
{
private:
	State *_state;
	BaseView *_view;
	MiniBaseView *_mini;

	BaseView *findView(State *state) const;
	MiniBaseView *findMiniView(State *state) const;
	RuleBaseFacility *getPlacementRule(State *state) const;
	bool isPlacementState(State *state) const;
	bool isPlaceable(State *state) const;
	std::string describeCell(State *state) const;
	std::string describeBase(State *state) const;
	std::string describeSummary(State *state) const;
	void moveCursor(State *state, int dx, int dy, std::vector<std::string> &messages);
	void activateCell(State *state);
	void moveBase(State *state, int direction, std::vector<std::string> &messages);
	void activateBase(State *state);

public:
	BasescapeAccessibility();
	void reset();
	static bool supports(State *state);
	BaseView *getView(State *state) const;
	InteractiveSurface *getPreferredControl(State *state) const;
	std::vector<AccessibleNode> getNodes(State *state) const;
	std::string getSummary(State *state) const;
	void activate(State *state, std::vector<std::string> &messages);
	bool handle(Action *action, State *state, InteractiveSurface *focused, std::vector<std::string> &messages);
};

}
