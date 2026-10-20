#pragma once

#include <string>
#include <vector>
#include "AccessibleNode.h"

namespace OpenXcom
{

class State;

class StateAccessibility
{
public:
	virtual ~StateAccessibility();
	virtual std::string getTitle(State *state) const = 0;
	virtual std::string getSummary(State *state) const = 0;
	virtual std::vector<AccessibleNode> getNodes(State *state) const = 0;
};

}
