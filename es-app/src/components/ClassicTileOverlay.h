/**
  * Copyright (c) 2026 DefKorns (https://defkorns.github.io/LICENSE)
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  */
#pragma once
#ifndef ES_APP_COMPONENTS_CLASSIC_TILE_OVERLAY_H
#define ES_APP_COMPONENTS_CLASSIC_TILE_OVERLAY_H

#include "components/GridTileOverlay.h"
#include "components/ImageComponent.h"
#include "components/PlayersIconComponent.h"
#include "components/SuspendSlotsComponent.h"
#include "SuspendSlotState.h"

#include <string>

class ClassicTileOverlay : public GridTileOverlay
{
public:
	explicit ClassicTileOverlay(Window* window);

	void setGame(const SlotStates& states, const std::string& players, bool hasSave);
	void setSelected(bool selected) override { mSelected = selected; }

	void applyTheme(const std::shared_ptr<ThemeData>& theme, const std::string& view, const std::string& element, unsigned int properties) override;
	void render(const Transform4x4f& parentTrans) override;

private:
	SuspendSlotsComponent mSlots;
	SuspendSlotsComponent mSlotsActive;
	PlayersIconComponent mPlayers;
	PlayersIconComponent mPlayersActive;
	ImageComponent mSaveIcon;
	bool mHasSave = false;
	bool mSelected = false;
};

#endif // ES_APP_COMPONENTS_CLASSIC_TILE_OVERLAY_H
