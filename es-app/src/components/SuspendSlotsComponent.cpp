/**
  * Copyright (c) 2026 DefKorns (https://defkorns.github.io/LICENSE)
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  */
#include "components/SuspendSlotsComponent.h"

#include "ThemeData.h"

#include <utility>

namespace
{
	template <typename T, typename... Args>
	std::unique_ptr<T> makeUnique(Args&&... args)
	{
		return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
	}
}

SuspendSlotsComponent::SuspendSlotsComponent(Window* window) : GuiComponent(window)
{
	for(int i = 0; i < SuspendPoints::MaxSlots; ++i)
	{
		mIcons.push_back(makeUnique<ImageComponent>(window));
		mIcons.back()->setOrigin(0.5f, 0.5f);
		addChild(mIcons.back().get());
	}
	mSize = Vector2f(0.0f, 0.0f);
}

void SuspendSlotsComponent::setStates(const SlotStates& states)
{
	if(states == mStates)
		return;
	mStates = states;
	refreshImages();
}

void SuspendSlotsComponent::onSizeChanged()
{
	layout();
}

void SuspendSlotsComponent::layout()
{
	const float step = mSize.x() / static_cast<float>(mIcons.size());
	for(std::size_t i = 0; i < mIcons.size(); ++i)
	{
		mIcons[i]->setMaxSize(step, mSize.y());
		mIcons[i]->setPosition((static_cast<float>(i) + 0.5f) * step, mSize.y() * 0.5f);
	}
}

const std::string& SuspendSlotsComponent::pathFor(SlotState state) const
{
	switch(state)
	{
	case SlotState::Saved: return mSavedPath;
	case SlotState::Locked: return mLockedPath;
	default: return mEmptyPath;
	}
}

void SuspendSlotsComponent::refreshImages()
{
	for(std::size_t i = 0; i < mIcons.size(); ++i)
		mIcons[i]->setImage(pathFor(mStates[i]));
}

void SuspendSlotsComponent::applyTheme(const std::shared_ptr<ThemeData>& theme, const std::string& view, const std::string& element, unsigned int properties)
{
	GuiComponent::applyTheme(theme, view, element, properties);

	const ThemeData::ThemeElement* elem = theme->getElement(view, element, "slots");
	if(!elem)
		return;

	using namespace ThemeFlags;
	if(properties & PATH)
	{
		if(elem->has("emptyPath"))
			mEmptyPath = elem->get<std::string>("emptyPath");
		if(elem->has("savedPath"))
			mSavedPath = elem->get<std::string>("savedPath");
		if(elem->has("lockedPath"))
			mLockedPath = elem->get<std::string>("lockedPath");
		refreshImages();
	}
	layout();
}
