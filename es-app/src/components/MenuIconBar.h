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
#include "ThemeData.h"
#include <memory>
#include <vector>

class MenuIconBar : public GuiComponent
{
public:
	static constexpr int Count = 5;

	explicit MenuIconBar(Window* window);

	int selected() const { return mSelected; }
	void setSelected(int index);
	void setFrameVisible(bool visible);
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

	std::vector<std::unique_ptr<ImageComponent>> mIcons;
	NinePatchComponent mFrame;
	NinePatchComponent mCaption;
	ImageComponent mCaptionArrow;
	TextComponent mCaptionLabel;
	float mScaleFrom[Count] = {};
	float mScaleTo[Count] = {};
	int mScaleMs[Count] = {};
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
	bool mFrameVisible = false;
	bool mThemed = false;
	Vector2f mBoxPos;
	Vector2f mBoxSize;
};

#endif // ES_APP_COMPONENTS_MENU_ICON_BAR_H
