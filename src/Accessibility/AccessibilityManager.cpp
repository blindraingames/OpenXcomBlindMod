#include "AccessibilityManager.h"
#include "AccessibilityInput.h"
#include "AccessibilityText.h"
#include "../Basescape/BaseView.h"
#include "../Battlescape/BattlescapeState.h"
#include "../Battlescape/InventoryState.h"
#include "../Battlescape/MedikitView.h"
#include "../Battlescape/ScannerState.h"
#include "../Battlescape/UnitInfoState.h"
#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/InteractiveSurface.h"
#include "../Engine/Logger.h"
#include "../Engine/Options.h"
#include "../Engine/State.h"
#include "../Interface/TextEdit.h"
#include "../Interface/TextButton.h"
#include "../Interface/ToggleTextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/ComboBox.h"
#include "../Interface/Slider.h"
#include "../Interface/ArrowButton.h"
#include <SDL.h>

namespace OpenXcom
{

namespace
{
	const size_t HISTORY_LIMIT = 25;
	const char *BRAND_MESSAGE = u8"Blind XCom v1.0a \u0440\u0430\u0437\u0440\u0430\u0431\u043e\u0442\u0430\u043d\u043e \u0441\u0442\u0443\u0434\u0438\u0435\u0439 \"\u0421\u043b\u0435\u043f\u043e\u0439 \u0414\u043e\u0436\u0434\u044c \u043d\u0430 \u0431\u0430\u0437\u0435 Open XCom\".";
}

AccessibilityManager::AccessibilityManager() : _basescape(0), _battlescape(0), _geoscape(0), _inventory(0), _scanner(0), _unitInfo(0), _state(0), _editing(0), _enabled(false), _brandAnnounced(false), _screenAnnounced(false)
{
	Log(LOG_INFO) << "Accessibility: requested mode " << static_cast<int>(Options::accessibilityMode) << ".";
	if (Options::accessibilityMode == ACCESSIBILITY_OFF)
	{
		Log(LOG_INFO) << "Accessibility: disabled by configuration.";
		return;
	}
	const bool bridgeReady = _bridge.initialize();
	if (!bridgeReady)
	{
		Log(LOG_WARNING) << "Accessibility: speech output initialization failed.";
	}
	_enabled = Options::accessibilityMode == ACCESSIBILITY_ON || (bridgeReady && _bridge.hasScreenReader());
	Log(LOG_INFO) << "Accessibility: " << (_enabled ? "enabled" : "not enabled because no screen reader was detected") << ".";
}

AccessibilityManager::~AccessibilityManager()
{
	delete _basescape;
	delete _battlescape;
	delete _geoscape;
	delete _inventory;
	delete _scanner;
	delete _unitInfo;
	_bridge.shutdown();
}

bool AccessibilityManager::isEnabled() const
{
	return _enabled;
}

void AccessibilityManager::remember(const std::string &text)
{
	_lastMessage = text;
	_history.push_back(text);
	while (_history.size() > HISTORY_LIMIT) _history.pop_front();
}

void AccessibilityManager::announce(const std::string &text, bool interrupt)
{
	if (!_enabled) return;
	const std::string cleaned = AccessibilityText::clean(text);
	if (cleaned.empty()) return;
	remember(cleaned);
	const bool accepted = _bridge.output(cleaned, interrupt);
	if (accepted)
	{
		Log(LOG_INFO) << "Accessibility: announcement accepted by Tolk.";
	}
	else
	{
		Log(LOG_WARNING) << "Accessibility: announcement could not be sent to Tolk.";
	}
}

void AccessibilityManager::silence()
{
	if (_enabled) _bridge.silence();
}

void AccessibilityManager::rebuildNodes(InteractiveSurface *preferred)
{
	std::vector<AccessibleNode> nodes = _adapter.getNodes(_state);
	if (_state && BasescapeAccessibility::supports(_state))
	{
		if (!_basescape) _basescape = new BasescapeAccessibility();
		std::vector<AccessibleNode> baseNodes = _basescape->getNodes(_state);
		nodes.insert(nodes.begin(), baseNodes.begin(), baseNodes.end());
	}
	if (_state && InventoryAccessibility::supports(_state))
	{
		if (!_inventory) _inventory = new InventoryAccessibility();
		std::vector<AccessibleNode> inventoryNodes = _inventory->getNodes(dynamic_cast<InventoryState*>(_state));
		nodes.insert(nodes.begin(), inventoryNodes.begin(), inventoryNodes.end());
	}
	if (_state && ScannerAccessibility::supports(_state))
	{
		if (!_scanner) _scanner = new ScannerAccessibility();
		std::vector<AccessibleNode> scannerNodes = _scanner->getNodes(dynamic_cast<ScannerState*>(_state));
		nodes.insert(nodes.begin(), scannerNodes.begin(), scannerNodes.end());
	}
	if (_state && UnitInfoAccessibility::supports(_state))
	{
		if (!_unitInfo) _unitInfo = new UnitInfoAccessibility();
		nodes = _unitInfo->getNodes(dynamic_cast<UnitInfoState*>(_state));
	}
	_focus.setNodes(nodes, preferred);
}

void AccessibilityManager::announceNode(const AccessibleNode *node, bool interrupt)
{
	if (node) announce(AccessibilityText::describe(*node), interrupt);
}

void AccessibilityManager::onStateActivated(State *state)
{
	if (!_enabled || !state) return;
	if (_editing) _editing->setFocus(false);
	_editing = 0;
	_state = state;
	if (_basescape) _basescape->reset();
	if (_battlescape) _battlescape->reset();
	if (_geoscape) _geoscape->reset();
	if (_inventory) _inventory->reset();
	if (_scanner) _scanner->reset();
	if (_unitInfo) _unitInfo->reset();
	rebuildNodes();
	_screenSummary = _adapter.getSummary(state);
	_screenAnnounced = false;
	Log(LOG_INFO) << "Accessibility: state activated with " << _focus.size() << " focusable control(s).";
}

void AccessibilityManager::update(State *state)
{
	if (!_enabled || !state) return;
	if (_state != state) onStateActivated(state);
	if (InventoryState *inventory = dynamic_cast<InventoryState*>(state))
	{
		if (!_inventory) _inventory = new InventoryAccessibility();
		if (_screenAnnounced) return;
		std::vector<std::string> messages;
		_inventory->activate(inventory, messages);
		rebuildNodes(_inventory->getPreferredControl(inventory));
		_screenSummary = _inventory->getSummary(inventory);
		bool interrupt = true;
		for (std::vector<std::string>::const_iterator i = messages.begin(); i != messages.end(); ++i)
		{
			announce(*i, interrupt);
			interrupt = false;
		}
		_screenAnnounced = true;
		return;
	}
	if (ScannerState *scanner = dynamic_cast<ScannerState*>(state))
	{
		if (!_scanner) _scanner = new ScannerAccessibility();
		if (_screenAnnounced) return;
		std::vector<std::string> messages;
		_scanner->activate(scanner, messages);
		rebuildNodes(_scanner->getPreferredControl(scanner));
		_screenSummary = _scanner->getSummary(scanner);
		bool interrupt = true;
		for (std::vector<std::string>::const_iterator i = messages.begin(); i != messages.end(); ++i)
		{
			announce(*i, interrupt);
			interrupt = false;
		}
		_screenAnnounced = true;
		return;
	}
	if (UnitInfoState *unitInfo = dynamic_cast<UnitInfoState*>(state))
	{
		if (!_unitInfo) _unitInfo = new UnitInfoAccessibility();
		if (_screenAnnounced) return;
		std::vector<std::string> messages;
		_unitInfo->activate(unitInfo, messages);
		rebuildNodes(_unitInfo->getPreferredControl(unitInfo));
		_screenSummary = _unitInfo->getSummary(unitInfo);
		bool interrupt = true;
		for (std::vector<std::string>::const_iterator i = messages.begin(); i != messages.end(); ++i)
		{
			announce(*i, interrupt);
			interrupt = false;
		}
		_screenAnnounced = true;
		return;
	}
	if (BasescapeAccessibility::supports(state))
	{
		if (!_basescape) _basescape = new BasescapeAccessibility();
		if (_screenAnnounced) return;
		std::vector<std::string> messages;
		_basescape->activate(state, messages);
		rebuildNodes(_basescape->getPreferredControl(state));
		_screenSummary = _basescape->getSummary(state);
		bool interrupt = true;
		if (!_brandAnnounced)
		{
			announce(BRAND_MESSAGE, true);
			_brandAnnounced = true;
			interrupt = false;
		}
		for (std::vector<std::string>::const_iterator i = messages.begin(); i != messages.end(); ++i)
		{
			announce(*i, interrupt);
			interrupt = false;
		}
		_screenAnnounced = true;
		return;
	}
	if (GeoscapeAccessibility::supports(state))
	{
		if (!_geoscape) _geoscape = new GeoscapeAccessibility();
		std::vector<std::string> messages;
		if (!_screenAnnounced)
		{
			_geoscape->activate(state, messages);
			if (!_brandAnnounced)
			{
				announce(BRAND_MESSAGE, true);
				_brandAnnounced = true;
			}
		}
		else
		{
			_geoscape->update(state, messages);
		}
		bool interrupt = true;
		for (std::vector<std::string>::const_iterator i = messages.begin(); i != messages.end(); ++i)
		{
			announce(*i, interrupt);
			interrupt = false;
		}
		_screenAnnounced = true;
		return;
	}
	if (BattlescapeState *battle = dynamic_cast<BattlescapeState*>(state))
	{
		if (!_battlescape) _battlescape = new BattlescapeAccessibility();
		std::vector<std::string> messages;
		if (_screenAnnounced) _battlescape->update(battle, messages);
		else _battlescape->activate(battle, messages);
		bool interrupt = true;
		if (!_brandAnnounced)
		{
			announce(BRAND_MESSAGE, true);
			_brandAnnounced = true;
			interrupt = false;
		}
		for (std::vector<std::string>::const_iterator i = messages.begin(); i != messages.end(); ++i)
		{
			announce(*i, interrupt);
			interrupt = false;
		}
		_screenAnnounced = true;
		return;
	}
	if (_screenAnnounced) return;

	InteractiveSurface *preferred = _focus.current() ? _focus.current()->control : 0;
	rebuildNodes(preferred);
	_screenSummary = _adapter.getSummary(state);
	const std::string title = _adapter.getTitle(state);
	if (_focus.empty())
	{
		if (!title.empty())
		{
			announce(title, true);
			_screenAnnounced = true;
		}
		return;
	}

	Log(LOG_INFO) << "Accessibility: screen ready with " << _focus.size() << " focusable control(s).";
	if (!_brandAnnounced)
	{
		announce(BRAND_MESSAGE, true);
		_brandAnnounced = true;
		if (!title.empty()) announce(title, false);
	}
	else if (!title.empty())
	{
		announce(title, true);
	}
	announceNode(_focus.current(), false);
	_screenAnnounced = true;
}

void AccessibilityManager::activateCurrent(Action *)
{
	AccessibleNode *node = _focus.current();
	if (!node || !node->control || !_state) return;
	InteractiveSurface *control = node->control;
	if (TextEdit *edit = dynamic_cast<TextEdit*>(control))
	{
		_editing = edit;
		_editing->setFocus(true);
		announce(AccessibilityText::describe(*node) + ", " + std::string(_state->tr("STR_ACCESSIBILITY_EDITING")), true);
		return;
	}
	if (ComboBox *combo = dynamic_cast<ComboBox*>(control))
	{
		SDL_Event event = {};
		event.type = SDL_MOUSEBUTTONUP;
		event.button.button = SDL_BUTTON_LEFT;
		Action action(&event, 1.0, 1.0, 0, 0);
		action.setSender(combo);
		combo->accessibilityMoveSelection(1, &action, _state);
		rebuildNodes(combo);
		announceNode(_focus.current());
		return;
	}
	if (dynamic_cast<MedikitView*>(control))
	{
		announceNode(node);
		return;
	}

	SDL_Event event = {};
	event.type = SDL_MOUSEBUTTONUP;
	event.button.button = SDL_BUTTON_LEFT;
	Action action(&event, 1.0, 1.0, 0, 0);
	action.setSender(control);
	State *activatedState = _state;
	Game *game = activatedState->getGame();
	control->mousePress(&action, _state);
	if (!game->isState(activatedState)) return;
	control->mouseRelease(&action, _state);
	if (!game->isState(activatedState)) return;
	control->mouseClick(&action, _state);
	if (!game->isState(activatedState)) return;
	rebuildNodes(control);
	announceNode(_focus.current());
}

void AccessibilityManager::adjustCurrent(int direction, Action *sourceAction)
{
	AccessibleNode *node = _focus.current();
	if (!node || !node->control || !_state) return;
	InteractiveSurface *control = node->control;
	bool changed = false;
	if (ComboBox *combo = dynamic_cast<ComboBox*>(control))
	{
		changed = combo->accessibilityMoveSelection(direction, sourceAction, _state);
	}
	else if (Slider *slider = dynamic_cast<Slider*>(control))
	{
		changed = slider->accessibilityAdjust(direction, sourceAction, _state);
	}
	else if (TextList *list = dynamic_cast<TextList*>(control))
	{
		changed = list->accessibilityMoveSelection(direction);
	}
	else if (MedikitView *medikit = dynamic_cast<MedikitView*>(control))
	{
		changed = medikit->accessibilityMoveSelection(direction);
	}
	if (changed)
	{
		rebuildNodes(control);
		announceNode(_focus.current());
	}
}

bool AccessibilityManager::handle(Action *action, State *state)
{
	if (!_enabled || !action || !state || action->getDetails()->type != SDL_KEYDOWN) return false;
	if (_state != state) onStateActivated(state);
	const SDLKey key = static_cast<SDLKey>(AccessibilityInput::getKey(action));
	const int modifiers = AccessibilityInput::getModifiers(action);
	if (key == SDLK_LCTRL || key == SDLK_RCTRL || key == SDLK_LALT || key == SDLK_RALT ||
		key == SDLK_LSHIFT || key == SDLK_RSHIFT || (modifiers & (KMOD_CTRL | KMOD_ALT)) != 0)
	{
		Log(LOG_INFO) << "Accessibility input: key=" << static_cast<int>(key)
			<< ", raw key=" << static_cast<int>(action->getDetails()->key.keysym.sym)
			<< ", scancode=" << static_cast<int>(action->getDetails()->key.keysym.scancode)
			<< ", event modifiers=" << static_cast<int>(action->getDetails()->key.keysym.mod)
			<< ", effective modifiers=" << modifiers << ".";
	}
	if (InventoryState *inventory = dynamic_cast<InventoryState*>(state))
	{
		if (!_inventory) _inventory = new InventoryAccessibility();
		std::vector<std::string> messages;
		InteractiveSurface *focused = _focus.current() ? _focus.current()->control : 0;
		if (_inventory->handle(action, inventory, focused, messages))
		{
			rebuildNodes(_inventory->getPreferredControl(inventory));
			bool interrupt = true;
			for (std::vector<std::string>::const_iterator i = messages.begin(); i != messages.end(); ++i)
			{
				announce(*i, interrupt);
				interrupt = false;
			}
			return true;
		}
	}
	if (ScannerState *scanner = dynamic_cast<ScannerState*>(state))
	{
		if (!_scanner) _scanner = new ScannerAccessibility();
		std::vector<std::string> messages;
		InteractiveSurface *focused = _focus.current() ? _focus.current()->control : 0;
		if (_scanner->handle(action, scanner, focused, messages))
		{
			rebuildNodes(_scanner->getPreferredControl(scanner));
			bool interrupt = true;
			for (std::vector<std::string>::const_iterator i = messages.begin(); i != messages.end(); ++i)
			{
				announce(*i, interrupt);
				interrupt = false;
			}
			return true;
		}
	}
	if (UnitInfoState *unitInfo = dynamic_cast<UnitInfoState*>(state))
	{
		if (!_unitInfo) _unitInfo = new UnitInfoAccessibility();
		std::vector<std::string> messages;
		InteractiveSurface *focused = _focus.current() ? _focus.current()->control : 0;
		if (_unitInfo->handle(action, unitInfo, focused, messages))
		{
			rebuildNodes(_unitInfo->getPreferredControl(unitInfo));
			bool interrupt = true;
			for (std::vector<std::string>::const_iterator i = messages.begin(); i != messages.end(); ++i)
			{
				announce(*i, interrupt);
				interrupt = false;
			}
			return true;
		}
	}
	if (BasescapeAccessibility::supports(state))
	{
		if (!_basescape) _basescape = new BasescapeAccessibility();
		std::vector<std::string> messages;
		InteractiveSurface *focused = _focus.current() ? _focus.current()->control : 0;
		if (_basescape->handle(action, state, focused, messages))
		{
			rebuildNodes(_basescape->getPreferredControl(state));
			bool interrupt = true;
			for (std::vector<std::string>::const_iterator i = messages.begin(); i != messages.end(); ++i)
			{
				announce(*i, interrupt);
				interrupt = false;
			}
			return true;
		}
	}
	if (GeoscapeAccessibility::supports(state))
	{
		if (!_geoscape) _geoscape = new GeoscapeAccessibility();
		std::vector<std::string> messages;
		if (_geoscape->handle(action, state, messages))
		{
			bool interrupt = true;
			for (std::vector<std::string>::const_iterator i = messages.begin(); i != messages.end(); ++i)
			{
				announce(*i, interrupt);
				interrupt = false;
			}
			return true;
		}
	}
	if (BattlescapeState *battle = dynamic_cast<BattlescapeState*>(state))
	{
		if (!_battlescape) _battlescape = new BattlescapeAccessibility();
		std::vector<std::string> messages;
		if (_battlescape->handle(action, battle, messages))
		{
			bool interrupt = true;
			for (std::vector<std::string>::const_iterator i = messages.begin(); i != messages.end(); ++i)
			{
				announce(*i, interrupt);
				interrupt = false;
			}
			return true;
		}
	}
	InteractiveSurface *preferred = _focus.current() ? _focus.current()->control : 0;
	rebuildNodes(preferred);
	if (key == SDLK_TAB || key == SDLK_LEFT || key == SDLK_RIGHT || key == SDLK_UP || key == SDLK_DOWN || key == SDLK_RETURN || key == SDLK_KP_ENTER || key == SDLK_SPACE)
	{
		Log(LOG_INFO) << "Accessibility: received navigation key " << static_cast<int>(key) << "; controls: " << _focus.size() << ".";
	}

	if (_editing)
	{
		if (key == SDLK_TAB || key == SDLK_ESCAPE || key == SDLK_RETURN || key == SDLK_KP_ENTER)
		{
			InteractiveSurface *edited = _editing;
			_editing->setFocus(false);
			_editing = 0;
			rebuildNodes(edited);
			if (key == SDLK_TAB)
			{
				announceNode((modifiers & KMOD_SHIFT) ? _focus.previous() : _focus.next());
			}
			else
			{
				announceNode(_focus.current());
			}
			return true;
		}
		return false;
	}

	if (AccessibilityInput::isCommandChord(modifiers) && key == SDLK_r)
	{
		announceNode(_focus.current());
		return true;
	}
	if (AccessibilityInput::isCommandChord(modifiers) && key == SDLK_s)
	{
		announce(_screenSummary, true);
		return true;
	}
	if (AccessibilityInput::isForwardFocusKey(key, modifiers))
	{
		announceNode(_focus.next());
		return true;
	}
	if (AccessibilityInput::isBackwardFocusKey(key, modifiers))
	{
		announceNode(_focus.previous());
		return true;
	}
	if (key == SDLK_HOME)
	{
		announceNode(_focus.first());
		return true;
	}
	if (key == SDLK_END)
	{
		announceNode(_focus.last());
		return true;
	}
	if (key == Options::keyOk || key == SDLK_RETURN || key == SDLK_KP_ENTER || key == SDLK_SPACE)
	{
		activateCurrent(action);
		return true;
	}
	if (key == SDLK_PAGEUP || key == SDLK_PAGEDOWN)
	{
		AccessibleNode *current = _focus.current();
		TextList *list = current && current->control ? dynamic_cast<TextList*>(current->control) : 0;
		if (list)
		{
			const size_t steps = list->getVisibleRows() > 1 ? list->getVisibleRows() - 1 : 1;
			bool changed = false;
			for (size_t i = 0; i < steps; ++i)
			{
				changed = list->accessibilityMoveSelection(key == SDLK_PAGEUP ? -1 : 1) || changed;
			}
			if (changed)
			{
				rebuildNodes(list);
				announceNode(_focus.current());
			}
			return true;
		}
	}
	if (key == SDLK_LEFT || key == SDLK_UP)
	{
		AccessibleNode *current = _focus.current();
		TextList *list = current && current->control ? dynamic_cast<TextList*>(current->control) : 0;
		if (key == SDLK_LEFT && list && list->hasAccessibilityAdjustment())
		{
			State *adjustedState = _state;
			Game *game = adjustedState->getGame();
			list->accessibilityAdjust(-1, action, adjustedState);
			if (game->isState(adjustedState))
			{
				rebuildNodes(list);
				announceNode(_focus.current());
			}
			return true;
		}
		if (current && (current->role == ACCESSIBLE_COMBO_BOX || current->role == ACCESSIBLE_SLIDER || current->role == ACCESSIBLE_LIST))
		{
			adjustCurrent(-1, action);
		}
		else
		{
			announceNode(_focus.previous());
		}
		return true;
	}
	if (key == SDLK_RIGHT || key == SDLK_DOWN)
	{
		AccessibleNode *current = _focus.current();
		TextList *list = current && current->control ? dynamic_cast<TextList*>(current->control) : 0;
		if (key == SDLK_RIGHT && list && list->hasAccessibilityAdjustment())
		{
			State *adjustedState = _state;
			Game *game = adjustedState->getGame();
			list->accessibilityAdjust(1, action, adjustedState);
			if (game->isState(adjustedState))
			{
				rebuildNodes(list);
				announceNode(_focus.current());
			}
			return true;
		}
		if (current && (current->role == ACCESSIBLE_COMBO_BOX || current->role == ACCESSIBLE_SLIDER || current->role == ACCESSIBLE_LIST))
		{
			adjustCurrent(1, action);
		}
		else
		{
			announceNode(_focus.next());
		}
		return true;
	}
	return false;
}

}
