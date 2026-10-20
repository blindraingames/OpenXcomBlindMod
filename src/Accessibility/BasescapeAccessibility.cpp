#include "BasescapeAccessibility.h"
#include "AccessibilityInput.h"
#include "../Basescape/BaseView.h"
#include "../Basescape/MiniBaseView.h"
#include "../Basescape/PlaceFacilityState.h"
#include "../Basescape/PlaceLiftState.h"
#include "../Engine/Action.h"
#include "../Engine/InteractiveSurface.h"
#include "../Engine/State.h"
#include "../Mod/RuleBaseFacility.h"
#include "../Savegame/BaseFacility.h"
#include "../Savegame/Base.h"
#include <SDL.h>
#include <climits>
#include <sstream>

namespace OpenXcom
{

namespace
{
	void appendSentence(std::ostringstream &out, const std::string &sentence)
	{
		if (sentence.empty()) return;
		if (out.tellp() > 0) out << " ";
		out << sentence;
	}
}

BasescapeAccessibility::BasescapeAccessibility() : _state(0), _view(0), _mini(0)
{
}

void BasescapeAccessibility::reset()
{
	_state = 0;
	_view = 0;
	_mini = 0;
}

MiniBaseView *BasescapeAccessibility::findMiniView(State *state) const
{
	if (!state) return 0;
	const std::vector<Surface*> &surfaces = state->getSurfaces();
	for (std::vector<Surface*>::const_iterator i = surfaces.begin(); i != surfaces.end(); ++i)
	{
		if (MiniBaseView *view = dynamic_cast<MiniBaseView*>(*i)) return view;
	}
	return 0;
}

BaseView *BasescapeAccessibility::findView(State *state) const
{
	if (!state) return 0;
	const std::vector<Surface*> &surfaces = state->getSurfaces();
	for (std::vector<Surface*>::const_iterator i = surfaces.begin(); i != surfaces.end(); ++i)
	{
		if (BaseView *view = dynamic_cast<BaseView*>(*i)) return view;
	}
	return 0;
}

bool BasescapeAccessibility::supports(State *state)
{
	if (!state) return false;
	const std::vector<Surface*> &surfaces = state->getSurfaces();
	for (std::vector<Surface*>::const_iterator i = surfaces.begin(); i != surfaces.end(); ++i)
	{
		if (dynamic_cast<BaseView*>(*i) || dynamic_cast<MiniBaseView*>(*i)) return true;
	}
	return false;
}

InteractiveSurface *BasescapeAccessibility::getPreferredControl(State *state) const
{
	if (BaseView *view = getView(state)) return view;
	return state == _state && _mini ? _mini : findMiniView(state);
}

BaseView *BasescapeAccessibility::getView(State *state) const
{
	return state == _state && _view ? _view : findView(state);
}

RuleBaseFacility *BasescapeAccessibility::getPlacementRule(State *state) const
{
	if (PlaceFacilityState *placement = dynamic_cast<PlaceFacilityState*>(state)) return placement->getFacilityRule();
	if (PlaceLiftState *lift = dynamic_cast<PlaceLiftState*>(state)) return lift->getFacilityRule();
	return 0;
}

bool BasescapeAccessibility::isPlacementState(State *state) const
{
	return getPlacementRule(state) != 0;
}

bool BasescapeAccessibility::isPlaceable(State *state) const
{
	BaseView *view = getView(state);
	RuleBaseFacility *rule = getPlacementRule(state);
	if (!view || !rule) return false;
	if (dynamic_cast<PlaceLiftState*>(state)) return view->isAreaAvailable(rule->getSize());
	return view->isPlaceable(rule);
}

std::string BasescapeAccessibility::describeCell(State *state) const
{
	BaseView *view = getView(state);
	if (!view) return std::string();

	std::ostringstream result;
	appendSentence(result, state->tr("STR_ACCESSIBILITY_BASE_CELL").arg(view->getGridY() + 1).arg(view->getGridX() + 1));
	BaseFacility *facility = view->getFacilityAt(view->getGridX(), view->getGridY());
	if (facility)
	{
		RuleBaseFacility *rule = facility->getRules();
		appendSentence(result, state->tr("STR_ACCESSIBILITY_BASE_FACILITY").arg(state->tr(rule->getType())));
		appendSentence(result, state->tr("STR_ACCESSIBILITY_BASE_FACILITY_SIZE").arg(rule->getSize()));
		if (facility->getBuildTime() == 0)
		{
			appendSentence(result, state->tr("STR_ACCESSIBILITY_BASE_OPERATIONAL"));
		}
		else if (facility->getBuildTime() == INT_MAX)
		{
			appendSentence(result, state->tr("STR_ACCESSIBILITY_BASE_QUEUED"));
		}
		else
		{
			appendSentence(result, state->tr("STR_ACCESSIBILITY_BASE_CONSTRUCTION").arg(facility->getBuildTime()));
		}
		if (!isPlacementState(state)) appendSentence(result, state->tr("STR_ACCESSIBILITY_BASE_ACTION_OPEN"));
	}
	else
	{
		appendSentence(result, state->tr("STR_ACCESSIBILITY_BASE_EMPTY"));
	}

	if (RuleBaseFacility *rule = getPlacementRule(state))
	{
		appendSentence(result, state->tr("STR_ACCESSIBILITY_BASE_PLACING").arg(state->tr(rule->getType())).arg(rule->getSize()));
		appendSentence(result, state->tr(isPlaceable(state) ? "STR_ACCESSIBILITY_BASE_PLACEABLE" : "STR_ACCESSIBILITY_BASE_NOT_PLACEABLE"));
		appendSentence(result, state->tr("STR_ACCESSIBILITY_BASE_ACTION_PLACE"));
	}
	return result.str();
}

std::string BasescapeAccessibility::describeBase(State *state) const
{
	MiniBaseView *mini = state == _state && _mini ? _mini : findMiniView(state);
	if (!mini || mini->getBaseCount() == 0) return state->tr("STR_ACCESSIBILITY_BASE_NO_BASES");
	Base *base = mini->getBaseAt(mini->getHoveredBase());
	if (!base) return state->tr("STR_ACCESSIBILITY_BASE_NO_BASES");
	return state->tr("STR_ACCESSIBILITY_BASE_POSITION")
		.arg(base->getName()).arg(mini->getHoveredBase() + 1).arg(mini->getBaseCount());
}

std::string BasescapeAccessibility::describeSummary(State *state) const
{
	std::ostringstream result;
	appendSentence(result, state->tr("STR_ACCESSIBILITY_BASESCAPE"));
	appendSentence(result, state->tr("STR_ACCESSIBILITY_BASE_HELP"));
	if (getView(state)) appendSentence(result, describeCell(state));
	else appendSentence(result, describeBase(state));
	return result.str();
}

std::string BasescapeAccessibility::getSummary(State *state) const
{
	return describeSummary(state);
}

std::vector<AccessibleNode> BasescapeAccessibility::getNodes(State *state) const
{
	std::vector<AccessibleNode> nodes;
	if (BaseView *view = getView(state))
	{
		AccessibleNode node;
		node.control = view;
		node.role = ACCESSIBLE_GRID;
		node.roleName = state ? state->tr("STR_ACCESSIBILITY_ROLE_GRID") : std::string();
		node.name = state ? state->tr("STR_ACCESSIBILITY_BASE_GRID") : std::string();
		node.value = describeCell(state);
		node.description = state ? state->tr("STR_ACCESSIBILITY_BASE_HELP") : std::string();
		nodes.push_back(node);
	}
	MiniBaseView *mini = state == _state && _mini ? _mini : findMiniView(state);
	if (mini && mini->getBaseCount() > 0)
	{
		AccessibleNode node;
		node.control = mini;
		node.role = ACCESSIBLE_LIST;
		node.roleName = state->tr("STR_ACCESSIBILITY_ROLE_LIST");
		node.name = state->tr("STR_ACCESSIBILITY_BASE_SELECTOR");
		node.value = describeBase(state);
		nodes.push_back(node);
	}
	return nodes;
}

void BasescapeAccessibility::activate(State *state, std::vector<std::string> &messages)
{
	_state = state;
	_view = findView(state);
	_mini = findMiniView(state);
	if (!_view && !_mini) return;
	messages.push_back(state->tr("STR_ACCESSIBILITY_BASESCAPE"));
	if (_view)
	{
		_view->accessibilitySetGridPosition(_view->getGridX(), _view->getGridY());
		messages.push_back(state->tr("STR_ACCESSIBILITY_BASE_HELP"));
		messages.push_back(describeCell(state));
	}
	else
	{
		if (_mini->getBaseCount() > 0) _mini->accessibilitySetHoveredBase(_mini->getSelectedBase());
		messages.push_back(state->tr("STR_ACCESSIBILITY_BASE_SELECTOR_HELP"));
		messages.push_back(describeBase(state));
	}
}

void BasescapeAccessibility::moveCursor(State *state, int dx, int dy, std::vector<std::string> &messages)
{
	BaseView *view = getView(state);
	if (!view) return;
	const int x = view->getGridX() + dx;
	const int y = view->getGridY() + dy;
	if (!view->accessibilitySetGridPosition(x, y))
	{
		messages.push_back(state->tr("STR_ACCESSIBILITY_BASE_EDGE"));
		return;
	}
	messages.push_back(describeCell(state));
}

void BasescapeAccessibility::activateCell(State *state)
{
	BaseView *view = getView(state);
	if (!view) return;
	SDL_Event event = {};
	event.type = SDL_MOUSEBUTTONUP;
	event.button.button = SDL_BUTTON_LEFT;
	Action action(&event, 1.0, 1.0, 0, 0);
	action.setSender(view);
	view->mouseClick(&action, state);
}

void BasescapeAccessibility::moveBase(State *state, int direction, std::vector<std::string> &messages)
{
	if (!_mini || _mini->getBaseCount() == 0) return;
	const size_t count = _mini->getBaseCount();
	const size_t current = _mini->getHoveredBase();
	const size_t next = direction < 0 ? (current + count - 1) % count : (current + 1) % count;
	_mini->accessibilitySetHoveredBase(next);
	messages.push_back(describeBase(state));
}

void BasescapeAccessibility::activateBase(State *state)
{
	if (!_mini || _mini->getBaseCount() == 0) return;
	SDL_Event event = {};
	event.type = SDL_MOUSEBUTTONUP;
	event.button.button = SDL_BUTTON_LEFT;
	Action action(&event, 1.0, 1.0, 0, 0);
	action.setSender(_mini);
	_mini->mouseClick(&action, state);
}

bool BasescapeAccessibility::handle(Action *action, State *state, InteractiveSurface *focused, std::vector<std::string> &messages)
{
	if (!action || !state || action->getDetails()->type != SDL_KEYDOWN) return false;
	if (_state != state || (!_view && !_mini))
	{
		_state = state;
		_view = findView(state);
		_mini = findMiniView(state);
	}
	if (focused == _mini && _mini)
	{
		const SDLKey key = action->getDetails()->key.keysym.sym;
		if (key == SDLK_LEFT || key == SDLK_UP) { moveBase(state, -1, messages); return true; }
		if (key == SDLK_RIGHT || key == SDLK_DOWN) { moveBase(state, 1, messages); return true; }
		if (key == SDLK_HOME)
		{
			if (_mini->getBaseCount() > 0) _mini->accessibilitySetHoveredBase(0);
			messages.push_back(describeBase(state));
			return true;
		}
		if (key == SDLK_END)
		{
			if (_mini->getBaseCount() > 0) _mini->accessibilitySetHoveredBase(_mini->getBaseCount() - 1);
			messages.push_back(describeBase(state));
			return true;
		}
		if (key == SDLK_RETURN || key == SDLK_KP_ENTER || key == SDLK_SPACE)
		{
			activateBase(state);
			return true;
		}
		return false;
	}
	if (!_view || focused != _view) return false;

	const SDLKey key = static_cast<SDLKey>(AccessibilityInput::getKey(action));
	const int modifiers = AccessibilityInput::getModifiers(action);
	if (AccessibilityInput::isCommandChord(modifiers) && key == SDLK_r)
	{
		messages.push_back(describeCell(state));
		return true;
	}
	if (AccessibilityInput::isCommandChord(modifiers) && key == SDLK_s)
	{
		messages.push_back(describeSummary(state));
		return true;
	}
	if (key == SDLK_LEFT) { moveCursor(state, -1, 0, messages); return true; }
	if (key == SDLK_RIGHT) { moveCursor(state, 1, 0, messages); return true; }
	if (key == SDLK_UP) { moveCursor(state, 0, -1, messages); return true; }
	if (key == SDLK_DOWN) { moveCursor(state, 0, 1, messages); return true; }
	if (key == SDLK_HOME)
	{
		_view->accessibilitySetGridPosition(0, 0);
		messages.push_back(describeCell(state));
		return true;
	}
	if (key == SDLK_RETURN || key == SDLK_KP_ENTER || key == SDLK_SPACE)
	{
		if (isPlacementState(state) && !isPlaceable(state))
		{
			messages.push_back(state->tr("STR_ACCESSIBILITY_BASE_NOT_PLACEABLE"));
			return true;
		}
		activateCell(state);
		return true;
	}
	return false;
}

}
