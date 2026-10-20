# Blind XCom accessibility

[Русская версия](accessibility_ru.md)

The Windows build loads Tolk dynamically. If Tolk or a supported screen reader is unavailable, OpenXcom continues normally without accessibility output.

## Starting the game

- Automatic mode is the default and enables accessibility when Tolk detects a running screen reader.
- `--accessibility` forces accessibility output on.
- `--no-accessibility` disables accessibility output.
- The persisted `accessibilityMode` option uses `0` for automatic, `1` for on, and `2` for off.

The x64 build must keep `Tolk.dll` and `nvdaControllerClient64.dll` beside `openxcom.exe`. `SAAPI64.dll` provides the System Access backend. Third-party licenses are stored in `deps/tolk`.

## Keyboard navigation

- `Tab` / `Shift+Tab`: next / previous accessible control.
- Arrow keys: move through controls; on lists, combo boxes and sliders they change the current value.
- `PageUp` / `PageDown`: move by a visible page in lists.
- `Home` / `End`: first / last control.
- `Enter` / `Space`: activate the current control.
- `Escape`: handled by the active OpenXcom state as usual.
- `Ctrl+Alt+R`: repeat the focused control.
- `Ctrl+Alt+S`: announce the current screen summary.

Text fields enter editing mode when activated. Press `Enter`, `Escape`, or `Tab` to leave editing mode.

## Battlescape

The tactical screen uses a separate logical cursor. It starts on the selected soldier and never announces undiscovered terrain or an enemy that is not currently visible.

- Arrow keys: move the logical cursor on the current level.
- `PageUp` / `PageDown`: move the cursor one level up / down.
- `Home`: return the cursor to the selected soldier.
- `Tab` / `Shift+Tab`: select the next / previous available soldier.
- `Space`: cycle Explore, Move, and Target modes.
- `Ctrl+Alt+E`, `Ctrl+Alt+M`, `Ctrl+Alt+T`: select Explore, Move, or Target mode directly.
- `Enter`: repeat the tile description in Explore mode, move along a checked path in Move mode, or perform the prepared action in Target mode.
- `Ctrl+Alt+F`: cycle snapshot, aimed, and automatic fire modes supported by the current weapon.
- `Ctrl+Alt+G`, `Ctrl+Alt+H`, `Ctrl+Alt+L`: prepare throw, melee, or guided-launcher actions.
- `Ctrl+Alt+P`, `Ctrl+Alt+C`: prepare panic or mind-control psionic actions.
- `Ctrl+Alt+1`, `Ctrl+Alt+2`: open the normal left- or right-hand action menu. Its entries expose action name, accuracy, and TU cost; this is also the route to grenade priming, scanners, mind probes, and medikits.
- `Ctrl+Alt+A` / `Ctrl+Alt+Shift+A`: next / previous currently visible enemy. Hidden enemies are never included.
- `Ctrl+Alt+X` / `Ctrl+Alt+Shift+X`: next / previous visible object on the battlefield, such as a corpse, weapon, ammunition, or other item. The cursor and camera move to the object.
- `Ctrl+Alt+Enter`: launch a guided projectile after at least one waypoint has been added with `Enter`.
- `Ctrl+Alt+K`: kneel or stand. `Ctrl+Alt+PageUp` / `Ctrl+Alt+PageDown`: move a flying unit up or down.
- `Ctrl+Alt+V`: cycle the TU reserve through none, snapshot, aimed, and automatic.
- `Ctrl+Alt+I`, `Ctrl+Alt+W`, `Ctrl+Alt+B`: inventory, reload, and abort-mission confirmation.
- `Ctrl+Alt+D` / `Ctrl+Alt+Shift+D`: cycle forward / backward through every door on the battle map, including doors on undiscovered tiles. The logical cursor and camera move to the door, and its index, coordinates, level, and tile edge are announced.
- `Ctrl+Alt+Q`: turn toward the cursor or open an adjacent door using the normal secondary action.
- `Ctrl+Alt+O`: announce the selected soldier's full status.
- `Ctrl+Alt+R`: repeat the current tile and, in Move mode, the path cost.
- `Ctrl+Alt+S`: announce mode, selected soldier, and current tile.
- The configured End Turn key requests the end of the turn and announces it.
- `Escape`: cancel Move or Target mode and return to Explore mode. In Explore mode it retains its normal game function.

