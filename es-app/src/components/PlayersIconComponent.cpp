/**
  * Copyright (c) 2026 DefKorns (https://defkorns.github.io/LICENSE)
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  */
#include "components/PlayersIconComponent.h"

#include "ThemeData.h"

PlayersIconComponent::PlayersIconComponent(Window* window) : GuiComponent(window), mIcon(window)
{
	mIcon.setOrigin(0.0f, 0.0f);
	addChild(&mIcon);
	mSize = Vector2f(0.0f, 0.0f);
}

void PlayersIconComponent::setPlayers(const std::string& players)
{
	if(players == mPlayers)
		return;
	mPlayers = players;
	refreshImage();
}

void PlayersIconComponent::onSizeChanged()
{
	layout();
}

void PlayersIconComponent::layout()
{
	mIcon.setPosition(0.0f, 0.0f);
	mIcon.setResize(mSize.x(), mSize.y());
}

void PlayersIconComponent::refreshImage()
{
	if(mPlayers.empty() || mPlayers == "0")
		mIcon.setImage("");
	else
		mIcon.setImage(mPlayers == "1" ? mOnePath : mManyPath);
}

void PlayersIconComponent::applyTheme(const std::shared_ptr<ThemeData>& theme, const std::string& view, const std::string& element, unsigned int properties)
{
	GuiComponent::applyTheme(theme, view, element, properties);

	const ThemeData::ThemeElement* elem = theme->getElement(view, element, "playersicon");
	if(!elem)
		return;

	using namespace ThemeFlags;
	if(properties & PATH)
	{
		if(elem->has("onePath"))
			mOnePath = elem->get<std::string>("onePath");
		if(elem->has("manyPath"))
			mManyPath = elem->get<std::string>("manyPath");
		refreshImage();
	}
	layout();
}
