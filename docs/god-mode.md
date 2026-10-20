# God mode

[Русская версия](god-mode_ru.md)

Start the game with `--god` to enable god mode for that process:

```text
openxcom.exe --accessibility --god -playIntro false
```

When a new campaign is created, its funds are set to the maximum signed
64-bit value. Existing saved campaigns keep their current funds.

During tactical combat, human XCOM soldiers do not receive health, stun,
armor, fire, or fatal-wound damage and cannot be knocked unconscious by a
falling unit. Aliens, civilians, tanks, and mind-controlled aliens are not
protected. An XCOM soldier remains protected while mind-controlled because
protection follows the unit's original faction.

God mode is controlled only by the command-line flag and is not stored in the
configuration file or savegame. Omit `--god` to run the game normally.