Turn changes, newly visible enemies, damage to known units, and important status changes are announced without exposing undiscovered units or terrain.

### Motion scanner

Use a motion scanner through the normal hand action menu. When its image opens,
the screen reader announces the number of detected movement locations and the
first entry in a logical list.

- `Up` / `Left` and `Down` / `Right`: previous / next movement location.
- `PageUp` / `PageDown`: previous / next movement location.
- `Home` / `End`: first / last movement location.
- `Ctrl+Alt+R`: repeat the current location; `Ctrl+Alt+S`: repeat the scanner summary.
- `Escape` or right-click: close the normal scanner image and its logical list together.

Each entry gives the horizontal and vertical offset from the soldier using the
scanner, a level offset when applicable, and the displayed blob size as small,
medium, or large. It does not reveal the unit's identity or allegiance.

### Mind probe and psionic attacks

- When a mind probe opens its target-information screen, the target name and the first visible statistic are announced automatically.
- Use the arrow keys or `PageUp` / `PageDown` to browse every visible statistic; `Home` / `End` jump to the first / last field, and `Enter` or `Space` repeats the current field.
- `Escape` or right-click closes the mind-probe screen normally.
- Panic and mind-control actions announce the action and target when they start. When the attack finishes, its success or failure is announced instead of the generic shot message.

### Inventory and item dialogs

- On the inventory control, arrows and `PageUp` / `PageDown` browse carried and ground items; `Home` / `End` jump to the ends.
- `Enter` or `Space` moves the selected item to the next compatible inventory section. Repeated presses cycle through both hands, belt, backpack, legs, shoulders, ground, and any mod-defined sections; sections that are full or too small are skipped.
- `L` or `U` loads compatible ammunition or unloads a loaded weapon.
- During pre-battle equipment, `P` primes a grenade at zero and a digit primes it with that timer; repeating the command deactivates it. During battle, use the hand action menu instead so normal TU rules apply.
- In the medikit screen, arrows choose a body part, while `Tab` reaches heal, stimulant, pain-killer, and close controls.
- Grenade timer and item action popups use the ordinary `Tab`, arrow, and `Enter` control navigation.
- In the soldier-statistics screen, the first visible field is announced automatically. Arrows and `PageUp` / `PageDown` browse all fields; `Home` / `End` jump to the ends. `Tab` reaches the previous- and next-soldier buttons, which now have spoken names.

## Geoscape navigation

The globe adapter only exposes targets already known to the player. Undetected UFOs, mission sites, and alien bases are never included.

- `Ctrl+Alt+A` / `Ctrl+Alt+Shift+A`: next / previous known globe target. The globe is centered on it and its type, name, status, location, and available action are announced.
- `Enter` or `Ctrl+Alt+Enter`: open the selected target through the normal Geoscape popup flow.
- `Ctrl+Alt+C`: open or close the geographic cursor on the main Geoscape.
- Arrow keys while the geographic cursor is active: move north, south, east, or west.
- `Shift` plus an arrow: move five times the selected step.
- `PageUp` / `PageDown`: increase / decrease the geographic cursor step between 1, 5, and 15 degrees.
- `Home`: return the cursor to its starting position.
- `Ctrl+Alt+R`: repeat the selected target or cursor position.
- `Ctrl+Alt+S`: announce the Geoscape summary, time speed, and current target or cursor.
- The configured Geoscape number keys retain their normal time-speed actions and announce the new speed.

Base placement and craft destination selection start directly in geographic-cursor mode. `Enter` confirms the coordinate through the normal game validation and confirmation screens; `Escape` cancels as usual. Known targets can also be selected with `Ctrl+Alt+A` and `Ctrl+Alt+Shift+A` while choosing a craft destination.

### Air combat

When an interceptor engages a UFO, the air-combat window is announced automatically with the interceptor, UFO, distance, damage, and engagement mode.

- `Tab` / `Shift+Tab` or the arrow keys: move through engagement modes, disengage, available weapons, and minimise.
- `Enter` / `Space`: activate the current air-combat control.
- `Home` / `End`: move to the first / last air-combat control.
- `Ctrl+Alt+R`: repeat the current air-combat control.
- `Ctrl+Alt+S`: announce the current air-combat summary.

Weapon controls announce their weapon name, ammunition, and enabled state. Changes to combat status and interceptor damage are announced while the window is active.

## Basescape navigation

