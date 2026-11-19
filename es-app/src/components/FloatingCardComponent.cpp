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
#include "resources/Font.h"

#include <cmath>
#include <cstdio>
#include <utility>

namespace
{
	constexpr float PI = 3.14159265f;
	constexpr int BobPeriodMs = 1800;
	constexpr float BobPixels = 6.0f;
	constexpr int WingFrameMs = 180;
	constexpr int WrapMs = 900000;
	constexpr int SmokeFrameMs = 70;

	const char* const PartNames[] = {
		"floating_card", "floating_screen",
		"suspend_float_card", "suspend_float_active", "suspend_float_frame", "suspend_float_wing_a", "suspend_float_wing_b",
		"suspend_float_wing_left_a", "suspend_float_wing_left_b", "suspend_float_ring",
		"floating_card_small", "floating_screen_small", "floating_frame_small", "floating_icon_back", "floating_arrow_a", "floating_arrow_b",
		"floating_wing_small_left_a", "floating_wing_small_left_b", "floating_wing_small_right_a", "floating_wing_small_right_b",
		"floating_smoke_1", "floating_smoke_2", "floating_smoke_3", "floating_smoke_4", "floating_smoke_5",
		"floating_smoke_small_1", "floating_smoke_small_2", "floating_smoke_small_3", "floating_smoke_small_4", "floating_smoke_small_5"
	};
}

FloatingCardComponent::FloatingCardComponent(Window* window)
	: GuiComponent(window),
	  mTime(window, "", Font::get(FONT_SIZE_SMALL), 0xFFFFFFFF, ALIGN_RIGHT)
{
	for(int index = 0; index < PartCount; ++index)
		mParts.emplace_back(new ImageComponent(window));
	mSize = Vector2f(static_cast<float>(Renderer::getScreenWidth()), static_cast<float>(Renderer::getScreenHeight()));
}

void FloatingCardComponent::applyTheme(const std::shared_ptr<ThemeData>& theme, const std::string& view)
{
	for(int index = 0; index < PartCount; ++index)
	{
		if(index >= HiCard && index <= HiRing)
			mParts[index]->applyTheme(theme, view, PartNames[index], ThemeFlags::PATH);
		else if(index != ScreenLarge && index != ScreenSmall)
			mParts[index]->applyTheme(theme, view, PartNames[index], ThemeFlags::ALL);
		else
			mParts[index]->applyTheme(theme, view, PartNames[index], ThemeFlags::ALL ^ ThemeFlags::PATH);
	}
	constexpr unsigned int textProperties = ThemeFlags::COLOR | ThemeFlags::FONT_PATH | ThemeFlags::FONT_SIZE;
	mTime.applyTheme(theme, view, "suspend_float_time", textProperties);
}

void FloatingCardComponent::setPlaytime(long seconds)
{
	char text[32];
	std::snprintf(text, sizeof text, "%ld:%02ld", seconds / 60, seconds % 60);
	mTime.setText(text);
}

void FloatingCardComponent::renderHighlighted(const Transform4x4f& trans, bool wingB)
{
	const Vector3f centre = part(CardLarge).getPosition();
	const auto draw = [&](ImageComponent& image, float width, float height, float dx, float dy) {
		image.setOrigin(0.5f, 0.5f);
		image.setResize(width, height);
		image.setPosition(centre.x() + dx, centre.y() + dy);
		image.render(trans);
	};

	draw(part(HiCard), 204.0f, 180.0f, 0.0f, 0.0f);
	draw(part(HiActive), 192.0f, 168.0f, 0.0f, 0.0f);
	draw(part(HiRing), 224.0f, 200.0f, 0.0f, 0.0f);
	draw(part(ScreenLarge), 160.0f, 120.0f, 0.0f, -10.0f);
	draw(part(HiFrame), 160.0f, 120.0f, 0.0f, -10.0f);
	draw(part(wingB ? HiWingLeftB : HiWingLeftA), 72.0f, 96.0f, -129.0f, -72.0f);
	draw(part(wingB ? HiWingRightB : HiWingRightA), 72.0f, 96.0f, 129.0f, -72.0f);

	const int phase = mElapsedMs % 1000;
	ImageComponent& dpad = part(phase >= 17 && phase < 517 ? DownB : DownA);
	dpad.setOrigin(0.5f, 0.5f);
	dpad.setPosition(centre.x(), centre.y() + 90.0f);
	dpad.render(trans);

	mTime.setSize(90.0f, 30.0f);
	mTime.setPosition(centre.x() - 6.0f, centre.y() + 52.0f);
	mTime.render(trans);
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
	mElapsedMs = (mElapsedMs + deltaTime) % WrapMs;
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

	Transform4x4f trans = parentTrans * getTransform();
	trans.translate(Vector3f(0.0f, bob, 0.0f));

	if(mHighlighted)
		renderHighlighted(trans, wingB);
	else
	{
		part(CardSmall).render(trans);
		part(ScreenSmall).render(trans);
		part(FrameSmall).render(trans);
		part(wingB ? WingSmallLeftB : WingSmallLeftA).render(trans);
		part(wingB ? WingSmallRightB : WingSmallRightA).render(trans);
		const int phase = mElapsedMs % 1000;
		ImageComponent& dpad = part(phase >= 17 && phase < 517 ? DownB : DownA);
		dpad.setOrigin(0.5f, 0.5f);
		dpad.setPosition(part(IconBack).getPosition());
		dpad.render(trans);
	}
}
