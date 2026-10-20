#include "BattlescapeAccessibility.h"
#include "AccessibilityInput.h"
#include "../Battlescape/BattlescapeGame.h"
#include "../Battlescape/BattlescapeState.h"
#include "../Battlescape/Camera.h"
#include "../Battlescape/Map.h"
#include "../Battlescape/Pathfinding.h"
#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Options.h"
#include "../Mod/MapData.h"
#include "../Mod/RuleItem.h"
#include "../Savegame/BattleItem.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Tile.h"
#include <SDL.h>
#include <algorithm>
#include <cstdlib>
#include <sstream>
#include <utility>

namespace OpenXcom
{

namespace
{
	std::string text(State *state, const std::string &id)
	{
		return state->tr(id);
	}

	void appendSentence(std::ostringstream &out, const std::string &sentence)
	{
		if (sentence.empty()) return;
		if (out.tellp() > 0) out << " ";
		out << sentence;
	}

	bool isDiscovered(Tile *tile)
	{
		return tile != 0 && tile->isDiscovered(2);
	}
}

BattlescapeAccessibility::BattlescapeAccessibility() : _mode(MODE_EXPLORE), _cursor(0, 0, 0), _initialized(false), _targetAction(BA_SNAPSHOT), _visibleEnemyIndex(-1), _visibleObjectIndex(-1), _doorIndex(-1), _lastTurn(-1), _lastSide(-1), _lastPsiAttackResult(0)
{
}

void BattlescapeAccessibility::reset()
{
	_mode = MODE_EXPLORE;
	_cursor = Position(0, 0, 0);
	_initialized = false;
	_targetAction = BA_SNAPSHOT;
	_visibleEnemyIndex = -1;
	_visibleObjectIndex = -1;
	_doorIndex = -1;
	_lastTurn = -1;
	_lastSide = -1;
	_lastPsiAttackResult = 0;
	_knownVisibleEnemies.clear();
	_knownHealth.clear();
	_knownStatus.clear();
}

std::string BattlescapeAccessibility::modeName(BattlescapeState *state) const
{
	switch (_mode)
	{
	case MODE_MOVE:
		return text(state, "STR_ACCESSIBILITY_MODE_MOVE");
	case MODE_TARGET:
		return text(state, "STR_ACCESSIBILITY_MODE_TARGET");
	case MODE_EXPLORE:
	default:
		return text(state, "STR_ACCESSIBILITY_MODE_EXPLORE");
	}
}

std::string BattlescapeAccessibility::directionName(BattlescapeState *state, int direction) const
{
	static const char *directionIds[8] =
	{
		"STR_NORTH", "STR_NORTH_EAST", "STR_EAST", "STR_SOUTH_EAST",
		"STR_SOUTH", "STR_SOUTH_WEST", "STR_WEST", "STR_NORTH_WEST"
	};
	if (direction < 0 || direction > 7) return text(state, "STR_ACCESSIBILITY_UNKNOWN");
	return text(state, directionIds[direction]);
}

std::string BattlescapeAccessibility::statusName(BattlescapeState *state, int status) const
{
	switch (status)
	{
	case STATUS_STANDING: return text(state, "STR_ACCESSIBILITY_STATUS_STANDING");
	case STATUS_WALKING: return text(state, "STR_ACCESSIBILITY_STATUS_WALKING");
	case STATUS_FLYING: return text(state, "STR_ACCESSIBILITY_STATUS_FLYING");
	case STATUS_TURNING: return text(state, "STR_ACCESSIBILITY_STATUS_TURNING");
	case STATUS_AIMING: return text(state, "STR_ACCESSIBILITY_STATUS_AIMING");
	case STATUS_COLLAPSING: return text(state, "STR_ACCESSIBILITY_STATUS_COLLAPSING");
	case STATUS_DEAD: return text(state, "STR_ACCESSIBILITY_STATUS_DEAD");
	case STATUS_UNCONSCIOUS: return text(state, "STR_ACCESSIBILITY_STATUS_UNCONSCIOUS");
	case STATUS_PANICKING: return text(state, "STR_ACCESSIBILITY_STATUS_PANICKING");
	case STATUS_BERSERK: return text(state, "STR_ACCESSIBILITY_STATUS_BERSERK");
	default: return text(state, "STR_ACCESSIBILITY_UNKNOWN");
	}
}

void BattlescapeAccessibility::syncCursorToSelected(BattlescapeState *state)
{
	SavedBattleGame *save = state->getBattleGame()->getSave();
	BattleUnit *unit = save->getSelectedUnit();
	if (unit)
	{
		_cursor = unit->getPosition();
	}
	else
	{
		_cursor.z = state->getMap()->getCamera()->getViewLevel();
	}
	state->getMap()->setSelectorPosition(_cursor);
}

std::string BattlescapeAccessibility::describeRelativePosition(BattlescapeState *state) const
{
	BattleUnit *unit = state->getBattleGame()->getSave()->getSelectedUnit();
	if (!unit) return std::string();

	Position origin = unit->getPosition();
	int dx = _cursor.x - origin.x;
	int dy = _cursor.y - origin.y;
	int dz = _cursor.z - origin.z;
	if (dx == 0 && dy == 0 && dz == 0)
	{
		return text(state, "STR_ACCESSIBILITY_SELECTED_POSITION");
	}

	std::ostringstream result;
	if (dx != 0 || dy != 0)
	{
		int direction = 0;
		if (dy < 0 && dx == 0) direction = 0;
		else if (dy < 0 && dx > 0) direction = 1;
		else if (dy == 0 && dx > 0) direction = 2;
		else if (dy > 0 && dx > 0) direction = 3;
		else if (dy > 0 && dx == 0) direction = 4;
		else if (dy > 0 && dx < 0) direction = 5;
		else if (dy == 0 && dx < 0) direction = 6;
		else direction = 7;
		int distance = std::max(std::abs(dx), std::abs(dy));
		result << state->tr("STR_ACCESSIBILITY_RELATIVE_POSITION").arg(directionName(state, direction)).arg(distance);
	}
	if (dz != 0)
	{
		if (result.tellp() > 0) result << " ";
		result << state->tr(dz > 0 ? "STR_ACCESSIBILITY_LEVELS_ABOVE" : "STR_ACCESSIBILITY_LEVELS_BELOW").arg(std::abs(dz));
	}
	return result.str();
}

std::string BattlescapeAccessibility::describeUnit(BattlescapeState *state, BattleUnit *unit, bool full) const
{
	if (!unit) return text(state, "STR_ACCESSIBILITY_NO_SELECTED_SOLDIER");

	Language *language = state->getGame()->getLanguage();
	std::string name = unit->getName(language, false);
	BattleItem *mainWeapon = unit->getMainHandWeapon(false);
	std::string weapon = mainWeapon ? text(state, mainWeapon->getRules()->getName()) : text(state, "STR_NONE");
	if (!full)
	{
		return state->tr("STR_ACCESSIBILITY_SOLDIER_SHORT").arg(name).arg(unit->getTimeUnits()).arg(weapon);
	}

	std::string rank = unit->getRankString().empty() ? text(state, "STR_NONE") : text(state, unit->getRankString());
	BattleItem *right = unit->getItem("STR_RIGHT_HAND");
	BattleItem *left = unit->getItem("STR_LEFT_HAND");
	std::string rightName = right ? text(state, right->getRules()->getName()) : text(state, "STR_NONE");
	std::string leftName = left ? text(state, left->getRules()->getName()) : text(state, "STR_NONE");
	Position pos = unit->getPosition();
	return state->tr("STR_ACCESSIBILITY_SOLDIER_FULL")
		.arg(name)
		.arg(rank)
		.arg(pos.x)
		.arg(pos.y)
		.arg(pos.z + 1)
		.arg(directionName(state, unit->getDirection()))
		.arg(unit->getTimeUnits())
		.arg(unit->getHealth())
		.arg(unit->getEnergy())
		.arg(unit->getMorale())
		.arg(statusName(state, unit->getStatus()))
		.arg(rightName)
		.arg(leftName);
}

std::string BattlescapeAccessibility::describePath(BattlescapeState *state, bool *canMove, int *cost) const
{
	*canMove = false;
	*cost = 0;
	SavedBattleGame *save = state->getBattleGame()->getSave();
	BattleUnit *unit = save->getSelectedUnit();
	Tile *destination = save->getTile(_cursor);
	if (!unit) return text(state, "STR_ACCESSIBILITY_NO_SELECTED_SOLDIER");
	if (!isDiscovered(destination)) return text(state, "STR_ACCESSIBILITY_PATH_UNEXPLORED");
	if (unit->getPosition() == _cursor) return text(state, "STR_ACCESSIBILITY_ALREADY_THERE");

	Pathfinding *pathfinding = state->getBattleGame()->getPathfinding();
	pathfinding->calculate(unit, _cursor);
	if (pathfinding->getStartDirection() == -1)
	{
		pathfinding->abortPath();
		return text(state, "STR_ACCESSIBILITY_NO_PATH");
	}

	bool crossesUnexplored = false;
	Position pathPosition = unit->getPosition();
	const std::vector<int> &path = pathfinding->getPath();
	for (std::vector<int>::const_reverse_iterator i = path.rbegin(); i != path.rend(); ++i)
	{
		Position vector;
		Pathfinding::directionToVector(*i, &vector);
		pathPosition += vector;
		if (!isDiscovered(save->getTile(pathPosition)))
		{
			crossesUnexplored = true;
			break;
		}
	}
	*cost = pathfinding->getTotalTUCost();
	pathfinding->abortPath();
	if (crossesUnexplored) return text(state, "STR_ACCESSIBILITY_PATH_UNEXPLORED");
	if (*cost > unit->getTimeUnits())
	{
		return state->tr("STR_ACCESSIBILITY_PATH_TOO_EXPENSIVE").arg(*cost).arg(unit->getTimeUnits());
	}
	*canMove = true;
	return state->tr("STR_ACCESSIBILITY_PATH_COST").arg(*cost).arg(unit->getTimeUnits());
}

std::string BattlescapeAccessibility::describeTile(BattlescapeState *state, bool includePath) const
{
	SavedBattleGame *save = state->getBattleGame()->getSave();
	Tile *tile = save->getTile(_cursor);
	std::ostringstream result;
	appendSentence(result, state->tr("STR_ACCESSIBILITY_COORDINATES").arg(_cursor.x).arg(_cursor.y).arg(_cursor.z + 1));
	appendSentence(result, describeRelativePosition(state));
	if (!tile || !tile->isDiscovered(2))
	{
		appendSentence(result, text(state, "STR_ACCESSIBILITY_UNEXPLORED_TILE"));
		return result.str();
	}

	if (tile->getMapData(O_FLOOR)) appendSentence(result, text(state, "STR_ACCESSIBILITY_FLOOR"));
	else appendSentence(result, text(state, "STR_ACCESSIBILITY_NO_FLOOR"));

	MapData *westWall = tile->getMapData(O_WESTWALL);
	MapData *northWall = tile->getMapData(O_NORTHWALL);
	MapData *object = tile->getMapData(O_OBJECT);
	if (tile->isDiscovered(0) && westWall)
	{
		appendSentence(result, text(state, westWall->isDoor() || westWall->isUFODoor() ? "STR_ACCESSIBILITY_WEST_DOOR" : "STR_ACCESSIBILITY_WEST_WALL"));
	}
	if (tile->isDiscovered(1) && northWall)
	{
		appendSentence(result, text(state, northWall->isDoor() || northWall->isUFODoor() ? "STR_ACCESSIBILITY_NORTH_DOOR" : "STR_ACCESSIBILITY_NORTH_WALL"));
	}
	if (object)
	{
		appendSentence(result, text(state, object->getTUCost(MT_WALK) >= 255 ? "STR_ACCESSIBILITY_OBSTACLE" : "STR_ACCESSIBILITY_OBJECT"));
	}

	if (tile->getVisible() > 0)
	{
		if (tile->getSmoke() > 0) appendSentence(result, state->tr("STR_ACCESSIBILITY_SMOKE").arg(tile->getSmoke()));
		if (tile->getFire() > 0) appendSentence(result, state->tr("STR_ACCESSIBILITY_FIRE").arg(tile->getFire()));
		if (!tile->getInventory()->empty()) appendSentence(result, state->tr("STR_ACCESSIBILITY_ITEMS").arg(tile->getInventory()->size()));

		BattleUnit *unit = tile->getUnit();
		if (unit && (unit->getFaction() == FACTION_PLAYER || unit->getVisible()))
		{
			std::string role;
			if (unit->getFaction() == FACTION_PLAYER) role = text(state, "STR_ACCESSIBILITY_FRIENDLY_UNIT");
			else if (unit->getFaction() == FACTION_HOSTILE) role = text(state, "STR_ACCESSIBILITY_HOSTILE_UNIT");
			else role = text(state, "STR_ACCESSIBILITY_NEUTRAL_UNIT");
			appendSentence(result, state->tr("STR_ACCESSIBILITY_UNIT_ON_TILE").arg(role).arg(unit->getName(state->getGame()->getLanguage(), false)));
		}
	}

	if (includePath)
	{
		bool canMove = false;
		int cost = 0;
		appendSentence(result, describePath(state, &canMove, &cost));
	}
	return result.str();
}

void BattlescapeAccessibility::moveCursor(BattlescapeState *state, int dx, int dy, int dz, std::vector<std::string> &messages)
{
	SavedBattleGame *save = state->getBattleGame()->getSave();
	Position next(_cursor.x + dx, _cursor.y + dy, _cursor.z + dz);
	next.x = std::max(0, std::min(save->getMapSizeX() - 1, next.x));
	next.y = std::max(0, std::min(save->getMapSizeY() - 1, next.y));
	next.z = std::max(0, std::min(save->getMapSizeZ() - 1, next.z));
	if (next == _cursor)
	{
		messages.push_back(text(state, "STR_ACCESSIBILITY_MAP_EDGE"));
		return;
	}
	_cursor = next;
	state->getMap()->setSelectorPosition(_cursor);
	messages.push_back(describeTile(state, _mode == MODE_MOVE));
}

bool BattlescapeAccessibility::prepareTargetMode(BattlescapeState *state, std::vector<std::string> &messages)
{
	BattleUnit *unit = state->getBattleGame()->getSave()->getSelectedUnit();
	BattleItem *weapon = unit ? unit->getItem(unit->getActiveHand()) : 0;
	if (!weapon || weapon->getRules()->getBattleType() != BT_FIREARM) weapon = unit ? unit->getItem("STR_RIGHT_HAND") : 0;
	if (!weapon || weapon->getRules()->getBattleType() != BT_FIREARM) weapon = unit ? unit->getItem("STR_LEFT_HAND") : 0;
	if (weapon)
	{
		RuleItem *rules = weapon->getRules();
		if ((_targetAction == BA_SNAPSHOT && rules->getAccuracySnap() == 0) ||
			(_targetAction == BA_AIMEDSHOT && rules->getAccuracyAimed() == 0) ||
			(_targetAction == BA_AUTOSHOT && rules->getAccuracyAuto() == 0))
		{
			if (rules->getAccuracySnap() != 0) _targetAction = BA_SNAPSHOT;
			else if (rules->getAccuracyAimed() != 0) _targetAction = BA_AIMEDSHOT;
			else if (rules->getAccuracyAuto() != 0) _targetAction = BA_AUTOSHOT;
		}
	}
	return prepareAction(state, _targetAction, messages);
}

std::string BattlescapeAccessibility::actionName(BattlescapeState *state, BattleActionType type) const
{
	switch (type)
	{
	case BA_AUTOSHOT: return text(state, "STR_AUTO_SHOT");
	case BA_SNAPSHOT: return text(state, "STR_SNAP_SHOT");
	case BA_AIMEDSHOT: return text(state, "STR_AIMED_SHOT");
	case BA_THROW: return text(state, "STR_THROW");
	case BA_HIT: return text(state, "STR_HIT_MELEE");
	case BA_LAUNCH: return text(state, "STR_LAUNCH_MISSILE");
	case BA_PANIC: return text(state, "STR_PANIC_UNIT");
	case BA_MINDCONTROL: return text(state, "STR_MIND_CONTROL");
	default: return text(state, "STR_ACCESSIBILITY_UNKNOWN");
	}
}

bool BattlescapeAccessibility::prepareAction(BattlescapeState *state, BattleActionType type, std::vector<std::string> &messages)
{
	BattlescapeGame *battle = state->getBattleGame();
	SavedBattleGame *save = battle->getSave();
	BattleUnit *unit = save->getSelectedUnit();
	if (!unit)
	{
		messages.push_back(text(state, "STR_ACCESSIBILITY_NO_SELECTED_SOLDIER"));
		return false;
	}
	if (!state->allowButtons() || battle->isBusy())
	{
		messages.push_back(text(state, "STR_ACCESSIBILITY_ACTION_IN_PROGRESS"));
		return false;
	}

	BattleItem *weapon = 0;
	if (type == BA_PANIC || type == BA_MINDCONTROL)
	{
		weapon = unit->getSpecialWeapon(BT_PSIAMP);
	}
	else if (type == BA_HIT)
	{
		weapon = unit->getItem(unit->getActiveHand());
		if (!weapon || weapon->getRules()->getTUMelee() == 0) weapon = unit->getMeleeWeapon();
	}
	else
	{
		weapon = unit->getItem(unit->getActiveHand());
		if (!weapon) weapon = unit->getItem("STR_RIGHT_HAND");
		if (!weapon) weapon = unit->getItem("STR_LEFT_HAND");
	}
	if (!weapon)
	{
		messages.push_back(text(state, type == BA_HIT ? "STR_ACCESSIBILITY_NO_MELEE_WEAPON" : (type == BA_PANIC || type == BA_MINDCONTROL ? "STR_ACCESSIBILITY_NO_PSI_AMP" : "STR_ACCESSIBILITY_NO_USABLE_ITEM")));
		return false;
	}
	RuleItem *rules = weapon->getRules();
	if ((type == BA_SNAPSHOT && rules->getAccuracySnap() == 0) ||
		(type == BA_AIMEDSHOT && rules->getAccuracyAimed() == 0) ||
		(type == BA_AUTOSHOT && rules->getAccuracyAuto() == 0) ||
		(type == BA_HIT && rules->getTUMelee() == 0) ||
		(type == BA_THROW && rules->isFixed()))
	{
		messages.push_back(text(state, "STR_ACCESSIBILITY_ACTION_NOT_AVAILABLE"));
		return false;
	}
	const bool firing = type == BA_SNAPSHOT || type == BA_AIMEDSHOT || type == BA_AUTOSHOT || type == BA_LAUNCH;
	if (type == BA_LAUNCH && rules->getWaypoints() == 0 && (!weapon->getAmmoItem() || weapon->getAmmoItem()->getRules()->getWaypoints() == 0))
	{
		messages.push_back(text(state, "STR_ACCESSIBILITY_ACTION_NOT_AVAILABLE"));
		return false;
	}
	if (firing && weapon->needsAmmo() && (!weapon->getAmmoItem() || weapon->getAmmoItem()->getAmmoQuantity() <= 0))
	{
		messages.push_back(text(state, "STR_NO_AMMUNITION_LOADED"));
		return false;
	}
	std::string usable = save->getItemUsable(weapon);
	if (!usable.empty())
	{
		messages.push_back(text(state, usable));
		return false;
	}
	if (unit->getOriginalFaction() == FACTION_PLAYER && !state->getGame()->getSavedGame()->isResearched(rules->getRequirements()))
	{
		messages.push_back(text(state, "STR_UNABLE_TO_USE_ALIEN_ARTIFACT_UNTIL_RESEARCHED"));
		return false;
	}

	int tu = unit->getActionTUs(type, weapon);
	if (tu > unit->getTimeUnits())
	{
		messages.push_back(text(state, "STR_NOT_ENOUGH_TIME_UNITS"));
		return false;
	}

	battle->cancelAllActions();
	BattleAction *action = battle->getCurrentAction();
	action->actor = unit;
	action->weapon = weapon;
	action->type = type;
	action->TU = tu;
	action->target = _cursor;
	action->targeting = true;
	action->result.clear();
	battle->setupCursor();
	_targetAction = type;
	_mode = MODE_TARGET;
	messages.push_back(state->tr("STR_ACCESSIBILITY_TARGET_READY").arg(actionName(state, type)).arg(text(state, rules->getName())).arg(tu));
	return true;
}

void BattlescapeAccessibility::cycleFireMode(BattlescapeState *state, std::vector<std::string> &messages)
{
	BattleUnit *unit = state->getBattleGame()->getSave()->getSelectedUnit();
	BattleItem *weapon = unit ? unit->getItem(unit->getActiveHand()) : 0;
	if (!weapon || weapon->getRules()->getBattleType() != BT_FIREARM) weapon = unit ? unit->getItem("STR_RIGHT_HAND") : 0;
	if (!weapon || weapon->getRules()->getBattleType() != BT_FIREARM) weapon = unit ? unit->getItem("STR_LEFT_HAND") : 0;
	if (!weapon)
	{
		messages.push_back(text(state, "STR_ACCESSIBILITY_NO_FIREARM"));
		return;
	}
	std::vector<BattleActionType> modes;
	if (weapon->getRules()->getAccuracySnap()) modes.push_back(BA_SNAPSHOT);
	if (weapon->getRules()->getAccuracyAimed()) modes.push_back(BA_AIMEDSHOT);
	if (weapon->getRules()->getAccuracyAuto()) modes.push_back(BA_AUTOSHOT);
	if (modes.empty())
	{
		messages.push_back(text(state, "STR_ACCESSIBILITY_NO_FIRE_MODE"));
		return;
	}
	std::vector<BattleActionType>::iterator current = std::find(modes.begin(), modes.end(), _targetAction);
	_targetAction = current == modes.end() || ++current == modes.end() ? modes.front() : *current;
	prepareAction(state, _targetAction, messages);
}

void BattlescapeAccessibility::selectVisibleEnemy(BattlescapeState *state, int delta, std::vector<std::string> &messages)
{
	BattleUnit *selected = state->getBattleGame()->getSave()->getSelectedUnit();
	std::vector<BattleUnit*> enemies;
	if (selected)
	{
		for (std::vector<BattleUnit*>::const_iterator i = selected->getVisibleUnits()->begin(); i != selected->getVisibleUnits()->end(); ++i)
		{
			if (*i && (*i)->getVisible() && (*i)->getFaction() != selected->getFaction() && !(*i)->isOut()) enemies.push_back(*i);
		}
	}
	if (enemies.empty())
	{
		_visibleEnemyIndex = -1;
		messages.push_back(text(state, "STR_ACCESSIBILITY_NO_VISIBLE_ENEMIES"));
		return;
	}
	const int count = static_cast<int>(enemies.size());
	if (_visibleEnemyIndex < 0 || _visibleEnemyIndex >= count)
		_visibleEnemyIndex = delta < 0 ? count - 1 : 0;
	else
		_visibleEnemyIndex = (_visibleEnemyIndex + delta + count) % count;
	BattleUnit *enemy = enemies[_visibleEnemyIndex];
	_cursor = enemy->getPosition();
	state->getMap()->setSelectorPosition(_cursor);
	state->getMap()->getCamera()->centerOnPosition(_cursor);
	messages.push_back(state->tr("STR_ACCESSIBILITY_VISIBLE_ENEMY_POSITION").arg(_visibleEnemyIndex + 1).arg(enemies.size()));
	messages.push_back(describeTile(state, _mode == MODE_MOVE));
}

void BattlescapeAccessibility::selectVisibleObject(BattlescapeState *state, int delta, std::vector<std::string> &messages)
{
	SavedBattleGame *save = state->getBattleGame()->getSave();
	std::vector<std::pair<BattleItem*, Position> > objects;
	std::vector<BattleUnit*> representedCorpses;
	for (int i = 0; i < save->getMapSizeXYZ(); ++i)
	{
		Tile *tile = save->getTiles()[i];
		if (!tile || tile->getVisible() <= 0) continue;
		for (std::vector<BattleItem*>::const_iterator item = tile->getInventory()->begin(); item != tile->getInventory()->end(); ++item)
		{
			if (!*item || !(*item)->getRules()) continue;
			RuleItem *rules = (*item)->getRules();
			if (rules->getFloorSprite() < 0) continue;

			BattleItem *representative = *item;
			BattleUnit *corpseUnit = (*item)->getUnit();
			if (rules->getBattleType() == BT_CORPSE && corpseUnit)
			{
				if (std::find(representedCorpses.begin(), representedCorpses.end(), corpseUnit) != representedCorpses.end()) continue;
				representedCorpses.push_back(corpseUnit);
				// Large corpses use several floor sprites. Find the recoverable,
				// named piece and announce the entire corpse only once.
				for (std::vector<BattleItem*>::const_iterator candidate = save->getItems()->begin(); candidate != save->getItems()->end(); ++candidate)
				{
					if (*candidate && (*candidate)->getUnit() == corpseUnit && (*candidate)->getRules() && (*candidate)->getRules()->isRecoverable())
					{
						representative = *candidate;
						break;
					}
				}
			}
			else if ((rules->getInventoryWidth() <= 0 || rules->getInventoryHeight() <= 0) && !rules->isRecoverable())
			{
				continue;
			}
			objects.push_back(std::make_pair(representative, tile->getPosition()));
		}
	}

	if (objects.empty())
	{
		_visibleObjectIndex = -1;
		messages.push_back(text(state, "STR_ACCESSIBILITY_NO_VISIBLE_OBJECTS"));
		return;
	}

	const int count = static_cast<int>(objects.size());
	if (_visibleObjectIndex < 0 || _visibleObjectIndex >= count)
		_visibleObjectIndex = delta < 0 ? count - 1 : 0;
	else
		_visibleObjectIndex = (_visibleObjectIndex + delta + count) % count;
	BattleItem *item = objects[_visibleObjectIndex].first;
	_cursor = objects[_visibleObjectIndex].second;
	state->getMap()->setSelectorPosition(_cursor);
	state->getMap()->getCamera()->centerOnPosition(_cursor);
	messages.push_back(state->tr("STR_ACCESSIBILITY_VISIBLE_OBJECT_POSITION")
		.arg(_visibleObjectIndex + 1).arg(objects.size()).arg(text(state, item->getRules()->getName())));
	messages.push_back(describeTile(state, _mode == MODE_MOVE));
}

void BattlescapeAccessibility::selectDoor(BattlescapeState *state, int delta, std::vector<std::string> &messages)
{
	SavedBattleGame *save = state->getBattleGame()->getSave();
	std::vector<std::pair<Position, TilePart> > doors;
	for (int i = 0; i < save->getMapSizeXYZ(); ++i)
	{
		Tile *tile = save->getTiles()[i];
		if (!tile) continue;
		for (int part = O_FLOOR; part <= O_OBJECT; ++part)
		{
			MapData *data = tile->getMapData(static_cast<TilePart>(part));
			if (data && (data->isDoor() || data->isUFODoor()))
			{
				doors.push_back(std::make_pair(tile->getPosition(), static_cast<TilePart>(part)));
			}
		}
	}

	if (doors.empty())
	{
		_doorIndex = -1;
		messages.push_back(text(state, "STR_ACCESSIBILITY_NO_DOORS"));
		return;
	}

	const int count = static_cast<int>(doors.size());
	if (_doorIndex < 0 || _doorIndex >= count)
		_doorIndex = delta < 0 ? count - 1 : 0;
	else
		_doorIndex = (_doorIndex + delta + count) % count;
	const Position position = doors[_doorIndex].first;
	const TilePart part = doors[_doorIndex].second;
	std::string location;
	if (part == O_WESTWALL) location = text(state, "STR_ACCESSIBILITY_DOOR_WEST_EDGE");
	else if (part == O_NORTHWALL) location = text(state, "STR_ACCESSIBILITY_DOOR_NORTH_EDGE");
	else if (part == O_FLOOR) location = text(state, "STR_ACCESSIBILITY_DOOR_FLOOR_PART");
	else location = text(state, "STR_ACCESSIBILITY_DOOR_OBJECT_PART");

	_cursor = position;
	state->getMap()->setSelectorPosition(_cursor);
	state->getMap()->getCamera()->centerOnPosition(_cursor);
	messages.push_back(state->tr("STR_ACCESSIBILITY_DOOR_POSITION")
		.arg(_doorIndex + 1).arg(doors.size()).arg(_cursor.x).arg(_cursor.y).arg(_cursor.z + 1).arg(location));
	const std::string relative = describeRelativePosition(state);
	if (!relative.empty()) messages.push_back(relative);
}

void BattlescapeAccessibility::cycleReserve(BattlescapeState *state, std::vector<std::string> &messages)
{
	BattlescapeGame *battle = state->getBattleGame();
	BattleActionType reserve = battle->getReservedAction();
	if (reserve == BA_NONE) reserve = BA_SNAPSHOT;
	else if (reserve == BA_SNAPSHOT) reserve = BA_AIMEDSHOT;
	else if (reserve == BA_AIMEDSHOT) reserve = BA_AUTOSHOT;
	else reserve = BA_NONE;
	battle->setTUReserved(reserve);
	messages.push_back(state->tr("STR_ACCESSIBILITY_RESERVE_SET").arg(reserve == BA_NONE ? text(state, "STR_NONE") : actionName(state, reserve)));
}

void BattlescapeAccessibility::setMode(BattlescapeState *state, Mode mode, std::vector<std::string> &messages)
{
	if (mode == MODE_TARGET)
	{
		if (!prepareTargetMode(state, messages)) return;
	}
	else if (_mode == MODE_TARGET || state->getBattleGame()->getCurrentAction()->targeting)
	{
		state->getBattleGame()->cancelCurrentAction(true);
	}
	_mode = mode;
	messages.insert(messages.begin(), modeName(state));
}

void BattlescapeAccessibility::selectUnit(BattlescapeState *state, bool previous, std::vector<std::string> &messages)
{
	if (!state->allowButtons())
	{
		messages.push_back(text(state, "STR_ACCESSIBILITY_ACTION_IN_PROGRESS"));
		return;
	}
	if (previous) state->selectPreviousPlayerUnit(true);
	else state->selectNextPlayerUnit(true, false);
	_mode = MODE_EXPLORE;
	syncCursorToSelected(state);
	BattleUnit *unit = state->getBattleGame()->getSave()->getSelectedUnit();
	messages.push_back(describeUnit(state, unit, false));
	messages.push_back(describeTile(state, false));
}

void BattlescapeAccessibility::activateAtCursor(BattlescapeState *state, std::vector<std::string> &messages)
{
	BattlescapeGame *battle = state->getBattleGame();
	if (_mode == MODE_EXPLORE)
	{
		messages.push_back(describeTile(state, false));
		return;
	}
	if (!state->allowButtons() || battle->isBusy())
	{
		messages.push_back(text(state, "STR_ACCESSIBILITY_ACTION_IN_PROGRESS"));
		return;
	}
	if (_mode == MODE_MOVE)
	{
		bool canMove = false;
		int cost = 0;
		std::string path = describePath(state, &canMove, &cost);
		if (!canMove)
		{
			messages.push_back(path);
			return;
		}
		battle->primaryAction(_cursor);
		// Path preview normally requires a confirming second mouse click. The
		// accessibility command is already an explicit confirmation.
		if (!battle->isBusy() && battle->getPathfinding()->isPathPreviewed())
		{
			battle->primaryAction(_cursor);
		}
		messages.push_back(state->tr("STR_ACCESSIBILITY_MOVEMENT_STARTED").arg(cost));
		return;
	}

	BattleAction *current = battle->getCurrentAction();
	if (!current->targeting && !prepareTargetMode(state, messages)) return;
	current = battle->getCurrentAction();
	const BattleActionType type = current->type;
	Tile *tile = battle->getSave()->getTile(_cursor);
	BattleUnit *target = tile ? tile->getUnit() : 0;
	const bool mindProbe = type == BA_USE && current->weapon && current->weapon->getRules()->getBattleType() == BT_MINDPROBE;
	const bool requiresVisibleTarget = type != BA_THROW && type != BA_LAUNCH;
	if (!tile || (requiresVisibleTarget ? tile->getVisible() <= 0 : !isDiscovered(tile)))
	{
		messages.push_back(text(state, "STR_ACCESSIBILITY_TARGET_NOT_VISIBLE"));
		return;
	}
	battle->primaryAction(_cursor);
	if (type == BA_LAUNCH)
	{
		messages.push_back(state->tr("STR_ACCESSIBILITY_WAYPOINT_ADDED").arg(battle->getCurrentAction()->waypoints.size()));
		return;
	}
	if (type == BA_PANIC || type == BA_MINDCONTROL)
	{
		messages.push_back(state->tr("STR_ACCESSIBILITY_PSI_ATTACK_STARTED")
			.arg(actionName(state, type))
			.arg(target ? target->getName(state->getGame()->getLanguage(), false) : text(state, "STR_ACCESSIBILITY_UNKNOWN")));
		return;
	}
	if (mindProbe)
	{
		messages.push_back(text(state, "STR_ACCESSIBILITY_MIND_PROBE_OPENING"));
		return;
	}
	// Confirm-fire mode normally requires a second click on the same tile.
	// Enter is the explicit confirmation in the accessibility interface.
	if (!battle->isBusy() && battle->getCurrentAction()->targeting && !battle->getCurrentAction()->waypoints.empty())
	{
		battle->primaryAction(_cursor);
	}
	messages.push_back(text(state, "STR_ACCESSIBILITY_SHOT_STARTED"));
}

void BattlescapeAccessibility::useSecondaryAction(BattlescapeState *state, std::vector<std::string> &messages)
{
	BattlescapeGame *battle = state->getBattleGame();
	SavedBattleGame *save = battle->getSave();
	BattleUnit *unit = save->getSelectedUnit();
	if (!state->allowButtons() || battle->isBusy() || !unit)
	{
		messages.push_back(text(state, "STR_ACCESSIBILITY_ACTION_IN_PROGRESS"));
		return;
	}
	Tile *tile = save->getTile(_cursor);
	if (!isDiscovered(tile))
	{
		messages.push_back(text(state, "STR_ACCESSIBILITY_UNEXPLORED_TILE"));
		return;
	}

	Position target = _cursor;
	if (_cursor == unit->getPosition())
	{
		// Doors are stored on the west or north edge of a tile. Targeting the
		// soldier's own tile yields no facing direction, so point through a door
		// on that edge and let the normal secondary action turn and open it.
		MapData *west = tile->getMapData(O_WESTWALL);
		MapData *north = tile->getMapData(O_NORTHWALL);
		if (west && (west->isDoor() || west->isUFODoor())) target.x--;
		else if (north && (north->isDoor() || north->isUFODoor())) target.y--;
	}
	// A mouse right-click cancels targeting before it reaches secondaryAction.
	// Do the same for the keyboard command; otherwise a previously selected
	// fire/throw action makes UnitTurnBState skip its door-opening branch.
	battle->cancelAllActions();
	battle->secondaryAction(target);
	messages.push_back(text(state, "STR_ACCESSIBILITY_TURN_OR_DOOR"));
}

void BattlescapeAccessibility::activate(BattlescapeState *state, std::vector<std::string> &messages)
{
	if (!state || _initialized) return;
	_initialized = true;
	BattleAction *current = state->getBattleGame()->getCurrentAction();
	if (current && current->targeting)
	{
		_mode = MODE_TARGET;
		_targetAction = current->type;
	}
	else
	{
		_mode = MODE_EXPLORE;
	}
	syncCursorToSelected(state);
	SavedBattleGame *save = state->getBattleGame()->getSave();
	_lastTurn = save->getTurn();
	_lastSide = save->getSide();
	_lastPsiAttackResult = state->getBattleGame()->getPsiAttackResultSequence();
	for (std::vector<BattleUnit*>::const_iterator i = save->getUnits()->begin(); i != save->getUnits()->end(); ++i)
	{
		_knownHealth[*i] = (*i)->getHealth();
		_knownStatus[*i] = (*i)->getStatus();
	}
	BattleUnit *selected = save->getSelectedUnit();
	if (selected)
	{
		for (std::vector<BattleUnit*>::const_iterator i = selected->getVisibleUnits()->begin(); i != selected->getVisibleUnits()->end(); ++i)
		{
			if (*i && (*i)->getVisible() && (*i)->getFaction() != selected->getFaction()) _knownVisibleEnemies.push_back(*i);
		}
	}
	messages.push_back(text(state, "STR_ACCESSIBILITY_BATTLESCAPE"));
	messages.push_back(modeName(state));
	messages.push_back(describeUnit(state, state->getBattleGame()->getSave()->getSelectedUnit(), false));
	messages.push_back(describeTile(state, false));
}

void BattlescapeAccessibility::update(BattlescapeState *state, std::vector<std::string> &messages)
{
	if (!state || !_initialized) return;
	SavedBattleGame *save = state->getBattleGame()->getSave();
	BattlescapeGame *battle = state->getBattleGame();
	const int psiResult = battle->getPsiAttackResultSequence();
	if (_lastPsiAttackResult != psiResult)
	{
		_lastPsiAttackResult = psiResult;
		if (!battle->wasLastPsiAttackSuccessful() && (save->getSide() == FACTION_PLAYER || save->getDebugMode()))
		{
			BattleUnit *target = battle->getLastPsiAttackTarget();
			messages.push_back(state->tr("STR_ACCESSIBILITY_PSI_ATTACK_FAILED")
				.arg(actionName(state, battle->getLastPsiAttackType()))
				.arg(target ? target->getName(state->getGame()->getLanguage(), false) : text(state, "STR_ACCESSIBILITY_UNKNOWN")));
		}
	}
	if (_lastTurn != save->getTurn() || _lastSide != save->getSide())
	{
		_lastTurn = save->getTurn();
		_lastSide = save->getSide();
		const std::string side = save->getSide() == FACTION_PLAYER ? text(state, "STR_ACCESSIBILITY_PLAYER_TURN") :
			(save->getSide() == FACTION_HOSTILE ? text(state, "STR_ACCESSIBILITY_ALIEN_TURN") : text(state, "STR_ACCESSIBILITY_NEUTRAL_TURN"));
		messages.push_back(state->tr("STR_ACCESSIBILITY_TURN_CHANGED").arg(_lastTurn).arg(side));
	}

	BattleUnit *selected = save->getSelectedUnit();
	std::vector<BattleUnit*> visible;
	if (selected)
	{
		for (std::vector<BattleUnit*>::const_iterator i = selected->getVisibleUnits()->begin(); i != selected->getVisibleUnits()->end(); ++i)
		{
			if (*i && (*i)->getVisible() && (*i)->getFaction() != selected->getFaction() && !(*i)->isOut()) visible.push_back(*i);
		}
	}
	for (std::vector<BattleUnit*>::const_iterator i = visible.begin(); i != visible.end(); ++i)
	{
		if (std::find(_knownVisibleEnemies.begin(), _knownVisibleEnemies.end(), *i) == _knownVisibleEnemies.end())
		{
			Position pos = (*i)->getPosition();
			messages.push_back(state->tr("STR_ACCESSIBILITY_ENEMY_SPOTTED")
				.arg((*i)->getName(state->getGame()->getLanguage(), false)).arg(pos.x).arg(pos.y).arg(pos.z + 1));
		}
	}
	_knownVisibleEnemies = visible;

	for (std::vector<BattleUnit*>::const_iterator i = save->getUnits()->begin(); i != save->getUnits()->end(); ++i)
	{
		BattleUnit *unit = *i;
		const bool mayAnnounce = unit->getOriginalFaction() == FACTION_PLAYER || std::find(visible.begin(), visible.end(), unit) != visible.end();
		if (!mayAnnounce)
		{
			_knownHealth[unit] = unit->getHealth();
			_knownStatus[unit] = unit->getStatus();
			continue;
		}
		std::map<BattleUnit*, int>::iterator oldHealth = _knownHealth.find(unit);
		if (oldHealth != _knownHealth.end() && unit->getHealth() < oldHealth->second)
		{
			messages.push_back(state->tr("STR_ACCESSIBILITY_UNIT_DAMAGED")
				.arg(unit->getName(state->getGame()->getLanguage(), false)).arg(oldHealth->second - unit->getHealth()).arg(unit->getHealth()));
		}
		std::map<BattleUnit*, int>::iterator oldStatus = _knownStatus.find(unit);
		if (oldStatus != _knownStatus.end() && unit->getStatus() != oldStatus->second)
		{
			messages.push_back(state->tr("STR_ACCESSIBILITY_UNIT_STATUS_CHANGED")
				.arg(unit->getName(state->getGame()->getLanguage(), false)).arg(statusName(state, unit->getStatus())));
		}
		_knownHealth[unit] = unit->getHealth();
		_knownStatus[unit] = unit->getStatus();
	}
}

bool BattlescapeAccessibility::handle(Action *action, BattlescapeState *state, std::vector<std::string> &messages)
{
	if (!action || !state || action->getDetails()->type != SDL_KEYDOWN) return false;
	if (!_initialized) activate(state, messages);

	SDLKey key = static_cast<SDLKey>(AccessibilityInput::getKey(action));
	int modifiers = AccessibilityInput::getModifiers(action);
	bool command = AccessibilityInput::isCommandChord(modifiers);
	if (command && key == SDLK_r)
	{
		messages.push_back(describeTile(state, _mode == MODE_MOVE));
		return true;
	}
	if (command && key == SDLK_s)
	{
		messages.push_back(modeName(state));
		messages.push_back(describeUnit(state, state->getBattleGame()->getSave()->getSelectedUnit(), false));
		messages.push_back(describeTile(state, _mode == MODE_MOVE));
		return true;
	}
	if (command && key == SDLK_i)
	{
		state->btnInventoryClick(action);
		messages.push_back(text(state, "STR_INVENTORY"));
		return true;
	}
	if (command && (key == SDLK_RETURN || key == SDLK_KP_ENTER))
	{
		BattleAction *current = state->getBattleGame()->getCurrentAction();
		if (current->type == BA_LAUNCH && current->targeting && !current->waypoints.empty())
		{
			state->btnLaunchClick(action);
			messages.push_back(text(state, "STR_ACCESSIBILITY_LAUNCH_STARTED"));
		}
		else
		{
			messages.push_back(text(state, "STR_ACCESSIBILITY_NO_WAYPOINTS"));
		}
		return true;
	}
	if (command && key == SDLK_f)
	{
		cycleFireMode(state, messages);
		return true;
	}
	if (command && key == SDLK_g)
	{
		prepareAction(state, BA_THROW, messages);
		return true;
	}
	if (command && key == SDLK_h)
	{
		prepareAction(state, BA_HIT, messages);
		return true;
	}
	if (command && key == SDLK_l)
	{
		prepareAction(state, BA_LAUNCH, messages);
		return true;
	}
	if (command && key == SDLK_p)
	{
		prepareAction(state, BA_PANIC, messages);
		return true;
	}
	if (command && key == SDLK_c)
	{
		prepareAction(state, BA_MINDCONTROL, messages);
		return true;
	}
	if (command && (key == SDLK_1 || key == SDLK_KP1))
	{
		state->btnLeftHandItemClick(action);
		messages.push_back(text(state, "STR_ACCESSIBILITY_LEFT_HAND_ACTIONS"));
		return true;
	}
	if (command && (key == SDLK_2 || key == SDLK_KP2))
	{
		state->btnRightHandItemClick(action);
		messages.push_back(text(state, "STR_ACCESSIBILITY_RIGHT_HAND_ACTIONS"));
		return true;
	}
	if (command && key == SDLK_a)
	{
		selectVisibleEnemy(state, (modifiers & KMOD_SHIFT) ? -1 : 1, messages);
		return true;
	}
	if (command && key == SDLK_x)
	{
		selectVisibleObject(state, (modifiers & KMOD_SHIFT) ? -1 : 1, messages);
		return true;
	}
	if (command && key == SDLK_v)
	{
		cycleReserve(state, messages);
		return true;
	}
	if (command && key == SDLK_k)
	{
		state->btnKneelClick(action);
		BattleUnit *unit = state->getBattleGame()->getSave()->getSelectedUnit();
		messages.push_back(text(state, unit && unit->isKneeled() ? "STR_ACCESSIBILITY_KNEELING" : "STR_ACCESSIBILITY_STANDING"));
		return true;
	}
	if (command && key == SDLK_o)
	{
		messages.push_back(describeUnit(state, state->getBattleGame()->getSave()->getSelectedUnit(), true));
		return true;
	}
	if (command && key == SDLK_b)
	{
		state->btnAbortClick(action);
		messages.push_back(text(state, "STR_ABORT_MISSION"));
		return true;
	}
	if (command && key == SDLK_w)
	{
		state->btnReloadClick(action);
		messages.push_back(text(state, "STR_ACCESSIBILITY_RELOAD_REQUESTED"));
		return true;
	}
	if (command && (key == SDLK_PAGEUP || key == SDLK_PAGEDOWN))
	{
		if (key == SDLK_PAGEUP) state->btnUnitUpClick(action);
		else state->btnUnitDownClick(action);
		messages.push_back(text(state, key == SDLK_PAGEUP ? "STR_ACCESSIBILITY_MOVE_UP_REQUESTED" : "STR_ACCESSIBILITY_MOVE_DOWN_REQUESTED"));
		return true;
	}
	if (command && key == SDLK_e)
	{
		setMode(state, MODE_EXPLORE, messages);
		return true;
	}
	if (command && key == SDLK_m)
	{
		setMode(state, MODE_MOVE, messages);
		return true;
	}
	if (command && key == SDLK_t)
	{
		setMode(state, MODE_TARGET, messages);
		return true;
	}
	if (command && key == SDLK_d)
	{
		selectDoor(state, (modifiers & KMOD_SHIFT) != 0 ? -1 : 1, messages);
		return true;
	}
	if (command && key == SDLK_q)
	{
		useSecondaryAction(state, messages);
		return true;
	}

	if (key == SDLK_TAB)
	{
		selectUnit(state, (modifiers & KMOD_SHIFT) != 0, messages);
		return true;
	}
	if ((key == SDLK_LSHIFT || key == SDLK_RSHIFT) &&
		(Options::keyBattlePrevUnit == SDLK_LSHIFT || Options::keyBattlePrevUnit == SDLK_RSHIFT))
	{
		// Shift is reserved as the modifier for Shift+Tab in accessibility mode.
		return true;
	}
	if (key == Options::keyBattlePrevUnit)
	{
		selectUnit(state, true, messages);
		return true;
	}
	if (key == Options::keyBattleNextUnit)
	{
		selectUnit(state, false, messages);
		return true;
	}
	if (key == SDLK_LEFT)
	{
		moveCursor(state, -1, 0, 0, messages);
		return true;
	}
	if (key == SDLK_RIGHT)
	{
		moveCursor(state, 1, 0, 0, messages);
		return true;
	}
	if (key == SDLK_UP)
	{
		moveCursor(state, 0, -1, 0, messages);
		return true;
	}
	if (key == SDLK_DOWN)
	{
		moveCursor(state, 0, 1, 0, messages);
		return true;
	}
	if (key == SDLK_PAGEUP)
	{
		moveCursor(state, 0, 0, 1, messages);
		return true;
	}
	if (key == SDLK_PAGEDOWN)
	{
		moveCursor(state, 0, 0, -1, messages);
		return true;
	}
	if (key == SDLK_HOME)
	{
		syncCursorToSelected(state);
		messages.push_back(describeTile(state, _mode == MODE_MOVE));
		return true;
	}
	if (key == Options::keyBattleEndTurn)
	{
		if (state->allowButtons())
		{
			state->btnEndTurnClick(action);
			messages.push_back(text(state, "STR_ACCESSIBILITY_END_TURN_REQUESTED"));
		}
		else
		{
			messages.push_back(text(state, "STR_ACCESSIBILITY_ACTION_IN_PROGRESS"));
		}
		return true;
	}
	if (key == SDLK_SPACE)
	{
		Mode next = _mode == MODE_EXPLORE ? MODE_MOVE : (_mode == MODE_MOVE ? MODE_TARGET : MODE_EXPLORE);
		setMode(state, next, messages);
		return true;
	}
	if (key == SDLK_RETURN || key == SDLK_KP_ENTER || key == Options::keyOk)
	{
		activateAtCursor(state, messages);
		return true;
	}
	if (key == SDLK_ESCAPE && _mode != MODE_EXPLORE)
	{
		setMode(state, MODE_EXPLORE, messages);
		return true;
	}
	return false;
}

}