The base layout and the row of base selectors are exposed as logical controls. No facility information is inferred from hidden game data.

- `Tab` / `Shift+Tab`: move between the base grid, base selector, and ordinary screen controls.
- Arrow keys on the base grid: move one cell.
- `PageUp` / `PageDown`: move five cells vertically.
- `Home` / `End`: move to the first / last grid cell.
- `Enter` or `Space`: open the facility at the current cell, or confirm facility/lift placement through the normal validation path.
- Arrow keys on the base selector: select the previous or next founded base.
- `Home` / `End` on the base selector: select the first or last founded base.
- `Enter` or `Space` on the base selector: open the selected base.
- `Ctrl+Alt+R`: repeat the current cell or selected base.
- `Ctrl+Alt+S`: announce the base name, funds, and current grid cell.

Each grid announcement includes row and column, facility name or an empty-cell indication, construction time where applicable, facility dimensions, and whether the current placement is valid.

### Purchasing, hiring, selling, and sacking

- Use `Tab` to focus the list, then use Up/Down to select equipment, a craft, or a member of staff.
- Right Arrow increases the quantity selected for purchase, hire, sale, or dismissal by one.
- Left Arrow decreases the selected quantity by one.
- The screen reader repeats the row with the new quantity after each change. Normal limits for funds, living quarters, hangars, and stores remain in effect.
- Use `Tab` to reach the OK button and press `Enter` to confirm the transaction. Cancel leaves the base unchanged.

### Craft equipment

- Use `Tab` to focus the equipment list, then use Up/Down to select an item.
- Right Arrow moves one unit of the selected item from base stores to the craft.
- Left Arrow returns one unit of the selected item from the craft to base stores.
- The screen reader repeats the item name, base quantity, and craft quantity after each change. Normal craft-capacity and heavy-weapon-platform ammunition limits remain in effect.
- To close the screen, use `Tab` to reach the OK button and press `Enter`.

## Initial coverage

The generic state adapter exposes `TextButton`, `ToggleTextButton`, `TextList`, `TextEdit`, `ComboBox`, `Slider`, and `ArrowButton`. This covers the main menu, the new-game difficulty dialog, and the options pages without direct Tolk calls in gameplay or UI classes.

## Manual smoke test

1. Start NVDA.
2. Run the x64 `openxcom.exe` from the build output directory.
3. Confirm the Blind XCom introduction is spoken once.
4. Use `Tab` to inspect the main menu, then open New Game and Options with `Enter`.
5. In Options, use arrow keys to change combo boxes and sliders; use `Space` on toggle buttons.
6. Close dialogs with `Escape` and verify mouse input still works.
7. Start a tactical battle and confirm that the screen reader announces Battlescape, the selected soldier, and the starting tile.
8. Move the logical cursor with arrows and change levels with `PageUp` / `PageDown`; undiscovered tiles must not reveal terrain or units.
9. Use `Space` to enter Move mode, inspect a path cost, and press `Enter` to move.
10. Open the tactical inventory, select a grenade or another item, and press `Enter` repeatedly. Verify that every compatible section is announced and that a full or undersized section is skipped.
11. Move a grenade into a hand, close the inventory, prime it through that hand's action menu, prepare a throw with `Ctrl+Alt+G`, select a discovered tile, and press `Enter` to throw.
12. Use a motion scanner and verify that its summary and first movement location are announced. Browse all locations with the arrow keys, then press `Escape` and verify that both the image and logical list close.
13. Select a soldier with a loaded firearm, enter Target mode, move to a visible tile, and press `Enter` to fire.
14. Load or start a campaign, use `Ctrl+Alt+A` and `Ctrl+Alt+Shift+A` on the Geoscape, and verify that only discovered targets are announced.
15. Change time speed with the configured number keys and verify that the new speed is announced.
16. Open base placement or craft destination selection, move the geographic cursor, change its step, and confirm a valid coordinate with `Enter`.
17. Use a mind probe on a visible enemy. Verify that the target and first statistic are announced, browse all fields with the arrows, and close the screen with `Escape`.
18. Try both Panic and Mind Control with a psi amp. Verify that the action and target are announced at the start and that both successful and failed results are spoken when they complete.
19. Open a soldier's statistics from the tactical inventory. Verify that all visible fields can be browsed with the arrows and that `Tab` announces the previous- and next-soldier buttons by name.
