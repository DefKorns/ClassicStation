/**
  * Copyright (c) 2026 DefKorns (https://defkorns.github.io/LICENSE)
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  */
#include "guis/GuiFramePicker.h"

#include "InputConfig.h"
#include "MenuColors.h"
#include "renderers/Renderer.h"
#include "resources/Font.h"
#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"
#include <algorithm>
#include <utility>

namespace
{
	const char* const FramesDir = "/usr/share/backgrounds";
}

GuiFramePicker::GuiFramePicker(Window* window, std::vector<std::string> frames, const std::string& current,
	std::string previewSuffix, std::function<void(const std::string&)> onPick) : GuiComponent(window),
	mFrames(std::move(frames)), mPreviewSuffix(std::move(previewSuffix)), mOnPick(std::move(onPick)),
	mPanel(window, ":/frame.png"), mPreview(window), mLeftArrow(window), mRightArrow(window), mName(window), mCounter(window)
{
	const auto found = std::find(mFrames.begin(), mFrames.end(), current);
	if (found != mFrames.end())
		mIndex = static_cast<size_t>(found - mFrames.begin());

	const float screenWidth = static_cast<float>(Renderer::getScreenWidth());
	const float screenHeight = static_cast<float>(Renderer::getScreenHeight());
	const float width = screenWidth * 0.6f;
	const float margin = width * 0.05f;
	const float textWidth = width - 2 * margin;
	mLeftArrow.setImage(":/option_arrow.svg");
	mLeftArrow.setFlipX(true);
	mRightArrow.setImage(":/option_arrow.svg");
	for (ImageComponent* arrow : { &mLeftArrow, &mRightArrow })
	{
		arrow->setResize(0, screenHeight * 0.06f);
		arrow->setColorShift(MenuColors::Icon);
	}
	const float arrowSpace = mLeftArrow.getSize().x() * 1.6f;
	mImageWidth = textWidth - 2 * arrowSpace;
	mImageHeight = mImageWidth * 9.0f / 16.0f;

	mPanel.setCenterColor(MenuColors::Panel);
	mPanel.setEdgeColor(MenuColors::Panel);

	mPreview.setOrigin(0.5f, 0.0f);

	mName.setFont(Font::get(FONT_SIZE_MENU));
	mName.setColor(MenuColors::Title);
	mName.setHorizontalAlignment(ALIGN_CENTER);
	mName.setSize(textWidth, 0.0f);

	mCounter.setFont(Font::get(FONT_SIZE_SMALL));
	mCounter.setColor(MenuColors::Dim);
	mCounter.setHorizontalAlignment(ALIGN_CENTER);
	mCounter.setSize(textWidth, 0.0f);

	const float gap = screenHeight * 0.015f;
	const float height = margin + mImageHeight + gap + mName.getFont()->getHeight() + mCounter.getFont()->getHeight() + margin;

	setSize(width, height);
	setPosition((screenWidth - width) / 2.0f, (screenHeight - height) / 2.0f);
	mPanel.fitTo(mSize, Vector3f::Zero(), Vector2f(-32, -32));

	mPreview.setPosition(width / 2.0f, margin);
	const float arrowY = margin + (mImageHeight - mLeftArrow.getSize().y()) / 2.0f;
	mLeftArrow.setPosition(margin, arrowY);
	mRightArrow.setPosition(width - margin - mRightArrow.getSize().x(), arrowY);
	mName.setPosition(margin, margin + mImageHeight + gap);
	mCounter.setPosition(margin, margin + mImageHeight + gap + mName.getFont()->getHeight());

	addChild(&mPanel);
	addChild(&mPreview);
	addChild(&mLeftArrow);
	addChild(&mRightArrow);
	addChild(&mName);
	addChild(&mCounter);
	show();
}

void GuiFramePicker::show()
{
	const std::string& frame = mFrames[mIndex];
	const std::string path = std::string(FramesDir) + "/" + frame + "/" + frame + "_" + mPreviewSuffix + "_preview.png";
	if (!frame.empty() && Utils::FileSystem::exists(path))
	{
		mPreview.setImage(path);
		mPreview.setMaxSize(mImageWidth, mImageHeight);
	}
	else
		mPreview.setImage("");
	mName.setText(frame.empty() ? "NONE" : Utils::String::toUpper(frame));
	mCounter.setText(std::to_string(mIndex + 1) + " / " + std::to_string(mFrames.size()));
}

void GuiFramePicker::step(int direction)
{
	const size_t count = mFrames.size();
	mIndex = (mIndex + (direction > 0 ? 1 : count - 1)) % count;
	show();
}

bool GuiFramePicker::input(InputConfig* config, Input input)
{
	if (input.value == 0)
		return true;
	if (config->isMappedLike("left", input))
		step(-1);
	else if (config->isMappedLike("right", input))
		step(1);
	else if (config->isMappedTo("a", input))
	{
		const auto onPick = mOnPick;
		const std::string frame = mFrames[mIndex];
		delete this;
		onPick(frame);
	}
	else if (config->isMappedTo("b", input))
		delete this;
	return true;
}

std::vector<HelpPrompt> GuiFramePicker::getHelpPrompts()
{
	return { HelpPrompt("left/right", "change"), HelpPrompt("a", "select"), HelpPrompt("b", "back") };
}
