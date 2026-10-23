#include "GeoscapeAccessibility.h"
#include "AccessibilityInput.h"
#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Options.h"
#include "../Engine/State.h"
#include "../Geoscape/BuildNewBaseState.h"
#include "../Geoscape/DogfightState.h"
#include "../Geoscape/GeoscapeState.h"
#include "../Geoscape/Globe.h"
#include "../Geoscape/SelectDestinationState.h"
#include "../Mod/RuleCountry.h"
#include "../Mod/RuleRegion.h"
#include "../Mod/RuleUfo.h"
#include "../Savegame/AlienBase.h"
#include "../Savegame/Base.h"
#include "../Savegame/Country.h"
#include "../Savegame/Craft.h"
#include "../Savegame/CraftWeapon.h"
#include "../Savegame/MissionSite.h"
#include "../Savegame/Region.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Target.h"
#include "../Savegame/Ufo.h"
#include "../Savegame/Waypoint.h"
#include "../Mod/RuleCraftWeapon.h"
#include "../fmath.h"
#include <SDL.h>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>

namespace OpenXcom
{

namespace
{
	const double CURSOR_STEPS[] = {1.0, 5.0, 15.0};
	const int CURSOR_STEP_COUNT = 3;
	enum DogfightControl
	{
		DOGFIGHT_STANDOFF,
		DOGFIGHT_CAUTIOUS,
		DOGFIGHT_STANDARD,
		DOGFIGHT_AGGRESSIVE,
		DOGFIGHT_DISENGAGE,
		DOGFIGHT_WEAPON_1,
		DOGFIGHT_WEAPON_2,
		DOGFIGHT_MINIMIZE
	};

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

