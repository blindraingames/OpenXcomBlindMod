#pragma once

#include "AccessibleNode.h"
#include <cstddef>
#include <string>
#include <vector>

namespace OpenXcom
{

class Action;
class InteractiveSurface;
class State;
class UnitInfoState;

/** Keyboard and speech adapter for soldier statistics and mind-probe results. */
class UnitInfoAccessibility
{
private:
	UnitInfoState *_state;
	std::size_t _index;
	bool _initialized;

	void normalize(UnitInfoState *state);
	std::string describe(UnitInfoState *state) const;
	void move(UnitInfoState *state, int delta, std::vector<std::string> &messages);

public:
	UnitInfoAccessibility();
	void reset();
	static bool supports(State *state);
	InteractiveSurface *getPreferredControl(UnitInfoState *state) const;
	std::vector<AccessibleNode> getNodes(UnitInfoState *state);
	std::string getSummary(UnitInfoState *state) const;
	void activate(UnitInfoState *state, std::vector<std::string> &messages);
	bool handle(Action *action, UnitInfoState *state, InteractiveSurface *focused, std::vector<std::string> &messages);
};

}
