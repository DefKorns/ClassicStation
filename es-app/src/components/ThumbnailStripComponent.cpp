/**
  * Copyright (c) 2026 DefKorns (https://defkorns.github.io/LICENSE)
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  */
#include "components/ThumbnailStripComponent.h"

#include "renderers/Renderer.h"

#include <algorithm>
#include <cmath>

namespace
{
	constexpr float Gap = 3.0f;
	constexpr int ArrowFrameMs = 150;
	constexpr int FadeSlots = 3;
	const char* const ArrowNames[] = { "thumbnail_arrow_a", "thumbnail_arrow_b", "thumbnail_arrow_c" };
}

ThumbnailStripComponent::ThumbnailStripComponent(Window* window) : GuiComponent(window)
{
	for(int slot = 0; slot < Slots; ++slot)
	{
		mImages.emplace_back(new ImageComponent(window, false));
		mImages.back()->setOrigin(0.5f, 1.0f);
		mAssigned.push_back(-1);
	}
	for(int frame = 0; frame < ArrowFrames; ++frame)
		mArrow.emplace_back(new ImageComponent(window));
	mSize = Vector2f(static_cast<float>(Renderer::getScreenWidth()), static_cast<float>(Renderer::getScreenHeight()));
}

void ThumbnailStripComponent::applyTheme(const std::shared_ptr<ThemeData>& theme, const std::string& view)
{
	const ThemeData::ThemeElement* strip = theme->getElement(view, "thumbnail_strip", "image");
	mEnabled = strip != nullptr && strip->has("pos") && strip->has("size");
	if(!mEnabled)
		return;

	const Vector2f screen(static_cast<float>(Renderer::getScreenWidth()), static_cast<float>(Renderer::getScreenHeight()));
	mCentre = strip->get<Vector2f>("pos") * screen;
	mThumbSize = strip->get<Vector2f>("size") * screen;
	mStep = mThumbSize.x() + Gap;

	for(int frame = 0; frame < ArrowFrames; ++frame)
		mArrow[frame]->applyTheme(theme, view, ArrowNames[frame], ThemeFlags::ALL);
	for(auto& assigned : mAssigned)
		assigned = -1;
}

void ThumbnailStripComponent::setEntries(std::vector<std::string> paths)
{
	mPaths = std::move(paths);
	for(auto& assigned : mAssigned)
		assigned = -1;
	mPosition = 0.0f;
}

void ThumbnailStripComponent::setPosition(float index)
{
	const float count = static_cast<float>(mPaths.size());
	if(count == 0.0f)
		return;
	mPosition = std::fmod(index, count);
	if(mPosition < 0.0f)
		mPosition += count;
}

void ThumbnailStripComponent::update(int deltaTime)
{
	GuiComponent::update(deltaTime);
	mElapsedMs += deltaTime;
}

void ThumbnailStripComponent::render(const Transform4x4f& parentTrans)
{
	const int count = static_cast<int>(mPaths.size());
	if(!mEnabled || count == 0)
		return;

	const Transform4x4f trans = parentTrans * getTransform();
	const int base = static_cast<int>(std::floor(mPosition + 0.5f));
	const int reach = std::min(Reach, (count - 1) / 2);
	const float bottom = mCentre.y() + mThumbSize.y() / 2.0f;

	for(int offset = -reach; offset <= reach; ++offset)
	{
		const int index = base + offset;
		const int wrapped = ((index % count) + count) % count;
		const int slot = ((index % Slots) + Slots) % Slots;

		ImageComponent& image = *mImages[slot];
		if(mAssigned[slot] != wrapped)
		{
			image.setMaxSize(mThumbSize.x(), mThumbSize.y());
			image.setImage(mPaths[wrapped]);
			mAssigned[slot] = wrapped;
		}

		const float distance = static_cast<float>(index) - mPosition;
		const float fade = std::max(0.0f, std::min(1.0f, (static_cast<float>(Reach) - std::fabs(distance)) / static_cast<float>(FadeSlots)));
		image.setOpacity(static_cast<unsigned char>(255.0f * fade));
		image.setPosition(mCentre.x() + distance * mStep, bottom);
		image.render(trans);
	}

	ImageComponent& arrow = *mArrow[(mElapsedMs / ArrowFrameMs) % ArrowFrames];
	arrow.render(trans);
}
