// $Id$

/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Deadly Rooms of Death.
 *
 * The Initial Developer of the Original Code is
 * Caravel Software.
 * Portions created by the Initial Developer are Copyright (C) 2001, 2002, 2005, 2020
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 * Maurycy Zarzycki (maurycy@evidentlycube.com)
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef PUZZLEMODEOPTIONS_H
#define PUZZLEMODEOPTIONS_H

#include <BackEndLib/Types.h>
#include <DRODLib/DbPackedVars.h>
#include <DRODLib/SettingsKeys.h>

struct PuzzleModeOptions {
	PuzzleModeOptions() :
		bIsEnabled(false),
		uGridOpacity(128),
		bHideAnimations(false),
		bHideBuildMarkers(false),
		bHideLighting(false),
		bHideWeather(false),
		bShowEvilEyeBeams(true),
		bShowReverseEvilEyeBeams(false),
		bShowBrokenWalls(false),
		bShowSecretWalls(false),
		bShowSpiders(true) {}

	PuzzleModeOptions(CDbPackedVars& playerSettings) {
		this->bIsEnabled = false;

		this->uGridOpacity = playerSettings.GetVar(Settings::PuzzleMode_GridOpacity, (Uint8)128);

		this->bHideAnimations = playerSettings.GetVar(Settings::PuzzleMode_VisibilityHideAnimations, false);
		this->bHideBuildMarkers = playerSettings.GetVar(Settings::PuzzleMode_VisibilityHideBuildMarkers, false);
		this->bHideLighting = playerSettings.GetVar(Settings::PuzzleMode_VisibilityHideLighting, false);
		this->bHideWeather = playerSettings.GetVar(Settings::PuzzleMode_VisibilityHideWeather, false);

		this->bShowEvilEyeBeams = playerSettings.GetVar(Settings::PuzzleMode_VisibilityEyeBeams, true);
		this->bShowReverseEvilEyeBeams = playerSettings.GetVar(Settings::PuzzleMode_VisibilityReverseEyeBeams, false);
		this->bShowBrokenWalls= playerSettings.GetVar(Settings::PuzzleMode_VisibilityBrokenWalls, false);
		this->bShowSecretWalls = playerSettings.GetVar(Settings::PuzzleMode_VisibilitySecretWalls, false);
		this->bShowSpiders = playerSettings.GetVar(Settings::PuzzleMode_VisibilitySpiders, true);
	}

	bool GetHideAnimations() const { return this->bIsEnabled && this->bHideAnimations; }
	bool GetHideBuildMarkers() const { return this->bIsEnabled && this->bHideBuildMarkers; }
	bool GetHideLighting() const { return this->bIsEnabled && this->bHideLighting; }
	bool GetHideWeather() const { return this->bIsEnabled && this->bHideWeather; }

	bool GetShowGrid() const { return this->bIsEnabled && this->uGridOpacity > 0; }

	bool GetShowEvilEyeBeams() const { return this->bIsEnabled && this->bShowEvilEyeBeams; }
	bool GetShowReverseEvilEyeBeams() const { return this->bIsEnabled && this->bShowReverseEvilEyeBeams; }
	bool GetShowBrokenWalls() const { return this->bIsEnabled && this->bShowBrokenWalls; }
	bool GetShowSecretWalls() const { return this->bIsEnabled && this->bShowSecretWalls; }
	bool GetShowSpiders() const { return this->bIsEnabled && this->bShowSpiders; }

	bool bIsEnabled;

	Uint8 uGridOpacity;

private:
	bool bShowEvilEyeBeams;
	bool bShowReverseEvilEyeBeams;
	bool bShowBrokenWalls;
	bool bShowSecretWalls;
	bool bShowSpiders;

	bool bHideAnimations;
	bool bHideBuildMarkers;
	bool bHideLighting;
	bool bHideWeather;
};

#endif //...#ifndef PUZZLEMODEOPTIONS_H
