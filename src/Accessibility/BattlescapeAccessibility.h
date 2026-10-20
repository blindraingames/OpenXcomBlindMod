#pragma once

#include "../Battlescape/BattlescapeGame.h"
#include <string>
#include <vector>
#include <map>

namespace OpenXcom
{

class Action;
class BattleUnit;
class BattlescapeState;
class State;

/**
 * Keyboard and speech adapter for the tactical battlescape.
 *
 * This class never talks to a screen reader directly. It exposes spoken
 * messages to AccessibilityManager and delegates game actions to the normal
 * BattlescapeGame API.
 */
class BattlescapeAccessibility
{
public:
	enum Mode
	{
		MODE_EXPLORE,
		MODE_MOVE,
		MODE_TARGET
	};

private:
	Mode _mode;
	Position _cursor;
	bool _initialized;
	BattleActionType _targetAction;
	int _visibleEnemyIndex;
	int _visibleObjectIndex;
	int _doorIndex;
	int _lastTurn;
	int _lastSide;
	int _lastPsiAttackResult;
	std::vector<BattleUnit*> _knownVisibleEnemies;
	std::map<BattleUnit*, int> _knownHealth;
	std::map<BattleUnit*, int> _knownStatus;

	std::string modeName(BattlescapeState *state) const;
	std::string directionName(BattlescapeState *state, int direction) const;
	std::string statusName(BattlescapeState *state, int status) const;
	std::string describeRelativePosition(BattlescapeState *state) const;
	std::string describeTile(BattlescapeState *state, bool includePath) const;
	std::string describeUnit(BattlescapeState *state, BattleUnit *unit, bool full) const;
	std::string describePath(BattlescapeState *state, bool *canMove, int *cost) const;
	void moveCursor(BattlescapeState *state, int dx, int dy, int dz, std::vector<std::string> &messages);
	void syncCursorToSelected(BattlescapeState *state);
	bool prepareTargetMode(BattlescapeState *state, std::vector<std::string> &messages);
	bool prepareAction(BattlescapeState *state, BattleActionType type, std::vector<std::string> &messages);
	std::string actionName(BattlescapeState *state, BattleActionType type) const;
	void cycleFireMode(BattlescapeState *state, std::vector<std::string> &messages);
	void selectVisibleEnemy(BattlescapeState *state, int delta, std::vector<std::string> &messages);
	void selectVisibleObject(BattlescapeState *state, int delta, std::vector<std::string> &messages);
	void selectDoor(BattlescapeState *state, int delta, std::vector<std::string> &messages);
	void cycleReserve(BattlescapeState *state, std::vector<std::string> &messages);
	void setMode(BattlescapeState *state, Mode mode, std::vector<std::string> &messages);
	void selectUnit(BattlescapeState *state, bool previous, std::vector<std::string> &messages);
	void activateAtCursor(BattlescapeState *state, std::vector<std::string> &messages);
	void useSecondaryAction(BattlescapeState *state, std::vector<std::string> &messages);

public:
	BattlescapeAccessibility();
	void reset();
	void activate(BattlescapeState *state, std::vector<std::string> &messages);
	void update(BattlescapeState *state, std::vector<std::string> &messages);
	bool handle(Action *action, BattlescapeState *state, std::vector<std::string> &messages);
};

}
