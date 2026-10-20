#pragma once

#include <deque>
#include <string>
#include "AccessibilityFocusManager.h"
#include "BasescapeAccessibility.h"
#include "BattlescapeAccessibility.h"
#include "GeoscapeAccessibility.h"
#include "GenericStateAccessibility.h"
#include "InventoryAccessibility.h"
#include "ScannerAccessibility.h"
#include "TolkBridge.h"
#include "UnitInfoAccessibility.h"

namespace OpenXcom
{

class Action;
class State;
class TextEdit;

class AccessibilityManager
{
private:
	TolkBridge _bridge;
	GenericStateAccessibility _adapter;
	BasescapeAccessibility *_basescape;
	BattlescapeAccessibility *_battlescape;
	GeoscapeAccessibility *_geoscape;
	InventoryAccessibility *_inventory;
	ScannerAccessibility *_scanner;
	UnitInfoAccessibility *_unitInfo;
	AccessibilityFocusManager _focus;
	State *_state;
	TextEdit *_editing;
	bool _enabled;
	bool _brandAnnounced;
	bool _screenAnnounced;
	std::string _lastMessage;
	std::string _screenSummary;
	std::deque<std::string> _history;

	void rebuildNodes(class InteractiveSurface *preferred = 0);
	void announceNode(const AccessibleNode *node, bool interrupt = true);
	void activateCurrent(Action *sourceAction);
	void adjustCurrent(int direction, Action *sourceAction);
	void remember(const std::string &text);
public:
	AccessibilityManager();
	~AccessibilityManager();
	bool isEnabled() const;
	void onStateActivated(State *state);
	void update(State *state);
	bool handle(Action *action, State *state);
	void announce(const std::string &text, bool interrupt = false);
	void silence();
};

}
