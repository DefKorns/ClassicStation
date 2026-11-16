/**
  * Copyright (c) 2026 DefKorns (https://defkorns.github.io/LICENSE)
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  */
#pragma once
#ifndef ES_APP_GUIS_GUI_FRAME_PICKER_H
#define ES_APP_GUIS_GUI_FRAME_PICKER_H

#include "components/ImageComponent.h"
#include "components/NinePatchComponent.h"
#include "components/TextComponent.h"
#include "GuiComponent.h"
#include <functional>
#include <string>
#include <vector>

class GuiFramePicker : public GuiComponent
{
public:
	struct Item
	{
		std::string label;
		std::string image;
	};

	GuiFramePicker(Window* window, std::vector<std::string> frames, const std::string& current,
		std::string previewSuffix, std::function<void(const std::string&)> onPick);
	GuiFramePicker(Window* window, std::vector<Item> items, size_t current, int imageWidth, int imageHeight,
		std::function<void(size_t)> onPick);

	bool input(InputConfig* config, Input input) override;
	std::vector<HelpPrompt> getHelpPrompts() override;

private:
	void show();
	void step(int direction);
	void pick();
	size_t count() const;

	std::vector<std::string> mFrames;
	std::string mPreviewSuffix;
	std::function<void(const std::string&)> mOnPick;
	std::vector<Item> mItems;
	std::function<void(size_t)> mOnPickIndex;
	bool mExactSize = false;
	size_t mIndex = 0;
	float mImageWidth = 0.0f;
	float mImageHeight = 0.0f;

	NinePatchComponent mPanel;
	ImageComponent mPreview;
	ImageComponent mLeftArrow;
	ImageComponent mRightArrow;
	TextComponent mName;
	TextComponent mCounter;
};

#endif // ES_APP_GUIS_GUI_FRAME_PICKER_H
