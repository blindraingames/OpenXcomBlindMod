#include "InventoryAccessibility.h"
#include "../Battlescape/Inventory.h"
#include "../Battlescape/InventoryState.h"
#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Mod/RuleInventory.h"
#include "../Mod/RuleItem.h"
#include "../Savegame/BattleItem.h"
#include "../Savegame/BattleUnit.h"
#include <SDL.h>
#include <algorithm>
#include <sstream>

namespace OpenXcom
{

InventoryAccessibility::InventoryAccessibility() : _state(0), _current(0), _index(0), _initialized(false)
{
}

void InventoryAccessibility::reset()
{
	_state = 0;
	_current = 0;
	_index = 0;
	_initialized = false;
}

bool InventoryAccessibility::supports(State *state)
{
	return dynamic_cast<InventoryState*>(state) != 0;
}

std::vector<BattleItem*> InventoryAccessibility::items(InventoryState *state) const
{
	return state && state->getInventory() ? state->getInventory()->getAccessibleItems() : std::vector<BattleItem*>();
}

void InventoryAccessibility::normalize(InventoryState *state)
{
	std::vector<BattleItem*> available = items(state);
	if (available.empty())
	{
		_current = 0;
		_index = 0;
		return;
	}
	std::vector<BattleItem*>::iterator found = std::find(available.begin(), available.end(), _current);
	if (found == available.end())
	{
		_index = std::min(_index, available.size() - 1);
		_current = available[_index];
	}
	else
	{
		_index = static_cast<std::size_t>(found - available.begin());
	}
}

std::string InventoryAccessibility::describe(InventoryState *state) const
{
	if (!state || !_current) return state ? std::string(state->tr("STR_ACCESSIBILITY_INVENTORY_EMPTY")) : std::string();
	std::vector<BattleItem*> available = items(state);
	std::ostringstream details;
	details << state->tr(_current->getRules()->getName());
	if (_current->getSlot()) details << ". " << state->tr(_current->getSlot()->getId());
	if (_current->getAmmoItem())
	{
		details << ". " << state->tr("STR_ACCESSIBILITY_LOADED_AMMO")
			<< " " << state->tr(_current->getAmmoItem()->getRules()->getName())
			<< ", " << _current->getAmmoItem()->getAmmoQuantity();
	}
	else if (_current->getRules()->getBattleType() == BT_AMMO)
	{
		details << ". " << state->tr("STR_ACCESSIBILITY_AMMO_REMAINING").arg(_current->getAmmoQuantity());
	}
	if (_current->getFuseTimer() >= 0)
	{
		details << ". " << state->tr("STR_ACCESSIBILITY_GRENADE_TIMER").arg(_current->getFuseTimer());
	}
	details << ". " << state->tr("STR_ACCESSIBILITY_POSITION_OF").arg(_index + 1).arg(available.size());
	return details.str();
}

InteractiveSurface *InventoryAccessibility::getPreferredControl(InventoryState *state) const
{
	return state ? state->getInventory() : 0;
}

std::vector<AccessibleNode> InventoryAccessibility::getNodes(InventoryState *state)
{
	std::vector<AccessibleNode> result;
	if (!state || !state->getInventory()) return result;
	normalize(state);
	AccessibleNode node;
	node.control = state->getInventory();
	node.role = ACCESSIBLE_INVENTORY_SLOT;
	node.roleName = state->tr("STR_ACCESSIBILITY_ROLE_INVENTORY_SLOT");
	node.name = state->tr("STR_ACCESSIBILITY_INVENTORY_ITEMS");
	node.value = describe(state);
	node.description = state->tr("STR_ACCESSIBILITY_INVENTORY_HELP");
	result.push_back(node);
	return result;
}

std::string InventoryAccessibility::getSummary(InventoryState *state)
{
	if (!state) return std::string();
	BattleUnit *unit = state->getSelectedUnit();
	const std::string name = unit ? unit->getName(state->getGame()->getLanguage(), false) : std::string(state->tr("STR_NONE"));
	return state->tr("STR_ACCESSIBILITY_INVENTORY_SUMMARY").arg(name).arg(items(state).size());
}

void InventoryAccessibility::activate(InventoryState *state, std::vector<std::string> &messages)
{
	if (!state || (_initialized && _state == state)) return;
	_state = state;
	_initialized = true;
	_current = 0;
	_index = 0;
	normalize(state);
	messages.push_back(getSummary(state));
	messages.push_back(describe(state));
}

void InventoryAccessibility::move(InventoryState *state, int delta, std::vector<std::string> &messages)
{
	std::vector<BattleItem*> available = items(state);
	if (available.empty())
	{
		messages.push_back(state->tr("STR_ACCESSIBILITY_INVENTORY_EMPTY"));
		return;
	}
	const int count = static_cast<int>(available.size());
	_index = static_cast<std::size_t>((static_cast<int>(_index) + delta + count) % count);
	_current = available[_index];
	messages.push_back(describe(state));
}

bool InventoryAccessibility::handle(Action *action, InventoryState *state, InteractiveSurface *focused, std::vector<std::string> &messages)
{
	if (!action || !state || focused != state->getInventory() || action->getDetails()->type != SDL_KEYDOWN) return false;
	if (!_initialized || _state != state) activate(state, messages);
	const SDLKey key = action->getDetails()->key.keysym.sym;
	if (key == SDLK_LEFT || key == SDLK_UP || key == SDLK_PAGEUP)
	{
		move(state, -1, messages);
		return true;
	}
	if (key == SDLK_RIGHT || key == SDLK_DOWN || key == SDLK_PAGEDOWN)
	{
		move(state, 1, messages);
		return true;
	}
	if (key == SDLK_HOME || key == SDLK_END)
	{
		std::vector<BattleItem*> available = items(state);
		if (!available.empty())
		{
			_index = key == SDLK_HOME ? 0 : available.size() - 1;
			_current = available[_index];
		}
		messages.push_back(describe(state));
		return true;
	}
	if (key == SDLK_RETURN || key == SDLK_KP_ENTER || key == SDLK_SPACE)
	{
		std::string result;
		if (_current && state->getInventory()->accessibilityCycleMove(_current, result))
			messages.push_back(state->tr("STR_ACCESSIBILITY_ITEM_MOVED").arg(state->tr(result)));
		else
			messages.push_back(result.empty() ? std::string(state->tr("STR_ACCESSIBILITY_INVENTORY_ACTION_FAILED")) : std::string(state->tr(result)));
		normalize(state);
		messages.push_back(describe(state));
		return true;
	}
	if (key == SDLK_l || key == SDLK_u)
	{
		std::string result;
		state->getInventory()->accessibilityLoadOrUnload(_current, result);
		messages.push_back(result.empty() ? std::string(state->tr("STR_ACCESSIBILITY_INVENTORY_ACTION_FAILED")) : std::string(state->tr(result)));
		normalize(state);
		messages.push_back(describe(state));
		return true;
	}
	if (key == SDLK_p || (key >= SDLK_0 && key <= SDLK_9))
	{
		std::string result;
		const int timer = key >= SDLK_0 && key <= SDLK_9 ? key - SDLK_0 : 0;
		if (state->getInventory()->accessibilityTogglePrime(_current, timer, result))
			messages.push_back(state->tr(result));
		else
			messages.push_back(state->tr(state->isTuMode() ? "STR_ACCESSIBILITY_PRIME_FROM_BATTLE" : "STR_ACCESSIBILITY_INVENTORY_ACTION_FAILED"));
		messages.push_back(describe(state));
		return true;
	}
	return false;
}

}
