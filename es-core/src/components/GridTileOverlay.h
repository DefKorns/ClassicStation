/**
  * Copyright (c) 2026 DefKorns (https://defkorns.github.io/LICENSE)
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  */
#pragma once
#ifndef ES_CORE_COMPONENTS_GRID_TILE_OVERLAY_H
#define ES_CORE_COMPONENTS_GRID_TILE_OVERLAY_H

#include "GuiComponent.h"

class GridTileOverlay : public GuiComponent
{
public:
	explicit GridTileOverlay(Window* window) : GuiComponent(window) {}

	virtual void setSelected(bool /*selected*/) {}
};

#endif // ES_CORE_COMPONENTS_GRID_TILE_OVERLAY_H
