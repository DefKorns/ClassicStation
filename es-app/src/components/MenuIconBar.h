/**
  * Copyright (c) 2026 DefKorns (https://defkorns.github.io/LICENSE)
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  */
#pragma once
#ifndef ES_APP_COMPONENTS_MENU_ICON_BAR_H
#define ES_APP_COMPONENTS_MENU_ICON_BAR_H

#include "GuiComponent.h"
#include "components/ImageComponent.h"
#include "components/NinePatchComponent.h"
#include "components/TextComponent.h"
#include "math/Vector2f.h"
#include "math/Vector4f.h"
#include "ThemeData.h"
#include <memory>
#include <vector>

class MenuIconBar : public GuiComponent
{
public:
	static constexpr int Count = 6;

	explicit MenuIconBar(Window* window);
	~MenuIconBar() override;

	int selected() const { return mSelected; }
	void setSelected(int index);
	void setFrameVisible(bool visible);
	void setLifted(bool lifted, bool animate);
	void flyFromCard(const Vector4f& card);
	void flyToCard(const Vector4f& card);
	bool flying() const { return mFlying; }
	bool shown() const { return mThemed; }
	void applyTheme(const std::shared_ptr<ThemeData>& theme, const std::string& view);

	void update(int deltaTime) override;
	void render(const Transform4x4f& parentTrans) override;

private:
	void layout();
	void retarget(bool snap);
	bool animated() const;
	float scaleOf(std::size_t index) const;
	float frameCentre() const;
	float captionScale() const;
	float liftNow() const;
	static void publishLift();

	std::vector<std::unique_ptr<ImageComponent>> mIcons;
	NinePatchComponent mFrame;
	NinePatchComponent mFlyFrame;
	NinePatchComponent mCaption;
	ImageComponent mCaptionArrow;
	TextComponent mCaptionLabel;
	float mScaleFrom[Count] = {};
	float mScaleTo[Count] = {};
	int mScaleMs[Count] = {};
	float mLiftFrom = 0.0f;
	float mLiftTo = 0.0f;
	int mLiftMs = 0;
	float mFrameFrom = 0.0f;
	float mFrameTo = 0.0f;
	int mFrameMs = 0;
	int mPulseMs = 0;
	int mCaptionMs = 0;
	float mLeft = 0.0f;
	float mStep = 0.0f;
	float mCenterY = 0.0f;
	float mSize = 0.0f;
	int mSelected = 0;
	int mCaptionIndex = -1;
	int mCaptionPx = 0;
	bool mFrameVisible = false;
	bool mLifted = false;
	bool mFlying = false;
	bool mFlyToIcon = false;
	int mFlyMs = 0;
	Vector4f mFlyCard;
	bool mThemed = false;
	Vector2f mBoxPos;
	Vector2f mBoxSize;

	static std::vector<MenuIconBar*> sBars;
};

#endif // ES_APP_COMPONENTS_MENU_ICON_BAR_H
