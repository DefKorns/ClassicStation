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
	GuiFramePicker(Window* window, std::vector<std::string> frames, const std::string& current,
		std::string previewSuffix, std::function<void(const std::string&)> onPick);

	bool input(InputConfig* config, Input input) override;
	std::vector<HelpPrompt> getHelpPrompts() override;

private:
	void show();
	void step(int direction);

	std::vector<std::string> mFrames;
	std::string mPreviewSuffix;
	std::function<void(const std::string&)> mOnPick;
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
