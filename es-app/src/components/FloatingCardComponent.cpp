/**
  * Copyright (c) 2026 DefKorns (https://defkorns.github.io/LICENSE)
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  */
#include "components/FloatingCardComponent.h"

#include "renderers/Renderer.h"

#include <cmath>
#include <utility>

namespace
{
	constexpr float PI = 3.14159265f;
	constexpr int BobPeriodMs = 1800;
	constexpr float BobPixels = 6.0f;
	constexpr int WingFrameMs = 180;
	constexpr int ArrowFrameMs = 450;
	constexpr int SmokeFrameMs = 70;

	const char* const PartNames[] = {
		"floating_card", "floating_card_active", "floating_screen", "floating_frame",
		"floating_wing_left_a", "floating_wing_left_b", "floating_wing_right_a", "floating_wing_right_b", "floating_arrow_a", "floating_arrow_b",
		"floating_card_small", "floating_screen_small", "floating_frame_small", "floating_icon_back",
		"floating_wing_small_left_a", "floating_wing_small_left_b", "floating_wing_small_right_a", "floating_wing_small_right_b",
		"floating_smoke_1", "floating_smoke_2", "floating_smoke_3", "floating_smoke_4", "floating_smoke_5",
		"floating_smoke_small_1", "floating_smoke_small_2", "floating_smoke_small_3", "floating_smoke_small_4", "floating_smoke_small_5"
	};
}

FloatingCardComponent::FloatingCardComponent(Window* window) : GuiComponent(window)
{
	for(int index = 0; index < PartCount; ++index)
		mParts.emplace_back(new ImageComponent(window));
	mSize = Vector2f(static_cast<float>(Renderer::getScreenWidth()), static_cast<float>(Renderer::getScreenHeight()));
}

void FloatingCardComponent::applyTheme(const std::shared_ptr<ThemeData>& theme, const std::string& view)
{
	for(int index = 0; index < PartCount; ++index)
		if(index != ScreenLarge && index != ScreenSmall)
			mParts[index]->applyTheme(theme, view, PartNames[index], ThemeFlags::ALL);
		else
			mParts[index]->applyTheme(theme, view, PartNames[index], ThemeFlags::ALL ^ ThemeFlags::PATH);
}

void FloatingCardComponent::setScreenshot(const std::string& path)
{
	part(ScreenLarge).setImage(path);
	part(ScreenSmall).setImage(path);
}

void FloatingCardComponent::erase()
{
	if(!mShown)
		return;
	mEraseLarge = mHighlighted;
	mEraseMs = 0;
	mShown = false;
}

void FloatingCardComponent::update(int deltaTime)
{
	GuiComponent::update(deltaTime);
	mElapsedMs += deltaTime;
	if(mEraseMs >= 0)
	{
		mEraseMs += deltaTime;
		if(mEraseMs >= SmokeFrames * SmokeFrameMs)
			mEraseMs = -1;
	}
}

void FloatingCardComponent::render(const Transform4x4f& parentTrans)
{
	if(mEraseMs >= 0)
	{
		const Transform4x4f smokeTrans = parentTrans * getTransform();
		part(static_cast<Part>((mEraseLarge ? SmokeLarge : SmokeSmall) + mEraseMs / SmokeFrameMs)).render(smokeTrans);
	}

	if(!mShown)
		return;

	const float bob = -BobPixels * std::sin(2.0f * PI * static_cast<float>(mElapsedMs % BobPeriodMs) / static_cast<float>(BobPeriodMs));
	const bool wingB = (mElapsedMs / WingFrameMs) % 2 == 1;
	const bool arrowB = (mElapsedMs / ArrowFrameMs) % 2 == 1;

	Transform4x4f trans = parentTrans * getTransform();
	trans.translate(Vector3f(0.0f, bob, 0.0f));

	if(mHighlighted)
	{
		part(CardLarge).render(trans);
		part(CardLargeActive).render(trans);
		part(ScreenLarge).render(trans);
		part(FrameLarge).render(trans);
		part(wingB ? WingLargeLeftB : WingLargeLeftA).render(trans);
		part(wingB ? WingLargeRightB : WingLargeRightA).render(trans);
		part(arrowB ? ArrowB : ArrowA).render(trans);
	}
	else
	{
		part(CardSmall).render(trans);
		part(ScreenSmall).render(trans);
		part(FrameSmall).render(trans);
		part(wingB ? WingSmallLeftB : WingSmallLeftA).render(trans);
		part(wingB ? WingSmallRightB : WingSmallRightA).render(trans);
		part(IconBack).render(trans);
	}
}
