#pragma once

#include <string>
#include <vector>

namespace OpenXcom
{

class Action;
class DogfightState;
class Globe;
class State;
class Target;

/**
 * Non-visual navigation adapter for the Geoscape globe.
 */
class GeoscapeAccessibility
{
private:
	State *_state;
	std::vector<Target*> _targets;
	Target *_selectedTarget;
	size_t _targetIndex;
	double _cursorLon;
	double _cursorLat;
	double _originLon;
	double _originLat;
	int _stepIndex;
	int _lastTimeSpeed;
	bool _cursorActive;
	DogfightState *_dogfight;
	size_t _dogfightControlIndex;
	std::string _dogfightStatus;
	int _dogfightDamage;

	Globe *getGlobe(State *state) const;
	void syncDogfight(State *state, std::vector<std::string> &messages);
	std::vector<int> getDogfightControls(DogfightState *dogfight) const;
	std::string describeDogfight(State *state, DogfightState *dogfight) const;
	std::string describeDogfightControl(State *state, DogfightState *dogfight, int control) const;
	bool handleDogfight(Action *action, State *state, DogfightState *dogfight, std::vector<std::string> &messages);
	void activateDogfightControl(Action *action, State *state, DogfightState *dogfight, int control, std::vector<std::string> &messages);
	void collectTargets(State *state);
	void cycleTarget(State *state, int direction, std::vector<std::string> &messages);
	void moveCursor(State *state, int dx, int dy, bool coarse, std::vector<std::string> &messages);
	void changeStep(State *state, int direction, std::vector<std::string> &messages);
	void activateCurrent(State *state, std::vector<std::string> &messages);
	std::string describePosition(State *state) const;
	std::string describePosition(State *state, double lon, double lat, Target *excludedTarget) const;
	std::string describeTarget(State *state, Target *target) const;
	std::string describeSummary(State *state) const;
	std::string timeSpeedName(State *state, int speed) const;

public:
	GeoscapeAccessibility();
	void reset();
	static bool supports(State *state);
	void activate(State *state, std::vector<std::string> &messages);
	void update(State *state, std::vector<std::string> &messages);
	bool handle(Action *action, State *state, std::vector<std::string> &messages);
};

}
