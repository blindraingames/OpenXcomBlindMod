#pragma once

#include "StateAccessibility.h"

namespace OpenXcom
{

class Surface;
class Text;

class GenericStateAccessibility : public StateAccessibility
{
private:
	std::string findLabel(State *state, Surface *control) const;
	std::string describeListRow(State *state, class TextList *list) const;
public:
	std::string getTitle(State *state) const;
	std::string getSummary(State *state) const;
	std::vector<AccessibleNode> getNodes(State *state) const;
};

}
