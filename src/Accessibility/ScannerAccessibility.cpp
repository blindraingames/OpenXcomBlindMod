#include "ScannerAccessibility.h"
#include "AccessibilityInput.h"
#include "../Battlescape/ScannerState.h"
#include "../Battlescape/ScannerView.h"
#include "../Engine/Action.h"
#include <SDL.h>
#include <algorithm>
#include <cstdlib>
#include <sstream>

namespace OpenXcom
{

namespace
{
	std::string tileCount(ScannerState *state, int count)
	{
		return state->tr("STR_ACCESSIBILITY_SCANNER_TILE_COUNT", count);
	}

	std::string levelCount(ScannerState *state, int count)
	{
		return state->tr("STR_ACCESSIBILITY_SCANNER_LEVEL_COUNT", count);
	}
}

ScannerAccessibility::ScannerAccessibility() : _state(0), _index(0), _initialized(false)
{
}

void ScannerAccessibility::reset()
{
	_state = 0;
	_index = 0;
	_initialized = false;
}

bool ScannerAccessibility::supports(State *state)
{
	return dynamic_cast<ScannerState*>(state) != 0;
}

void ScannerAccessibility::normalize(ScannerState *state)
{
	const std::size_t count = state && state->getScannerView() ? state->getScannerView()->getBlips().size() : 0;
	if (count == 0) _index = 0;
	else if (_index >= count) _index = count - 1;
}

std::string ScannerAccessibility::describe(ScannerState *state) const
{
	if (!state || !state->getScannerView()) return std::string();
	const std::vector<MotionScannerBlip> blips = state->getScannerView()->getBlips();
	if (blips.empty()) return state->tr("STR_ACCESSIBILITY_SCANNER_NO_MOVEMENT");

	const std::size_t index = std::min(_index, blips.size() - 1);
	const MotionScannerBlip &blip = blips[index];
	std::ostringstream result;
	if (blip.x == 0 && blip.y == 0)
	{
		result << state->tr("STR_ACCESSIBILITY_SCANNER_SAME_POSITION");
	}
	else if (blip.x == 0)
	{
		result << state->tr("STR_ACCESSIBILITY_SCANNER_ONE_AXIS")
			.arg(tileCount(state, std::abs(blip.y)))
			.arg(state->tr(blip.y < 0 ? "STR_ACCESSIBILITY_SCANNER_HIGHER" : "STR_ACCESSIBILITY_SCANNER_LOWER"));
	}
	else if (blip.y == 0)
	{
		result << state->tr("STR_ACCESSIBILITY_SCANNER_ONE_AXIS")
			.arg(tileCount(state, std::abs(blip.x)))
			.arg(state->tr(blip.x < 0 ? "STR_ACCESSIBILITY_SCANNER_LEFT" : "STR_ACCESSIBILITY_SCANNER_RIGHT"));
	}
	else
	{
		result << state->tr("STR_ACCESSIBILITY_SCANNER_TWO_AXES")
			.arg(tileCount(state, std::abs(blip.x)))
			.arg(state->tr(blip.x < 0 ? "STR_ACCESSIBILITY_SCANNER_LEFT" : "STR_ACCESSIBILITY_SCANNER_RIGHT"))
			.arg(tileCount(state, std::abs(blip.y)))
			.arg(state->tr(blip.y < 0 ? "STR_ACCESSIBILITY_SCANNER_HIGHER" : "STR_ACCESSIBILITY_SCANNER_LOWER"));
	}
	if (blip.z != 0)
	{
		result << " " << state->tr("STR_ACCESSIBILITY_SCANNER_LEVEL_OFFSET")
			.arg(levelCount(state, std::abs(blip.z)))
			.arg(state->tr(blip.z > 0 ? "STR_ACCESSIBILITY_SCANNER_ABOVE" : "STR_ACCESSIBILITY_SCANNER_BELOW"));
	}
	const char *sizeId = blip.size < 2 ? "STR_ACCESSIBILITY_SCANNER_SMALL"
		: (blip.size < 4 ? "STR_ACCESSIBILITY_SCANNER_MEDIUM" : "STR_ACCESSIBILITY_SCANNER_LARGE");
	result << " " << state->tr(sizeId);
	result << " " << state->tr("STR_ACCESSIBILITY_POSITION_OF").arg(index + 1).arg(blips.size());
	return result.str();
}

InteractiveSurface *ScannerAccessibility::getPreferredControl(ScannerState *state) const
{
	return state ? state->getScannerView() : 0;
}

std::vector<AccessibleNode> ScannerAccessibility::getNodes(ScannerState *state)
{
	std::vector<AccessibleNode> result;
	if (!state || !state->getScannerView()) return result;
	normalize(state);
	AccessibleNode node;
	node.control = state->getScannerView();
	node.role = ACCESSIBLE_LIST;
	node.roleName = state->tr("STR_ACCESSIBILITY_ROLE_LIST");
	node.name = state->tr("STR_ACCESSIBILITY_SCANNER_DETECTIONS");
	node.value = describe(state);
	result.push_back(node);
	return result;
}

std::string ScannerAccessibility::getSummary(ScannerState *state) const
{
	const std::size_t count = state && state->getScannerView() ? state->getScannerView()->getBlips().size() : 0;
	return state ? std::string(state->tr("STR_ACCESSIBILITY_SCANNER_SUMMARY").arg(count)) : std::string();
}

void ScannerAccessibility::activate(ScannerState *state, std::vector<std::string> &messages)
{
	if (!state || (_initialized && _state == state)) return;
	_state = state;
	_index = 0;
	_initialized = true;
	normalize(state);
	messages.push_back(getSummary(state));
	messages.push_back(describe(state));
}

void ScannerAccessibility::move(ScannerState *state, int delta, std::vector<std::string> &messages)
{
	const std::vector<MotionScannerBlip> blips = state->getScannerView()->getBlips();
	if (blips.empty())
	{
		messages.push_back(state->tr("STR_ACCESSIBILITY_SCANNER_NO_MOVEMENT"));
		return;
	}
	const int count = static_cast<int>(blips.size());
	_index = static_cast<std::size_t>((static_cast<int>(_index) + delta + count) % count);
	messages.push_back(describe(state));
}

bool ScannerAccessibility::handle(Action *action, ScannerState *state, InteractiveSurface *focused, std::vector<std::string> &messages)
{
	if (!action || !state || focused != state->getScannerView() || action->getDetails()->type != SDL_KEYDOWN) return false;
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
		const std::vector<MotionScannerBlip> blips = state->getScannerView()->getBlips();
		if (!blips.empty()) _index = key == SDLK_HOME ? 0 : blips.size() - 1;
		messages.push_back(describe(state));
		return true;
	}
	return false;
}

}
