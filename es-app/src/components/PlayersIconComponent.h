/**
  * Copyright (c) 2026 DefKorns (https://defkorns.github.io/LICENSE)
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  */
#pragma once
#ifndef ES_APP_COMPONENTS_PLAYERS_ICON_COMPONENT_H
#define ES_APP_COMPONENTS_PLAYERS_ICON_COMPONENT_H

#include "components/ImageComponent.h"
#include "GuiComponent.h"
#include <memory>
#include <string>

class PlayersIconComponent : public GuiComponent
{
public:
	explicit PlayersIconComponent(Window* window);

	void setPlayers(const std::string& players);

	void onSizeChanged() override;
	void applyTheme(const std::shared_ptr<ThemeData>& theme, const std::string& view, const std::string& element, unsigned int properties) override;

private:
	void layout();
	void refreshImage();

	ImageComponent mIcon;
	std::string mPlayers;
	std::string mOnePath;
	std::string mManyPath;
};

#endif // ES_APP_COMPONENTS_PLAYERS_ICON_COMPONENT_H
