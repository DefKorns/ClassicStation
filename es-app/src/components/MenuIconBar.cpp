/**
  * Copyright (c) 2026 DefKorns (https://defkorns.github.io/LICENSE)
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  */
#include "components/MenuIconBar.h"

#include "renderers/Renderer.h"
#include "resources/Font.h"
#include "resources/ResourceManager.h"
#include "Settings.h"

#include <algorithm>
#include <cmath>

namespace
{
	const char* const IconPaths[] = { ":/menu_display.svg", ":/menu_sound.svg", ":/menu_library.svg", ":/menu_network.svg", ":/menu_system.svg", ":/menu_help.svg" };
	const char* const Names[] = { "Display", "Sound", "Library", "Network", "Tools", "System" };
	constexpr float IconSizeRatio = 48.0f / 720.0f;
	constexpr float IconStepRatio = 0.118f;
	constexpr float IconCenterRatio = 0.075f;
	constexpr float FirstCenterRatio = 0.398f;
	constexpr float FrameBleed = 4.0f;
	constexpr float ThemedFrameBleed = 16.0f;
	constexpr float SelectedIconScale = 1.2f;
	constexpr float FrameHalfWidth = 0.8f;
	constexpr float NeighbourHalfWidth = 0.5f;
	constexpr float NeighbourGap = 4.0f;
	constexpr int FrameCorner = 10;
	constexpr int ThemedFrameCorner = 32;
	constexpr int ScaleMs = 200;
	constexpr int LiftMs = 200;
	constexpr int FlyMs = 300;
	constexpr int FlyReturnMs = 52;
	constexpr float LiftRatio = 5.0f / 720.0f;
	constexpr int SlideMs = 300;
	constexpr int CaptionDelayMs = 250;
	constexpr int CaptionGrowMs = 200;
	constexpr int PulseMs = 2250;
	constexpr float CaptionHeight = 66.0f;
	constexpr float CaptionMinWidth = 144.0f;
	constexpr float CaptionEdges = 24.0f;
	constexpr float CaptionOffset = 60.0f;
	constexpr float ArrowOffset = 36.0f;
	constexpr float LabelOffset = 66.0f;

	float outExpo(float progress)
	{
		return progress >= 1.0f ? 1.0f : 1.0f - std::pow(2.0f, -10.0f * progress);
	}

	float eased(float from, float to, int elapsed, int duration)
	{
		return from + (to - from) * outExpo(std::min(1.0f, static_cast<float>(elapsed) / static_cast<float>(duration)));
	}

	unsigned int pulseColour(int elapsed)
	{
		const float white = 255.0f;
		const float dim = 191.0f;
		float red = white;
		if(elapsed < 500)
			red = dim + (white - dim) * static_cast<float>(elapsed) / 500.0f;
		else if(elapsed >= 1250 && elapsed < 2000)
			red = white - (white - dim) * static_cast<float>(elapsed - 1250) / 750.0f;
		else if(elapsed >= 2000)
			red = dim;
		const unsigned int channel = static_cast<unsigned int>(red + 0.5f);
		return (channel << 24) | (channel << 16) | 0xFF00u | 0xFFu;
	}
}

std::vector<MenuIconBar*> MenuIconBar::sBars;

MenuIconBar::MenuIconBar(Window* window)
	: GuiComponent(window), mFrame(window, ":/menu_selected.png"), mFlyFrame(window, ":/menu_selected.png"), mCaption(window, ":/menu_caption.png"), mCaptionArrow(window), mCaptionLabel(window)
{
	mFrame.setCornerSize(FrameCorner, FrameCorner);
	mCaption.setCornerSize(12, 12);
	mCaptionArrow.setImage(":/menu_caption_arrow.png");
	mCaptionArrow.setOrigin(0.5f, 0.5f);
	mCaptionLabel.setHorizontalAlignment(ALIGN_CENTER);
	mCaptionLabel.setColor(0x000000FF);
	for (const char* path : IconPaths)
	{
		mIcons.emplace_back(new ImageComponent(window));
		mIcons.back()->setImage(ResourceManager::getInstance()->preferPng(path));
		mIcons.back()->setOrigin(0.5f, 0.5f);
	}
	for(int index = 0; index < Count; ++index)
	{
		mScaleFrom[index] = 1.0f;
		mScaleTo[index] = 1.0f;
		mScaleMs[index] = ScaleMs;
	}
	sBars.push_back(this);
	mLiftMs = LiftMs;
	layout();
	retarget(true);
}

MenuIconBar::~MenuIconBar()
{
	sBars.erase(std::remove(sBars.begin(), sBars.end(), this), sBars.end());
	publishLift();
}

bool MenuIconBar::animated() const
{
	return Settings::getInstance()->getBool("MenuAnimations");
}

float MenuIconBar::scaleOf(std::size_t index) const
{
	return animated() ? eased(mScaleFrom[index], mScaleTo[index], mScaleMs[index], ScaleMs) : mScaleTo[index];
}

