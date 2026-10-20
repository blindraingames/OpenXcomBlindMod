#pragma once

#include <vector>
#include "AccessibleNode.h"

namespace OpenXcom
{

class InteractiveSurface;

class AccessibilityFocusManager
{
private:
	std::vector<AccessibleNode> _nodes;
	size_t _index;
public:
	AccessibilityFocusManager();
	void setNodes(const std::vector<AccessibleNode> &nodes, InteractiveSurface *preferred = 0);
	bool empty() const;
	size_t size() const;
	const AccessibleNode *current() const;
	AccessibleNode *current();
	const AccessibleNode *next();
	const AccessibleNode *previous();
	const AccessibleNode *first();
	const AccessibleNode *last();
};

}
