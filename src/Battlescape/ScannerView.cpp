/*
 * Copyright 2010-2016 OpenXcom Developers.
 *
 * This file is part of OpenXcom.
 *
 * OpenXcom is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * OpenXcom is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "ScannerView.h"
#include "../Engine/Game.h"
#include "../Engine/SurfaceSet.h"
#include "../Mod/Mod.h"
#include "../Engine/Action.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/Tile.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/SavedBattleGame.h"

namespace OpenXcom
{

/**
 * Initializes the Scanner view.
 * @param w The ScannerView width.
 * @param h The ScannerView height.
 * @param x The ScannerView x origin.
 * @param y The ScannerView y origin.
 * @param game Pointer to the core game.
 * @param unit The current unit.
 */
ScannerView::ScannerView (int w, int h, int x, int y, Game * game, BattleUnit *unit) : InteractiveSurface(w, h, x, y), _game(game), _unit(unit), _frame(0)
{
	_redraw = true;
}

/**
 * Gets the movement markers currently displayed by the scanner.
 * Coordinates are relative to the scanning unit; size is the unanimated
 * DETBLOB frame from 0 (smallest) to 5 (largest).
 * @return Movement markers in the same order as they are drawn.
 */
std::vector<MotionScannerBlip> ScannerView::getBlips() const
{
	std::vector<MotionScannerBlip> blips;
	if (!_game || !_unit || !_game->getSavedGame() || !_game->getSavedGame()->getSavedBattle()) return blips;

	SavedBattleGame *battle = _game->getSavedGame()->getSavedBattle();
	for (int x = -9; x < 10; ++x)
	{
		for (int y = -9; y < 10; ++y)
		{
			for (int z = 0; z < battle->getMapSizeZ(); ++z)
			{
				Tile *tile = battle->getTile(Position(x, y, z) + Position(_unit->getPosition().x, _unit->getPosition().y, 0));
				if (!tile || !tile->getUnit() || !tile->getUnit()->getMotionPoints()) continue;

				int size = tile->getUnit()->getMotionPoints() / 5;
				if (size < 0) continue;
				if (size > 5) size = 5;
				MotionScannerBlip blip = { x, y, z - _unit->getPosition().z, size };
				blips.push_back(blip);
			}
		}
	}
	return blips;
}

/**
 * Draws the ScannerView view.
 */
void ScannerView::draw()
{
	SurfaceSet *set = _game->getMod()->getSurfaceSet("DETBLOB.DAT");
	Surface *surface = 0;

	clear();

	this->lock();
	const std::vector<MotionScannerBlip> blips = getBlips();
	for (std::vector<MotionScannerBlip>::const_iterator i = blips.begin(); i != blips.end(); ++i)
	{
		surface = set->getFrame(i->size + _frame);
		surface->blitNShade(this, Surface::getX()+((9+i->x)*8)-4, Surface::getY()+((9+i->y)*8)-4, 0);
	}

	// the arrow of the direction the unit is pointed
	surface = set->getFrame(7 + _unit->getDirection());

	surface->blitNShade(this, Surface::getX()+(9*8)-4, Surface::getY()+(9*8)-4, 0);
	this->unlock();


}

/**
 * Handles clicks on the scanner view.
 * @param action Pointer to an action.
 * @param state State that the action handlers belong to.
 */
void ScannerView::mouseClick (Action *, State *)
{
}

/**
 * Updates the scanner animation.
 */
void ScannerView::animate()
{
	_frame++;
	if (_frame > 1)
	{
		_frame = 0;
	}
	_redraw = true;
}

}