float MenuIconBar::frameCentre() const
{
	return animated() ? eased(mFrameFrom, mFrameTo, mFrameMs, SlideMs) : mFrameTo;
}

void MenuIconBar::flyFromCard(const Vector4f& card)
{
	if(!mThemed || !animated())
		return;
	mFlyCard = card;
	mFlyToIcon = true;
	mFlyMs = 0;
	mFlying = true;
}

void MenuIconBar::flyToCard(const Vector4f& card)
{
	if(!mThemed || !animated())
		return;
	mFlyCard = card;
	mFlyToIcon = false;
	mFlyMs = 0;
	mFlying = true;
}

float MenuIconBar::liftNow() const
{
	if(!animated())
		return mLiftTo;
	return mLiftFrom + (mLiftTo - mLiftFrom) * std::min(1.0f, static_cast<float>(mLiftMs) / static_cast<float>(LiftMs));
}

void MenuIconBar::publishLift()
{
	float lift = 0.0f;
	for(const MenuIconBar* bar : sBars)
		if(bar->mThemed)
			lift = std::max(lift, bar->liftNow());
	ImageComponent::setHeaderLift(lift);
}

void MenuIconBar::setLifted(bool lifted, bool animate)
{
	mLifted = lifted;
	const float target = mThemed && lifted ? LiftRatio * static_cast<float>(Renderer::getScreenHeight()) : 0.0f;
	if(target == mLiftTo)
		return;
	mLiftFrom = animate ? liftNow() : target;
	mLiftTo = target;
	mLiftMs = animate ? 0 : LiftMs;
	layout();
	publishLift();
}

float MenuIconBar::captionScale() const
{
	if(!animated())
		return 1.0f;
	const int elapsed = mCaptionMs - CaptionDelayMs;
	return elapsed <= 0 ? 0.0f : outExpo(std::min(1.0f, static_cast<float>(elapsed) / static_cast<float>(CaptionGrowMs)));
}

void MenuIconBar::retarget(bool snap)
{
	for(std::size_t index = 0; index < mIcons.size(); ++index)
	{
		const float target = mThemed && mFrameVisible && static_cast<int>(index) == mSelected ? SelectedIconScale : 1.0f;
		if(target == mScaleTo[index])
			continue;
		mScaleFrom[index] = snap ? target : scaleOf(index);
		mScaleTo[index] = target;
		mScaleMs[index] = snap ? ScaleMs : 0;
	}

	const float centre = mLeft + mStep * (static_cast<float>(mSelected) + 0.5f);
	if(snap)
	{
		mFrameFrom = centre;
		mFrameTo = centre;
		mFrameMs = SlideMs;
	}
	else if(centre != mFrameTo)
	{
		mFrameFrom = frameCentre();
		mFrameTo = centre;
		mFrameMs = 0;
	}
	mCaptionMs = 0;
}

void MenuIconBar::setSelected(int index)
{
	mSelected = index;
	layout();
	retarget(false);
}

void MenuIconBar::setFrameVisible(bool visible)
{
	mFrameVisible = visible;
	retarget(visible);
	layout();
}

void MenuIconBar::applyTheme(const std::shared_ptr<ThemeData>& theme, const std::string& view)
{
	const ThemeData::ThemeElement* elem = theme->getElement(view, "menuIcons", "container");
	mThemed = elem != nullptr && elem->has("pos") && elem->has("size");
	if (mThemed)
	{
		const Vector2f screen(static_cast<float>(Renderer::getScreenWidth()), static_cast<float>(Renderer::getScreenHeight()));
		mBoxPos = elem->get<Vector2f>("pos") * screen;
		mBoxSize = elem->get<Vector2f>("size") * screen;
		mCaptionLabel.setColor(elem->has("captionColor") ? elem->get<unsigned int>("captionColor") : 0x000000FFu);
	}
	layout();
	retarget(true);
	setLifted(mLifted, false);
}

void MenuIconBar::layout()
{
	const float width = static_cast<float>(Renderer::getScreenWidth());
	const float height = static_cast<float>(Renderer::getScreenHeight());
	const float count = static_cast<float>(mIcons.size());
	mStep = mThemed ? mBoxSize.x() / count : height * IconStepRatio;
	const float fit = mThemed ? std::max(0.1f, std::min(1.0f, (mStep - NeighbourGap) / ((FrameHalfWidth + NeighbourHalfWidth) * mBoxSize.y() + ThemedFrameBleed))) : 1.0f;
	mSize = mThemed ? mBoxSize.y() * fit : height * IconSizeRatio;
	mLeft = mThemed ? mBoxPos.x() : width * FirstCenterRatio - mStep / 2;
	mCenterY = (mThemed ? mBoxPos.y() + mBoxSize.y() / 2 : height * IconCenterRatio) - liftNow();
	const float bleed = mThemed ? ThemedFrameBleed * fit : FrameBleed;

	for (std::size_t index = 0; index < mIcons.size(); ++index)
	{
		const float iconSize = mSize * scaleOf(index);
		mIcons[index]->setResize(iconSize, iconSize);
		mIcons[index]->setPosition(mLeft + mStep * (static_cast<float>(index) + 0.5f), mCenterY);
	}

	const float grow = scaleOf(static_cast<std::size_t>(mSelected));
	const float boxWidth = mThemed ? mSize * 1.3333f * grow : mStep * 0.9f;
	const float boxHeight = mThemed ? mSize * 0.9722f * grow : mStep * 0.8f;
	mFrame.setCornerSize(mThemed ? ThemedFrameCorner : FrameCorner, mThemed ? ThemedFrameCorner : FrameCorner);
	const float centreX = mThemed ? frameCentre() : mLeft + mStep * (static_cast<float>(mSelected) + 0.5f);
	mFrame.setSize(boxWidth + 2 * bleed, boxHeight + 2 * bleed);
	mFrame.setPosition(centreX - boxWidth / 2 - bleed, mCenterY - boxHeight / 2 - bleed);
}

