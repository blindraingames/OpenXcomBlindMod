#include "AccessibilityFocusManager.h"

namespace OpenXcom
{

AccessibilityFocusManager::AccessibilityFocusManager() : _index(0)
{
}

void AccessibilityFocusManager::setNodes(const std::vector<AccessibleNode> &nodes, InteractiveSurface *preferred)
{
	_nodes = nodes;
	_index = 0;
	if (preferred)
	{
		for (size_t i = 0; i < _nodes.size(); ++i)
		{
			if (_nodes[i].control == preferred)
			{
				_index = i;
				break;
			}
		}
	}
}

bool AccessibilityFocusManager::empty() const
{
	return _nodes.empty();
}

size_t AccessibilityFocusManager::size() const
{
	return _nodes.size();
}

const AccessibleNode *AccessibilityFocusManager::current() const
{
	return _nodes.empty() ? 0 : &_nodes[_index];
}

AccessibleNode *AccessibilityFocusManager::current()
{
	return _nodes.empty() ? 0 : &_nodes[_index];
}

const AccessibleNode *AccessibilityFocusManager::next()
{
	if (_nodes.empty()) return 0;
	_index = (_index + 1) % _nodes.size();
	return current();
}

const AccessibleNode *AccessibilityFocusManager::previous()
{
	if (_nodes.empty()) return 0;
	_index = (_index + _nodes.size() - 1) % _nodes.size();
	return current();
}

const AccessibleNode *AccessibilityFocusManager::first()
{
	if (_nodes.empty()) return 0;
	_index = 0;
	return current();
}

const AccessibleNode *AccessibilityFocusManager::last()
{
	if (_nodes.empty()) return 0;
	_index = _nodes.size() - 1;
	return current();
}

}
