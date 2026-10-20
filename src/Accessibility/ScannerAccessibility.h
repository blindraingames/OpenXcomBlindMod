#pragma once

#include "AccessibleNode.h"
#include <cstddef>
#include <string>
#include <vector>

namespace OpenXcom
{

class Action;
class InteractiveSurface;
class ScannerState;
class State;

/** Keyboard and speech adapter for the battlescape motion scanner. */
class ScannerAccessibility
{
private:
	ScannerState *_state;
	std::size_t _index;
	bool _initialized;

	void normalize(ScannerState *state);
	std::string describe(ScannerState *state) const;
	void move(ScannerState *state, int delta, std::vector<std::string> &messages);

public:
	ScannerAccessibility();
	void reset();
	static bool supports(State *state);
	InteractiveSurface *getPreferredControl(ScannerState *state) const;
	std::vector<AccessibleNode> getNodes(ScannerState *state);
	std::string getSummary(ScannerState *state) const;
	void activate(ScannerState *state, std::vector<std::string> &messages);
	bool handle(Action *action, ScannerState *state, InteractiveSurface *focused, std::vector<std::string> &messages);
};

}