void MenuIconBar::update(int deltaTime)
{
	GuiComponent::update(deltaTime);
	if(!mThemed)
		return;

	bool busy = false;
	for(int index = 0; index < Count; ++index)
	{
		if(mScaleMs[index] < ScaleMs)
		{
			mScaleMs[index] += deltaTime;
			busy = true;
		}
	}
	if(mFrameMs < SlideMs)
	{
		mFrameMs += deltaTime;
		busy = true;
	}
	if(mLiftMs < LiftMs)
	{
		mLiftMs += deltaTime;
		busy = true;
	}
	if(mFlying)
	{
		mFlyMs += deltaTime;
		if(mFlyMs >= FlyMs || (!mFlyToIcon && mFlyMs > FlyReturnMs))
			mFlying = false;
		busy = true;
	}
	if(mFrameVisible)
	{
		mCaptionMs += deltaTime;
		mPulseMs = (mPulseMs + deltaTime) % PulseMs;
		const unsigned int colour = animated() ? pulseColour(mPulseMs) : 0xFFFFFFFFu;
		mFrame.setCenterColor(colour);
		mFrame.setEdgeColor(colour);
	}
	if(busy)
	{
		layout();
		publishLift();
	}
}

void MenuIconBar::render(const Transform4x4f& parentTrans)
{
	if (!mThemed)
		return;

	if (mFlying)
	{
		const float time = std::min(1.0f, static_cast<float>(mFlyMs) / static_cast<float>(FlyMs));
		const float t = mFlyToIcon ? outExpo(time) : 1.0f - outExpo(time);
		const Vector3f iconPos = mFrame.getPosition();
		const Vector2f iconSize = mFrame.getSize();
		const float x = mFlyCard.x() + (iconPos.x() - mFlyCard.x()) * t;
		const float y = mFlyCard.y() + (iconPos.y() - mFlyCard.y()) * t;
		const float w = mFlyCard.z() + (iconSize.x() - mFlyCard.z()) * t;
		const float h = mFlyCard.w() + (iconSize.y() - mFlyCard.w()) * t;
		mFlyFrame.setEdgeColor(pulseColour(mPulseMs));
		mFlyFrame.setCornerSize(ThemedFrameCorner, ThemedFrameCorner);
		mFlyFrame.setSize(w, h);
		mFlyFrame.setPosition(x, y);
		mFlyFrame.render(parentTrans);
	}
	else if (mFrameVisible)
		mFrame.render(parentTrans);
	for (const auto& icon : mIcons)
		icon->render(parentTrans);

	const float scale = captionScale();
	if(!mFrameVisible || scale <= 0.0f)
		return;

	const float height = static_cast<float>(Renderer::getScreenHeight());
	const std::string name = Names[mSelected];
	const int captionPx = static_cast<int>(0.03f * height);
	if(mCaptionIndex != mSelected || mCaptionPx != captionPx)
	{
		mCaptionIndex = mSelected;
		mCaptionPx = captionPx;
		mCaptionLabel.setFont(Font::get(captionPx));
		mCaptionLabel.setText(name);
	}
	const float labelWidth = mCaptionLabel.getFont()->sizeText(name).x();
	const float total = std::max(CaptionMinWidth, labelWidth) + CaptionEdges;
	const float centreX = frameCentre();
	const float boxCentreY = mCenterY + CaptionOffset;

	mCaption.setSize(total, CaptionHeight);
	mCaption.setPosition(centreX - total / 2, boxCentreY - CaptionHeight / 2);
	mCaptionArrow.setPosition(centreX, mCenterY + ArrowOffset);
	mCaptionLabel.setSize(total, 0.0f);
	mCaptionLabel.setPosition(centreX - total / 2, mCenterY + LabelOffset - mCaptionLabel.getFont()->getHeight() / 2);

	Transform4x4f trans = parentTrans;
	trans.translate(Vector3f(centreX, boxCentreY, 0.0f));
	trans.scale(Vector3f(scale, scale, 1.0f));
	trans.translate(Vector3f(-centreX, -boxCentreY, 0.0f));
	mCaption.render(trans);
	mCaptionArrow.render(trans);
	mCaptionLabel.render(trans);
}