	double normalizedLongitude(double lon)
	{
		while (lon < 0.0) lon += M_PI * 2.0;
		while (lon >= M_PI * 2.0) lon -= M_PI * 2.0;
		return lon;
	}
}

GeoscapeAccessibility::GeoscapeAccessibility() : _state(0), _selectedTarget(0), _targetIndex(0),
	_cursorLon(0.0), _cursorLat(0.0), _originLon(0.0), _originLat(0.0), _stepIndex(1),
	_lastTimeSpeed(-1), _cursorActive(false), _dogfight(0), _dogfightControlIndex(0),
	_dogfightDamage(-1)
{
}

void GeoscapeAccessibility::reset()
{
	_state = 0;
	_targets.clear();
	_selectedTarget = 0;
	_targetIndex = 0;
	_lastTimeSpeed = -1;
	_cursorActive = false;
	_dogfight = 0;
	_dogfightControlIndex = 0;
	_dogfightStatus.clear();
	_dogfightDamage = -1;
}

bool GeoscapeAccessibility::supports(State *state)
{
	return dynamic_cast<GeoscapeState*>(state) != 0 ||
		dynamic_cast<BuildNewBaseState*>(state) != 0 ||
		dynamic_cast<SelectDestinationState*>(state) != 0;
}

Globe *GeoscapeAccessibility::getGlobe(State *state) const
{
	if (GeoscapeState *geoscape = dynamic_cast<GeoscapeState*>(state)) return geoscape->getGlobe();
	if (BuildNewBaseState *base = dynamic_cast<BuildNewBaseState*>(state)) return base->getGlobe();
	if (SelectDestinationState *destination = dynamic_cast<SelectDestinationState*>(state)) return destination->getGlobe();
	return 0;
}

std::vector<int> GeoscapeAccessibility::getDogfightControls(DogfightState *dogfight) const
{
	std::vector<int> controls;
	if (!dogfight) return controls;
	controls.push_back(DOGFIGHT_STANDOFF);
	controls.push_back(DOGFIGHT_CAUTIOUS);
	controls.push_back(DOGFIGHT_STANDARD);
	controls.push_back(DOGFIGHT_AGGRESSIVE);
	controls.push_back(DOGFIGHT_DISENGAGE);
	Craft *craft = dogfight->getCraft();
	if (craft && craft->getWeapons()->size() > 0 && craft->getWeapons()->at(0)) controls.push_back(DOGFIGHT_WEAPON_1);
	if (craft && craft->getWeapons()->size() > 1 && craft->getWeapons()->at(1)) controls.push_back(DOGFIGHT_WEAPON_2);
	controls.push_back(DOGFIGHT_MINIMIZE);
	return controls;
}

std::string GeoscapeAccessibility::describeDogfight(State *state, DogfightState *dogfight) const
{
	if (!state || !dogfight || !dogfight->getCraft() || !dogfight->getUfo()) return std::string();
	const char *mode = "STR_STANDOFF";
	switch (dogfight->getEngagementMode())
	{
	case DogfightState::MODE_CAUTIOUS: mode = "STR_CAUTIOUS_ATTACK"; break;
	case DogfightState::MODE_STANDARD: mode = "STR_STANDARD_ATTACK"; break;
	case DogfightState::MODE_AGGRESSIVE: mode = "STR_AGGRESSIVE_ATTACK"; break;
	case DogfightState::MODE_DISENGAGE: mode = "STR_DISENGAGING"; break;
	default: break;
	}
	return state->tr("STR_ACCESSIBILITY_DOGFIGHT_SUMMARY")
		.arg(dogfight->getCraft()->getName(state->getGame()->getLanguage()))
		.arg(dogfight->getUfo()->getName(state->getGame()->getLanguage()))
		.arg(dogfight->getCurrentDistance())
		.arg(dogfight->getCraft()->getDamagePercentage())
		.arg(text(state, mode));
}

std::string GeoscapeAccessibility::describeDogfightControl(State *state, DogfightState *dogfight, int control) const
{
	if (!state || !dogfight) return std::string();
	std::string name;
	DogfightState::EngagementMode mode = DogfightState::MODE_STANDOFF;
	bool modeControl = true;
	switch (control)
	{
	case DOGFIGHT_STANDOFF: name = text(state, "STR_STANDOFF"); mode = DogfightState::MODE_STANDOFF; break;
	case DOGFIGHT_CAUTIOUS: name = text(state, "STR_CAUTIOUS_ATTACK"); mode = DogfightState::MODE_CAUTIOUS; break;
	case DOGFIGHT_STANDARD: name = text(state, "STR_STANDARD_ATTACK"); mode = DogfightState::MODE_STANDARD; break;
	case DOGFIGHT_AGGRESSIVE: name = text(state, "STR_AGGRESSIVE_ATTACK"); mode = DogfightState::MODE_AGGRESSIVE; break;
	case DOGFIGHT_DISENGAGE: name = text(state, "STR_ACCESSIBILITY_DOGFIGHT_DISENGAGE"); mode = DogfightState::MODE_DISENGAGE; break;
	case DOGFIGHT_MINIMIZE:
		return text(state, "STR_ACCESSIBILITY_DOGFIGHT_MINIMIZE");
	case DOGFIGHT_WEAPON_1:
	case DOGFIGHT_WEAPON_2:
	{
		const size_t weaponIndex = control == DOGFIGHT_WEAPON_1 ? 0 : 1;
		Craft *craft = dogfight->getCraft();
		if (!craft || craft->getWeapons()->size() <= weaponIndex || !craft->getWeapons()->at(weaponIndex)) return std::string();
		CraftWeapon *weapon = craft->getWeapons()->at(weaponIndex);
		return state->tr("STR_ACCESSIBILITY_DOGFIGHT_WEAPON")
			.arg(static_cast<int>(weaponIndex + 1))
			.arg(text(state, weapon->getRules()->getType()))
			.arg(weapon->getAmmo())
			.arg(text(state, dogfight->isWeaponEnabled(weaponIndex) ? "STR_ACCESSIBILITY_ON" : "STR_ACCESSIBILITY_OFF"));
	}
	default: modeControl = false; break;
	}
	if (!modeControl) return std::string();
	std::string result = state->tr("STR_ACCESSIBILITY_DOGFIGHT_ACTION").arg(name);
	if (dogfight->getEngagementMode() == mode)
	{
		if (!result.empty()) result += " ";
		result += text(state, "STR_ACCESSIBILITY_SELECTED");
	}
	return result;
}

void GeoscapeAccessibility::syncDogfight(State *state, std::vector<std::string> &messages)
{
	GeoscapeState *geoscape = dynamic_cast<GeoscapeState*>(state);
	DogfightState *current = geoscape ? geoscape->getActiveDogfight() : 0;
	if (current != _dogfight)
	{
		_dogfight = current;
		_dogfightControlIndex = 0;
		_dogfightStatus.clear();
		_dogfightDamage = -1;
		if (_dogfight)
		{
			messages.push_back(text(state, "STR_ACCESSIBILITY_DOGFIGHT"));
			messages.push_back(describeDogfight(state, _dogfight));
			std::vector<int> controls = getDogfightControls(_dogfight);
			if (!controls.empty()) messages.push_back(describeDogfightControl(state, _dogfight, controls.front()));
			messages.push_back(text(state, "STR_ACCESSIBILITY_DOGFIGHT_HELP"));
			_dogfightStatus = _dogfight->getStatusText();
			_dogfightDamage = _dogfight->getCraft() ? _dogfight->getCraft()->getDamagePercentage() : -1;
		}
		return;
	}
	if (!_dogfight) return;

	const std::string status = _dogfight->getStatusText();
	if (status != _dogfightStatus)
	{
		_dogfightStatus = status;
		if (!status.empty()) messages.push_back(state->tr("STR_ACCESSIBILITY_DOGFIGHT_STATUS").arg(status));
	}
	const int damage = _dogfight->getCraft() ? _dogfight->getCraft()->getDamagePercentage() : -1;
	if (_dogfightDamage >= 0 && damage != _dogfightDamage)
	{
		messages.push_back(state->tr("STR_ACCESSIBILITY_DOGFIGHT_DAMAGE").arg(damage));
	}
	_dogfightDamage = damage;
}

void GeoscapeAccessibility::activateDogfightControl(Action *action, State *state, DogfightState *dogfight, int control, std::vector<std::string> &messages)
{
	if (!dogfight) return;
	switch (control)
	{
	case DOGFIGHT_STANDOFF: dogfight->accessibilitySetEngagementMode(DogfightState::MODE_STANDOFF); break;
	case DOGFIGHT_CAUTIOUS: dogfight->accessibilitySetEngagementMode(DogfightState::MODE_CAUTIOUS); break;
	case DOGFIGHT_STANDARD: dogfight->accessibilitySetEngagementMode(DogfightState::MODE_STANDARD); break;
	case DOGFIGHT_AGGRESSIVE: dogfight->accessibilitySetEngagementMode(DogfightState::MODE_AGGRESSIVE); break;
	case DOGFIGHT_DISENGAGE: dogfight->accessibilitySetEngagementMode(DogfightState::MODE_DISENGAGE); break;
	case DOGFIGHT_WEAPON_1: dogfight->weapon1Click(action); break;
	case DOGFIGHT_WEAPON_2: dogfight->weapon2Click(action); break;
	case DOGFIGHT_MINIMIZE: dogfight->btnMinimizeClick(action); break;
	default: return;
	}
	_dogfightStatus = dogfight->getStatusText();
	if (control == DOGFIGHT_MINIMIZE && dogfight->isMinimized())
	{
		messages.push_back(text(state, "STR_ACCESSIBILITY_DOGFIGHT_MINIMIZED"));
		_dogfight = 0;
		return;
	}
	if (control == DOGFIGHT_MINIMIZE && !dogfight->getStatusText().empty())
	{
		messages.push_back(state->tr("STR_ACCESSIBILITY_DOGFIGHT_STATUS").arg(dogfight->getStatusText()));
		return;
	}
	messages.push_back(describeDogfightControl(state, dogfight, control));
}

bool GeoscapeAccessibility::handleDogfight(Action *action, State *state, DogfightState *dogfight, std::vector<std::string> &messages)
{
	std::vector<int> controls = getDogfightControls(dogfight);
	if (controls.empty()) return false;
	if (_dogfightControlIndex >= controls.size()) _dogfightControlIndex = 0;
	const SDLKey key = static_cast<SDLKey>(AccessibilityInput::getKey(action));
	const int modifiers = AccessibilityInput::getModifiers(action);
	const bool command = AccessibilityInput::isCommandChord(modifiers);
	if (command && key == SDLK_s)
	{
		messages.push_back(describeDogfight(state, dogfight));
		return true;
	}
	if (command && key == SDLK_r)
	{
		messages.push_back(describeDogfightControl(state, dogfight, controls[_dogfightControlIndex]));
		return true;
	}
	int direction = 0;
	if (key == SDLK_TAB) direction = (modifiers & KMOD_SHIFT) ? -1 : 1;
	else if (key == SDLK_LEFT || key == SDLK_UP) direction = -1;
	else if (key == SDLK_RIGHT || key == SDLK_DOWN) direction = 1;
	if (direction != 0)
	{
		if (direction < 0) _dogfightControlIndex = _dogfightControlIndex == 0 ? controls.size() - 1 : _dogfightControlIndex - 1;
		else _dogfightControlIndex = (_dogfightControlIndex + 1) % controls.size();
		messages.push_back(describeDogfightControl(state, dogfight, controls[_dogfightControlIndex]));
		return true;
	}
	if (key == SDLK_HOME)
	{
		_dogfightControlIndex = 0;
		messages.push_back(describeDogfightControl(state, dogfight, controls.front()));
		return true;
	}
	if (key == SDLK_END)
	{
		_dogfightControlIndex = controls.size() - 1;
		messages.push_back(describeDogfightControl(state, dogfight, controls.back()));
		return true;
	}
	if (key == SDLK_RETURN || key == SDLK_KP_ENTER || key == SDLK_SPACE)
	{
		activateDogfightControl(action, state, dogfight, controls[_dogfightControlIndex], messages);
		return true;
	}
	return false;
}

void GeoscapeAccessibility::collectTargets(State *state)
{
	Target *previous = _selectedTarget;
	_targets.clear();
	SavedGame *save = state->getGame()->getSavedGame();
	const bool choosingDestination = dynamic_cast<SelectDestinationState*>(state) != 0;

	if (!choosingDestination)
	{
		for (std::vector<Base*>::iterator base = save->getBases()->begin(); base != save->getBases()->end(); ++base)
		{
			if ((*base)->getLongitude() == 0.0 && (*base)->getLatitude() == 0.0) continue;
			_targets.push_back(*base);
			for (std::vector<Craft*>::iterator craft = (*base)->getCrafts()->begin(); craft != (*base)->getCrafts()->end(); ++craft)
			{
				_targets.push_back(*craft);
			}
		}
	}
	for (std::vector<Ufo*>::iterator ufo = save->getUfos()->begin(); ufo != save->getUfos()->end(); ++ufo)
	{
		if ((*ufo)->getDetected() && (*ufo)->getStatus() != Ufo::DESTROYED) _targets.push_back(*ufo);
	}
	for (std::vector<MissionSite*>::iterator site = save->getMissionSites()->begin(); site != save->getMissionSites()->end(); ++site)
	{
		if ((*site)->getDetected()) _targets.push_back(*site);
	}
	for (std::vector<AlienBase*>::iterator base = save->getAlienBases()->begin(); base != save->getAlienBases()->end(); ++base)
	{
		if ((*base)->isDiscovered()) _targets.push_back(*base);
	}
	for (std::vector<Waypoint*>::iterator waypoint = save->getWaypoints()->begin(); waypoint != save->getWaypoints()->end(); ++waypoint)
	{
		_targets.push_back(*waypoint);
	}

	_selectedTarget = 0;
	if (previous)
	{
		std::vector<Target*>::iterator found = std::find(_targets.begin(), _targets.end(), previous);
		if (found != _targets.end())
		{
			_targetIndex = static_cast<size_t>(found - _targets.begin());
			_selectedTarget = *found;
		}
	}
	if (_targetIndex >= _targets.size()) _targetIndex = 0;
}

std::string GeoscapeAccessibility::timeSpeedName(State *state, int speed) const
{
	static const char *ids[] =
	{
		"STR_5_SECONDS", "STR_1_MINUTE", "STR_5_MINUTES",
		"STR_30_MINUTES", "STR_1_HOUR", "STR_1_DAY"
	};
	if (speed < 0 || speed >= 6) return text(state, "STR_ACCESSIBILITY_UNKNOWN");
	return text(state, ids[speed]);
}

std::string GeoscapeAccessibility::describePosition(State *state) const
{
	return describePosition(state, _cursorLon, _cursorLat, 0);
}

std::string GeoscapeAccessibility::describePosition(State *state, double lon, double lat, Target *excludedTarget) const
{
	SavedGame *save = state->getGame()->getSavedGame();
	Globe *globe = getGlobe(state);
	double lonDegrees = Rad2Deg(lon);
	if (lonDegrees > 180.0) lonDegrees -= 360.0;
	const double latDegrees = Rad2Deg(lat);

	std::ostringstream latitude;
	latitude << std::fixed << std::setprecision(1) << std::fabs(latDegrees);
	std::ostringstream longitude;
	longitude << std::fixed << std::setprecision(1) << std::fabs(lonDegrees);

	std::ostringstream result;
	appendSentence(result, state->tr("STR_ACCESSIBILITY_GEO_COORDINATES")
		.arg(latitude.str()).arg(text(state, latDegrees > 0.0 ? "STR_ACCESSIBILITY_SOUTH" : "STR_ACCESSIBILITY_NORTH"))
		.arg(longitude.str()).arg(text(state, lonDegrees < 0.0 ? "STR_ACCESSIBILITY_WEST" : "STR_ACCESSIBILITY_EAST")));

	Region *region = save->locateRegion(lon, lat);
	if (region) appendSentence(result, state->tr("STR_ACCESSIBILITY_GEO_REGION").arg(text(state, region->getRules()->getType())));
	for (std::vector<Country*>::iterator country = save->getCountries()->begin(); country != save->getCountries()->end(); ++country)
	{
		if ((*country)->getRules()->insideCountry(lon, lat))
		{
			appendSentence(result, state->tr("STR_ACCESSIBILITY_GEO_COUNTRY").arg(text(state, (*country)->getRules()->getType())));
			break;
		}
	}
	appendSentence(result, text(state, globe && globe->insideLand(lon, lat) ? "STR_ACCESSIBILITY_GEO_LAND" : "STR_ACCESSIBILITY_GEO_WATER"));

	Target *nearest = 0;
	double nearestDistance = std::numeric_limits<double>::max();
	for (std::vector<Target*>::const_iterator target = _targets.begin(); target != _targets.end(); ++target)
	{
		if (*target == excludedTarget) continue;
		const double distance = (*target)->getDistance(lon, lat);
		if (distance < nearestDistance)
		{
			nearest = *target;
			nearestDistance = distance;
		}
	}
	if (nearest)
	{
		std::ostringstream distance;
		distance << std::fixed << std::setprecision(1) << Rad2Deg(nearestDistance);
		appendSentence(result, state->tr("STR_ACCESSIBILITY_GEO_NEAREST")
			.arg(nearest->getName(state->getGame()->getLanguage())).arg(distance.str()));
	}
	return result.str();
}

std::string GeoscapeAccessibility::describeTarget(State *state, Target *target) const
{
	if (!target) return text(state, "STR_ACCESSIBILITY_GEO_NO_TARGETS");
	std::ostringstream result;
	appendSentence(result, target->getName(state->getGame()->getLanguage()) + ".");

	std::string status;
	std::string action;
	if (dynamic_cast<Base*>(target))
	{
		appendSentence(result, text(state, "STR_ACCESSIBILITY_GEO_BASE"));
		status = text(state, "STR_ACCESSIBILITY_GEO_OPERATIONAL");
		action = text(state, "STR_ACCESSIBILITY_GEO_ACTION_BASE");
	}
	else if (Craft *craft = dynamic_cast<Craft*>(target))
	{
		appendSentence(result, text(state, "STR_ACCESSIBILITY_GEO_CRAFT"));
		status = text(state, craft->getStatus());
		action = text(state, "STR_ACCESSIBILITY_GEO_ACTION_CRAFT");
	}
	else if (Ufo *ufo = dynamic_cast<Ufo*>(target))
	{
		appendSentence(result, text(state, "STR_ACCESSIBILITY_GEO_UFO"));
		appendSentence(result, text(state, ufo->getRules()->getType()));
		switch (ufo->getStatus())
		{
		case Ufo::LANDED: status = text(state, "STR_ACCESSIBILITY_GEO_LANDED"); break;
		case Ufo::CRASHED: status = text(state, "STR_ACCESSIBILITY_GEO_CRASHED"); break;
		case Ufo::FLYING: status = text(state, "STR_ACCESSIBILITY_GEO_FLYING"); break;
		default: status = text(state, "STR_ACCESSIBILITY_UNKNOWN"); break;
		}
		action = text(state, "STR_ACCESSIBILITY_GEO_ACTION_UFO");
	}
	else if (MissionSite *site = dynamic_cast<MissionSite*>(target))
	{
		appendSentence(result, text(state, "STR_ACCESSIBILITY_GEO_MISSION_SITE"));
		if (!site->getCity().empty()) appendSentence(result, text(state, site->getCity()));
		status = text(state, "STR_ACCESSIBILITY_GEO_ACTIVE");
		action = text(state, "STR_ACCESSIBILITY_GEO_ACTION_SITE");
	}
	else if (dynamic_cast<AlienBase*>(target))
	{
		appendSentence(result, text(state, "STR_ACCESSIBILITY_GEO_ALIEN_BASE"));
		status = text(state, "STR_ACCESSIBILITY_GEO_DETECTED");
		action = text(state, "STR_ACCESSIBILITY_GEO_ACTION_SITE");
	}
	else
	{
		appendSentence(result, text(state, "STR_ACCESSIBILITY_GEO_WAYPOINT"));
		action = text(state, "STR_ACCESSIBILITY_GEO_ACTION_WAYPOINT");
	}
	appendSentence(result, status);

	appendSentence(result, describePosition(state, target->getLongitude(), target->getLatitude(), target));
	appendSentence(result, action);
	appendSentence(result, state->tr("STR_ACCESSIBILITY_GEO_TARGET_POSITION").arg(static_cast<int>(_targetIndex + 1)).arg(static_cast<int>(_targets.size())));
	return result.str();
}

std::string GeoscapeAccessibility::describeSummary(State *state) const
{
	std::ostringstream result;
	appendSentence(result, text(state, "STR_ACCESSIBILITY_GEOSCAPE"));
	if (GeoscapeState *geoscape = dynamic_cast<GeoscapeState*>(state))
	{
		appendSentence(result, state->tr("STR_ACCESSIBILITY_GEO_TIME_SPEED").arg(timeSpeedName(state, geoscape->getTimeSpeedIndex())));
	}
	appendSentence(result, state->tr("STR_ACCESSIBILITY_GEO_TARGET_COUNT").arg(static_cast<int>(_targets.size())));
	if (_selectedTarget) appendSentence(result, describeTarget(state, _selectedTarget));
	else if (_cursorActive) appendSentence(result, describePosition(state));
	return result.str();
}

void GeoscapeAccessibility::activate(State *state, std::vector<std::string> &messages)
{
	_state = state;
	_selectedTarget = 0;
	_targetIndex = 0;
	_stepIndex = 1;
	_lastTimeSpeed = -1;
	collectTargets(state);

	SavedGame *save = state->getGame()->getSavedGame();
	_cursorLon = save->getGlobeLongitude();
	_cursorLat = save->getGlobeLatitude();
	if (SelectDestinationState *destination = dynamic_cast<SelectDestinationState*>(state))
	{
		_cursorLon = destination->getCraft()->getLongitude();
		_cursorLat = destination->getCraft()->getLatitude();
	}
	_originLon = _cursorLon;
	_originLat = _cursorLat;
	_cursorActive = dynamic_cast<GeoscapeState*>(state) == 0;

	if (GeoscapeState *geoscape = dynamic_cast<GeoscapeState*>(state))
	{
		messages.push_back(text(state, "STR_ACCESSIBILITY_GEOSCAPE"));
		messages.push_back(state->tr("STR_ACCESSIBILITY_GEO_TARGET_COUNT").arg(static_cast<int>(_targets.size())));
		_lastTimeSpeed = geoscape->getTimeSpeedIndex();
		messages.push_back(state->tr("STR_ACCESSIBILITY_GEO_TIME_SPEED").arg(timeSpeedName(state, _lastTimeSpeed)));
		messages.push_back(text(state, "STR_ACCESSIBILITY_GEO_HELP"));
	}
	else
	{
		if (Globe *globe = getGlobe(state)) globe->center(_cursorLon, _cursorLat);
		messages.push_back(text(state, dynamic_cast<BuildNewBaseState*>(state) ? "STR_SELECT_SITE_FOR_NEW_BASE" : "STR_SELECT_DESTINATION"));
		messages.push_back(text(state, "STR_ACCESSIBILITY_GEO_CURSOR_ON"));
		messages.push_back(describePosition(state));
		messages.push_back(state->tr("STR_ACCESSIBILITY_GEO_STEP").arg(CURSOR_STEPS[_stepIndex]));
	}
	syncDogfight(state, messages);
}

void GeoscapeAccessibility::update(State *state, std::vector<std::string> &messages)
{
	GeoscapeState *geoscape = dynamic_cast<GeoscapeState*>(state);
	if (!geoscape) return;
	syncDogfight(state, messages);
	if (_dogfight) return;
	const int speed = geoscape->getTimeSpeedIndex();
	if (_lastTimeSpeed != -1 && speed != _lastTimeSpeed)
	{
		messages.push_back(state->tr("STR_ACCESSIBILITY_GEO_TIME_SPEED").arg(timeSpeedName(state, speed)));
	}
	_lastTimeSpeed = speed;
}

void GeoscapeAccessibility::cycleTarget(State *state, int direction, std::vector<std::string> &messages)
{
	collectTargets(state);
	if (_targets.empty())
	{
		messages.push_back(text(state, "STR_ACCESSIBILITY_GEO_NO_TARGETS"));
		return;
	}
	if (!_selectedTarget)
	{
		_targetIndex = direction < 0 ? _targets.size() - 1 : 0;
	}
	else if (direction < 0)
	{
		_targetIndex = _targetIndex == 0 ? _targets.size() - 1 : _targetIndex - 1;
	}
	else
	{
		_targetIndex = (_targetIndex + 1) % _targets.size();
	}
	_selectedTarget = _targets[_targetIndex];
	_cursorLon = _selectedTarget->getLongitude();
	_cursorLat = _selectedTarget->getLatitude();
	if (Globe *globe = getGlobe(state)) globe->center(_cursorLon, _cursorLat);
	messages.push_back(describeTarget(state, _selectedTarget));
}

void GeoscapeAccessibility::moveCursor(State *state, int dx, int dy, bool coarse, std::vector<std::string> &messages)
{
	const double multiplier = coarse ? 5.0 : 1.0;
	const double step = Deg2Rad(CURSOR_STEPS[_stepIndex] * multiplier);
	_cursorLon = normalizedLongitude(_cursorLon + dx * step);
	_cursorLat = std::max(-M_PI_2, std::min(M_PI_2, _cursorLat + dy * step));
	_cursorActive = true;
	_selectedTarget = 0;
	collectTargets(state);
	if (Globe *globe = getGlobe(state)) globe->center(_cursorLon, _cursorLat);
	messages.push_back(describePosition(state));
}

void GeoscapeAccessibility::changeStep(State *state, int direction, std::vector<std::string> &messages)
{
	_stepIndex = std::max(0, std::min(CURSOR_STEP_COUNT - 1, _stepIndex + direction));
	messages.push_back(state->tr("STR_ACCESSIBILITY_GEO_STEP").arg(CURSOR_STEPS[_stepIndex]));
}

void GeoscapeAccessibility::activateCurrent(State *state, std::vector<std::string> &messages)
{
	if (BuildNewBaseState *base = dynamic_cast<BuildNewBaseState*>(state))
	{
		base->placeBase(_cursorLon, _cursorLat);
		return;
	}
	if (SelectDestinationState *destination = dynamic_cast<SelectDestinationState*>(state))
	{
		if (_selectedTarget) destination->selectTarget(_selectedTarget);
		else destination->selectDestination(_cursorLon, _cursorLat);
		return;
	}
	if (GeoscapeState *geoscape = dynamic_cast<GeoscapeState*>(state))
	{
		if (_selectedTarget) geoscape->openTarget(_selectedTarget);
		else messages.push_back(describePosition(state));
	}
}

bool GeoscapeAccessibility::handle(Action *action, State *state, std::vector<std::string> &messages)
{
	if (!action || !supports(state) || action->getDetails()->type != SDL_KEYDOWN) return false;
	if (_state != state) activate(state, messages);

	const SDLKey key = static_cast<SDLKey>(AccessibilityInput::getKey(action));
	const int modifiers = AccessibilityInput::getModifiers(action);
	const bool command = AccessibilityInput::isCommandChord(modifiers);
	const bool mainGeoscape = dynamic_cast<GeoscapeState*>(state) != 0;
	if (mainGeoscape)
	{
		syncDogfight(state, messages);
		if (_dogfight) return handleDogfight(action, state, _dogfight, messages);
	}

	if (command && key == SDLK_a)
	{
		cycleTarget(state, (modifiers & KMOD_SHIFT) != 0 ? -1 : 1, messages);
		return true;
	}
	if (command && key == SDLK_c)
	{
		if (mainGeoscape) _cursorActive = !_cursorActive;
		else _cursorActive = true;
		_selectedTarget = 0;
		messages.push_back(text(state, _cursorActive ? "STR_ACCESSIBILITY_GEO_CURSOR_ON" : "STR_ACCESSIBILITY_GEO_CURSOR_OFF"));
		if (_cursorActive) messages.push_back(describePosition(state));
		return true;
	}
	if (command && key == SDLK_r)
	{
		if (_selectedTarget) messages.push_back(describeTarget(state, _selectedTarget));
		else if (_cursorActive) messages.push_back(describePosition(state));
		else messages.push_back(describeSummary(state));
		return true;
	}
	if (command && key == SDLK_s)
	{
		collectTargets(state);
		messages.push_back(describeSummary(state));
		return true;
	}
	if ((command && (key == SDLK_RETURN || key == SDLK_KP_ENTER)) ||
		((key == SDLK_RETURN || key == SDLK_KP_ENTER) && (_cursorActive || _selectedTarget)))
	{
		activateCurrent(state, messages);
		return true;
	}

	if (_cursorActive)
	{
		const bool coarse = (modifiers & KMOD_SHIFT) != 0;
		if (key == SDLK_LEFT) { moveCursor(state, -1, 0, coarse, messages); return true; }
		if (key == SDLK_RIGHT) { moveCursor(state, 1, 0, coarse, messages); return true; }
		if (key == SDLK_UP) { moveCursor(state, 0, -1, coarse, messages); return true; }
		if (key == SDLK_DOWN) { moveCursor(state, 0, 1, coarse, messages); return true; }
		if (key == SDLK_PAGEUP) { changeStep(state, 1, messages); return true; }
		if (key == SDLK_PAGEDOWN) { changeStep(state, -1, messages); return true; }
		if (key == SDLK_HOME)
		{
			_cursorLon = _originLon;
			_cursorLat = _originLat;
			_selectedTarget = 0;
			if (Globe *globe = getGlobe(state)) globe->center(_cursorLon, _cursorLat);
			messages.push_back(describePosition(state));
			return true;
		}
		if (key == SDLK_ESCAPE && mainGeoscape)
		{
			_cursorActive = false;
			_selectedTarget = 0;
			messages.push_back(text(state, "STR_ACCESSIBILITY_GEO_CURSOR_OFF"));
			return true;
		}
	}
	else if (key == SDLK_ESCAPE && mainGeoscape && _selectedTarget)
	{
		_selectedTarget = 0;
		messages.push_back(text(state, "STR_ACCESSIBILITY_GEO_OBJECT_BROWSING_OFF"));
		return true;
	}
	return false;
}

}
