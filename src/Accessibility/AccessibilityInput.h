#pragma once

namespace OpenXcom
{

class Action;

class AccessibilityInput
{
public:
	static int getKey(Action *action);
	static int getModifiers(Action *action);
	static bool isCommandChord(int modifiers);
	static bool isForwardFocusKey(int key, int modifiers);
	static bool isBackwardFocusKey(int key, int modifiers);
};

}
