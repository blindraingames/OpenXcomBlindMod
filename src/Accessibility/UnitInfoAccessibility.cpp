#include "UnitInfoAccessibility.h"
#include "AccessibilityInput.h"
#include "../Battlescape/UnitInfoState.h"
#include "../Engine/Action.h"
#include "../Interface/TextButton.h"
#include <SDL.h>
#include <algorithm>
#include <sstream>
#include <utility>

namespace OpenXcom
{

UnitInfoAccessibility::UnitInfoAccessibility() : _state(0), _index(0), _initialized(false)
{
}

void UnitInfoAccessibility::reset()
{
	_state = 0;
	_index = 0;
	_initialized = false;
}

bool UnitInfoAccessibility::supports(State *state)
{
	return dynamic_cast<UnitInfoState*>(state) != 0;
}

void UnitInfoAccessibility::normalize(UnitInfoState *state)
{
	const std::size_t count = state ? state->getStatRows().size() : 0;
	if (count == 0) _index = 0;
	else if (_index >= count) _index = count - 1;
}

std::string UnitInfoAccessibility::describe(UnitInfoState *state) const
{
	if (!state) return std::string();
	const std::vector<std::pair<std::string, std::string> > rows = state->getStatRows();
	if (rows.empty()) return state->tr("STR_ACCESSIBILITY_UNIT_INFO_EMPTY");
	const std::size_t index = std::min(_index, rows.size() - 1);
	return state->tr("STR_ACCESSIBILITY_UNIT_INFO_FIELD")
		.arg(rows[index].first).arg(rows[index].second)
		.arg(index + 1).arg(rows.size());
}

InteractiveSurface *UnitInfoAccessibility::getPreferredControl(UnitInfoState *state) const
{
	return state ? state->getInfoControl() : 0;
}

std::vector<AccessibleNode> UnitInfoAccessibility::getNodes(UnitInfoState *state)
{
	std::vector<AccessibleNode> result;
	if (!state || !state->getInfoControl()) return result;
	normalize(state);

	AccessibleNode list;
	list.control = state->getInfoControl();
	list.role = ACCESSIBLE_LIST;
	list.roleName = state->tr("STR_ACCESSIBILITY_ROLE_LIST");
	list.name = state->tr(state->isMindProbe() ? "STR_ACCESSIBILITY_MIND_PROBE_FIELDS" : "STR_ACCESSIBILITY_UNIT_STATS_FIELDS");
	list.value = describe(state);
	result.push_back(list);

	if (state->getPreviousButton())
	{
		AccessibleNode previous;
		previous.control = state->getPreviousButton();
		previous.role = ACCESSIBLE_BUTTON;
		previous.roleName = state->tr("STR_ACCESSIBILITY_ROLE_BUTTON");
		previous.name = state->tr("STR_PREVIOUS_UNIT");
		result.push_back(previous);
	}
	if (state->getNextButton())
	{
		AccessibleNode next;
		next.control = state->getNextButton();
		next.role = ACCESSIBLE_BUTTON;
		next.roleName = state->tr("STR_ACCESSIBILITY_ROLE_BUTTON");
		next.name = state->tr("STR_NEXT_UNIT");
		result.push_back(next);
	}
	return result;
}

std::string UnitInfoAccessibility::getSummary(UnitInfoState *state) const
{
	if (!state) return std::string();
	const std::size_t count = state->getStatRows().size();
	return state->tr(state->isMindProbe() ? "STR_ACCESSIBILITY_MIND_PROBE_SUMMARY" : "STR_ACCESSIBILITY_UNIT_STATS_SUMMARY")
		.arg(state->getDisplayedName()).arg(count);
}

void UnitInfoAccessibility::activate(UnitInfoState *state, std::vector<std::string> &messages)
{
	if (!state || (_initialized && _state == state)) return;
	_state = state;
	_index = 0;
	_initialized = true;
	normalize(state);
	messages.push_back(getSummary(state));
	messages.push_back(describe(state));
}

void UnitInfoAccessibility::move(UnitInfoState *state, int delta, std::vector<std::string> &messages)
{
	const std::vector<std::pair<std::string, std::string> > rows = state->getStatRows();
	if (rows.empty())
	{
		messages.push_back(state->tr("STR_ACCESSIBILITY_UNIT_INFO_EMPTY"));
		return;
	}
	const int count = static_cast<int>(rows.size());
	_index = static_cast<std::size_t>((static_cast<int>(_index) + delta + count) % count);
	messages.push_back(describe(state));
}

bool UnitInfoAccessibility::handle(Action *action, UnitInfoState *state, InteractiveSurface *focused, std::vector<std::string> &messages)
{
	if (!action || !state || focused != state->getInfoControl() || action->getDetails()->type != SDL_KEYDOWN) return false;
	if (!_initialized || _state != state) activate(state, messages);
	const SDLKey key = static_cast<SDLKey>(AccessibilityInput::getKey(action));
	const int modifiers = AccessibilityInput::getModifiers(action);
	if ((modifiers & (KMOD_CTRL | KMOD_ALT)) != 0) return false;

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
		const std::vector<std::pair<std::string, std::string> > rows = state->getStatRows();
		if (!rows.empty()) _index = key == SDLK_HOME ? 0 : rows.size() - 1;
		messages.push_back(describe(state));
		return true;
	}
	if (key == SDLK_RETURN || key == SDLK_KP_ENTER || key == SDLK_SPACE)
	{
		messages.push_back(describe(state));
		return true;
	}
	return false;
}

}
