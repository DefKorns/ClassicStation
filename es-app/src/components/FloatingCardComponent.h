/**
  * Copyright (c) 2026 DefKorns (https://defkorns.github.io/LICENSE)
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  */
#pragma once
#ifndef ES_APP_COMPONENTS_FLOATING_CARD_COMPONENT_H
#define ES_APP_COMPONENTS_FLOATING_CARD_COMPONENT_H

#include "components/ImageComponent.h"
#include "GuiComponent.h"
#include "ThemeData.h"

#include <memory>
#include <string>
#include <vector>

class FloatingCardComponent : public GuiComponent
{
public:
	explicit FloatingCardComponent(Window* window);

	void applyTheme(const std::shared_ptr<ThemeData>& theme, const std::string& view);

	void setScreenshot(const std::string& path);
	void setShown(bool shown) { mShown = shown; }
	void setHighlighted(bool highlighted) { mHighlighted = highlighted; }
	void erase();

	void update(int deltaTime) override;
	void render(const Transform4x4f& parentTrans) override;

private:
	static constexpr int SmokeFrames = 5;

	enum Part
	{
		CardLarge, CardLargeActive, ScreenLarge, FrameLarge,
		WingLargeLeftA, WingLargeLeftB, WingLargeRightA, WingLargeRightB, ArrowA, ArrowB,
		CardSmall, ScreenSmall, FrameSmall, IconBack,
		WingSmallLeftA, WingSmallLeftB, WingSmallRightA, WingSmallRightB,
		SmokeLarge, SmokeSmall = SmokeLarge + SmokeFrames, PartCount = SmokeSmall + SmokeFrames
	};

	ImageComponent& part(Part which) { return *mParts[which]; }

	std::vector<std::unique_ptr<ImageComponent>> mParts;
	bool mShown = false;
	bool mHighlighted = false;
	int mElapsedMs = 0;
	int mEraseMs = -1;
	bool mEraseLarge = false;
};

#endif // ES_APP_COMPONENTS_FLOATING_CARD_COMPONENT_H
