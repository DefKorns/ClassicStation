/**
  * Copyright (c) 2026 DefKorns (https://defkorns.github.io/LICENSE)
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  */
#include "components/ClassicTileOverlay.h"

#include "ThemeData.h"

ClassicTileOverlay::ClassicTileOverlay(Window* window)
	: GridTileOverlay(window), mSlots(window), mSlotsActive(window), mPlayers(window), mPlayersActive(window)
{
}

void ClassicTileOverlay::setGame(const SlotStates& states, const std::string& players)
{
	mSlots.setStates(states);
	mSlotsActive.setStates(states);
	mPlayers.setPlayers(players);
	mPlayersActive.setPlayers(players);
}

void ClassicTileOverlay::applyTheme(const std::shared_ptr<ThemeData>& theme, const std::string& view, const std::string& /*element*/, unsigned int properties)
{
	mSlots.applyTheme(theme, view, "tile_slots", properties);
	mSlotsActive.applyTheme(theme, view, "tile_slots_active", properties);
	mPlayers.applyTheme(theme, view, "tile_players", properties);
	mPlayersActive.applyTheme(theme, view, "tile_players_active", properties);
}

void ClassicTileOverlay::render(const Transform4x4f& parentTrans)
{
	const Transform4x4f trans = parentTrans * getTransform();
	if(mSelected)
	{
		mSlotsActive.render(trans);
		mPlayersActive.render(trans);
	}
	else
	{
		mSlots.render(trans);
		mPlayers.render(trans);
	}
}
