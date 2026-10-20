#pragma once

#include <string>

namespace OpenXcom
{

struct AccessibleNode;

class AccessibilityText
{
public:
	static std::string clean(const std::string &text);
	static std::string describe(const AccessibleNode &node);
};

}
