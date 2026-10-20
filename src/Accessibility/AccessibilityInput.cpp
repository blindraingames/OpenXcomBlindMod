#include "AccessibilityInput.h"
#include "../Engine/Action.h"
#include <SDL.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace OpenXcom
{

namespace
{
	bool isPressed(const Uint8 *keys, int count, SDLKey key)
	{
		return keys && static_cast<int>(key) >= 0 && static_cast<int>(key) < count && keys[key] != 0;
	}
}

int AccessibilityInput::getKey(Action *action)
{
	if (!action || (action->getDetails()->type != SDL_KEYDOWN && action->getDetails()->type != SDL_KEYUP)) return SDLK_UNKNOWN;
	const SDLKey key = action->getDetails()->key.keysym.sym;
#ifdef _WIN32
	// SDL 1.2 can report letter symbols according to the active keyboard layout.
	// Accessibility shortcuts are physical commands and must also work with
	// Russian and other non-Latin layouts, so normalize alphanumeric keys from
	// their hardware scan code on Windows.
	const UINT virtualKey = MapVirtualKey(action->getDetails()->key.keysym.scancode, MAPVK_VSC_TO_VK);
	if (virtualKey >= 'A' && virtualKey <= 'Z') return SDLK_a + (virtualKey - 'A');
	if (virtualKey >= '0' && virtualKey <= '9') return SDLK_0 + (virtualKey - '0');
#endif
	return key;
}

int AccessibilityInput::getModifiers(Action *action)
{
	int modifiers = static_cast<int>(SDL_GetModState());
	if (action && (action->getDetails()->type == SDL_KEYDOWN || action->getDetails()->type == SDL_KEYUP))
	{
		modifiers |= static_cast<int>(action->getDetails()->key.keysym.mod);
	}

	int keyCount = 0;
	const Uint8 *keys = SDL_GetKeyState(&keyCount);
	if (isPressed(keys, keyCount, SDLK_LSHIFT)) modifiers |= KMOD_LSHIFT;
	if (isPressed(keys, keyCount, SDLK_RSHIFT)) modifiers |= KMOD_RSHIFT;
	if (isPressed(keys, keyCount, SDLK_LCTRL)) modifiers |= KMOD_LCTRL;
	if (isPressed(keys, keyCount, SDLK_RCTRL)) modifiers |= KMOD_RCTRL;
	if (isPressed(keys, keyCount, SDLK_LALT)) modifiers |= KMOD_LALT;
	if (isPressed(keys, keyCount, SDLK_RALT)) modifiers |= KMOD_RALT;

#ifdef _WIN32
	// keysym.mod is occasionally incomplete in SDL 1.2 on Windows.  Native key
	// state is used as the final fallback so Ctrl+Alt commands are not lost.
	if ((GetAsyncKeyState(VK_LSHIFT) & 0x8000) != 0) modifiers |= KMOD_LSHIFT;
	if ((GetAsyncKeyState(VK_RSHIFT) & 0x8000) != 0) modifiers |= KMOD_RSHIFT;
	if ((GetAsyncKeyState(VK_LCONTROL) & 0x8000) != 0) modifiers |= KMOD_LCTRL;
	if ((GetAsyncKeyState(VK_RCONTROL) & 0x8000) != 0) modifiers |= KMOD_RCTRL;
	if ((GetAsyncKeyState(VK_LMENU) & 0x8000) != 0) modifiers |= KMOD_LALT;
	if ((GetAsyncKeyState(VK_RMENU) & 0x8000) != 0) modifiers |= KMOD_RALT;
#endif

	return modifiers;
}

bool AccessibilityInput::isCommandChord(int modifiers)
{
	return (modifiers & KMOD_CTRL) != 0 && (modifiers & KMOD_ALT) != 0;
}

bool AccessibilityInput::isForwardFocusKey(int key, int modifiers)
{
	return key == SDLK_TAB && (modifiers & KMOD_SHIFT) == 0;
}

bool AccessibilityInput::isBackwardFocusKey(int key, int modifiers)
{
	return key == SDLK_TAB && (modifiers & KMOD_SHIFT) != 0;
}

}
