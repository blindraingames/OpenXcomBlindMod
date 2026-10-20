# Accessibility state coverage

[Русская версия](accessibility-state-matrix_ru.md)

This matrix records how Blind XCom exposes OpenXcom states. It is organized by interaction model rather than by individual class so newly added ordinary dialogs inherit coverage automatically.

| Area | Coverage route | Keyboard result | Verification |
|---|---|---|---|
| Main menu, new game, loading, options, confirmations, Ufopaedia | Generic state adapter | Focus order, activation, toggles, lists, edits, combo boxes, sliders, arrow buttons, tooltip-labelled icon buttons | Startup smoke tests; localized control roles |
| Geoscape | Geoscape adapter plus generic dialogs | Known-target browsing, geographic cursor, time speed, base placement, destination selection | Hidden targets excluded by adapter filtering |
| Basescape | Base-grid and base-selector adapters plus generic dialogs | Six-by-six grid, facility status and construction, placement validation, founded-base selection | Normal game callbacks perform open/place/select actions |
| Battlescape map | Battlescape adapter | Logical cursor, path cost, movement, all firearm modes, throwing, melee, psi, guided waypoints, visible enemies, all-door navigation, stance, level movement, TU reserve, reload, inventory, abort and end turn | Undiscovered tiles and non-visible enemies are filtered before speech, except that the explicit all-door command reveals door coordinates as requested |
| Item action and grenade timer dialogs | Generic adapter with tactical popup metadata | Action name, accuracy and TU cost; timer selection and activation | Normal action-menu and grenade callbacks are used |
| Tactical inventory | Inventory adapter plus tooltip-labelled buttons | Item browsing, cyclic placement through every compatible inventory section, load/unload, pre-battle priming, unit switching and exit | Inventory rules, TU costs and slot-fit validation remain authoritative |
| Medikit | Generic adapter plus accessible body-part list | Body-part selection, wound count, heal, stimulant, pain killer and exit | Normal medikit callbacks spend TU and quantities |
| Mission abort and results | Generic state adapter | Summary text and confirm/cancel controls | Existing mission rules and confirmation remain authoritative |

## Cross-cutting checks

- Every `STR_ACCESSIBILITY_*` key used by C++ exists in common `en-US`, `en-GB`, and `ru`, so both UFO and TFTD masters and content mods inherit the accessibility vocabulary.
- Gameplay and UI code do not call Tolk. Only `TolkBridge` resolves and invokes its symbols.
- Both x86 and x64 Tolk/NVDA controller runtime sets are checked by `scripts/Test-Accessibility.ps1`.
- Accessibility-off and forced-accessibility startup smoke tests run against isolated user/config folders.
- Generic list navigation uses the selected row, all visible columns, localized position, and page-sized movement.

Run the automated audit from a Visual Studio developer shell:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/Test-Accessibility.ps1 -Smoke
```

Manual NVDA verification remains required for live speech timing, tactical actions in a real save, and mod-specific custom `InteractiveSurface` subclasses. Such subclasses are deliberately not guessed: they need an adapter or a localized tooltip before entering the accessibility focus order.
