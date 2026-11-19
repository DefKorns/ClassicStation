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
	constexpr int ArrowFrameMs = 33;
	constexpr int ArrowSequence[] = { 0, 1, 1, 1, 2, 2, 2, 1, 1, 1, 0, 0, 0 };
	constexpr int ArrowSequenceLength = sizeof(ArrowSequence) / sizeof(ArrowSequence[0]);
	constexpr int ArrowWrapMs = ArrowFrameMs * ArrowSequenceLength * 1000;
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
	mTotalDirty = true;
	if(strip->has("visibleCount"))
		mFullCount = std::max(1, static_cast<int>(strip->get<float>("visibleCount")));

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
	mWidths.assign(mPaths.size(), 0.0f);
	mTotalDirty = true;
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
	mElapsedMs = (mElapsedMs + deltaTime) % ArrowWrapMs;
	for(auto& image : mImages)
		image->update(deltaTime);
}

void ThumbnailStripComponent::render(const Transform4x4f& parentTrans)
{
	const int count = static_cast<int>(mPaths.size());
	if(!mEnabled || count == 0)
		return;

	const Transform4x4f trans = demoLift(parentTrans) * getTransform();
	const int base = static_cast<int>(std::floor(mPosition + 0.5f));
	const int half = count / 2;
	const int reach = std::min(half, MaxReach);
	const float bottom = mCentre.y() + mThumbSize.y() / 2.0f;
	const float limit = std::min(static_cast<float>(mFullCount / 2) * mThumbSize.x(), static_cast<float>(MaxReach) * mThumbSize.x() / 2.0f);
	const float fadeSpan = static_cast<float>(FadeItems) * mThumbSize.x();

	const auto wrap = [&](int offset) { return (((base + offset) % count) + count) % count; };
	const auto slotOf = [&](int offset) { return (((base + offset) % Slots) + Slots) % Slots; };
	const auto widthOf = [&](int offset) {
		const float width = mWidths[static_cast<std::size_t>(wrap(offset))];
		return width > 0.0f ? width : mThumbSize.x();
	};

	std::vector<float>& centre = mCentres;
	centre.assign(static_cast<std::size_t>(2 * reach + 1), 0.0f);
	for(int offset = 1; offset <= reach; ++offset)
	{
		centre[static_cast<std::size_t>(reach + offset)] = centre[static_cast<std::size_t>(reach + offset - 1)] + (widthOf(offset - 1) + widthOf(offset)) / 2.0f;
		centre[static_cast<std::size_t>(reach - offset)] = centre[static_cast<std::size_t>(reach - offset + 1)] - (widthOf(-offset + 1) + widthOf(-offset)) / 2.0f;
	}
	const float shift = mPosition - static_cast<float>(base);
	float origin = 0.0f;
	if(reach >= 1)
		origin = shift >= 0.0f ? shift * centre[static_cast<std::size_t>(reach + 1)] : -shift * centre[static_cast<std::size_t>(reach - 1)];

	if(mTotalDirty)
	{
		mTotalWidth = 0.0f;
		for(int index = 0; index < count; ++index)
			mTotalWidth += mWidths[static_cast<std::size_t>(index)] > 0.0f ? mWidths[static_cast<std::size_t>(index)] : mThumbSize.x();
		mTotalDirty = false;
	}
	const float total = mTotalWidth;
	const bool fits = reach >= half && total <= 2.0f * limit;

	for(int offset = -reach; offset <= reach; ++offset)
	{
		if(offset == -half && count == 2 * half)
			continue;
		const float middle = centre[static_cast<std::size_t>(reach + offset)] - origin;
		const float inner = std::fabs(middle) - widthOf(offset) / 2.0f;
		if(!fits && inner >= limit)
			continue;

		const int wrapped = wrap(offset);
		const int slot = slotOf(offset);
		ImageComponent& image = *mImages[slot];
		if(mAssigned[slot] != wrapped)
		{
			image.setMaxSize(mThumbSize.x(), mThumbSize.y());
			image.setImageAsync(mPaths[wrapped], false, true);
			mAssigned[slot] = wrapped;
		}
		if(image.getSize().x() > 0.0f && mWidths[static_cast<std::size_t>(wrapped)] != image.getSize().x())
		{
			mWidths[static_cast<std::size_t>(wrapped)] = image.getSize().x();
			mTotalDirty = true;
		}

		const float fade = fits ? 1.0f : std::max(0.0f, std::min(1.0f, (limit - inner) / fadeSpan));
		image.setOpacity(static_cast<unsigned char>(255.0f * fade));
		image.setPosition(mCentre.x() + middle, bottom);
		image.render(trans);
	}

	ImageComponent& arrow = *mArrow[ArrowSequence[(mElapsedMs / ArrowFrameMs) % ArrowSequenceLength]];
	arrow.render(trans);
}
