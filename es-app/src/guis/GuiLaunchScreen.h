/**
  * Copyright (c) 2026 DefKorns (https://defkorns.github.io/LICENSE)
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  */
#pragma once
#ifndef ES_APP_GUIS_GUI_LAUNCH_SCREEN_H
#define ES_APP_GUIS_GUI_LAUNCH_SCREEN_H

#include "components/ImageComponent.h"
#include "components/TextComponent.h"
#include "guis/GuiLaunchOptions.h"
#include "GuiComponent.h"
#include <functional>
#include <future>
#include <string>
#include <vector>

class FileData;

class GuiLaunchScreen : public GuiComponent
{
public:
	GuiLaunchScreen(Window* window, FileData* game, const std::string& missingProgram, std::function<void()> launch);

	bool input(InputConfig* config, Input input) override;
	void update(int deltaTime) override;
	void render(const Transform4x4f& parentTrans) override;
	std::vector<HelpPrompt> getHelpPrompts() override;

private:
	enum class State { Loading, Waiting, Options };

	void showWaiting();
	void openOptions();
	void finish();
	void layoutText(bool onSplash);

	ImageComponent mImage;
	TextComponent mTitle;
	TextComponent mDetail;
	TextComponent mHint;
	std::future<std::string> mList;
	std::function<void()> mLaunch;
	std::string mGameName;
	std::string mCode;
	std::string mMissing;
	LaunchChoices mChoices;
	State mState = State::Loading;
	bool mSplash = false;
	int mWaitMs = 0;
	int mLimitMs = 0;
};

#endif // ES_APP_GUIS_GUI_LAUNCH_SCREEN_H
