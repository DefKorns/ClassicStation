/**
  * Copyright (c) 2026 DefKorns (https://defkorns.github.io/LICENSE)
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  */
#include "guis/GuiLaunchScreen.h"

#include "renderers/Renderer.h"
#include "resources/Font.h"
#include "utils/FileSystemUtil.h"
#include "ConsoleTools.h"
#include "FileData.h"
#include "I18n.h"
#include "Settings.h"
#include "SystemData.h"
#include "Window.h"
#include <chrono>

namespace
{
	constexpr int MissingLimitMs = 10000;
	constexpr int IgnoreInputMs = 400;
	constexpr unsigned int Background = 0x181818FF;
	constexpr unsigned int Panel = 0xC6C6C6FF;
	constexpr unsigned int Ink = 0x101018FF;
	constexpr float PanelLeft = 0.152f;
	constexpr float PanelTop = 0.312f;
	constexpr float PanelWidth = 0.690f;
	constexpr float PanelHeight = 0.368f;
	constexpr float Inset = 0.006f;
	constexpr float LineStep = 0.082f;
	constexpr float FirstLine = 0.370f;
	constexpr float SplashFirstLine = 0.745f;
	constexpr unsigned int SplashBright = 0xFFFFFFFF;
	constexpr unsigned int SplashDim = 0x8C8C8CFF;
	const char* const SplashDir = "/media/data/emulationstation/launch/splash/";

	std::string baseName(const std::string& path)
	{
		const size_t slash = path.rfind('/');
		return slash == std::string::npos ? path : path.substr(slash + 1);
	}
}

GuiLaunchScreen::GuiLaunchScreen(Window* window, FileData* game, const std::string& missingProgram, std::function<void()> launch)
	: GuiComponent(window), mImage(window), mTitle(window, "", Font::get(FONT_SIZE_SMALL), Ink, ALIGN_LEFT),
	  mDetail(window, "", Font::get(FONT_SIZE_SMALL), Ink, ALIGN_LEFT), mHint(window, "", Font::get(FONT_SIZE_SMALL), Ink, ALIGN_LEFT),
	  mLaunch(std::move(launch)), mGameName(game->getName()), mCode(game->getSourceFileData()->metadata.get("code")), mMissing(missingProgram)
{
	const float width = static_cast<float>(Renderer::getScreenWidth());
	const float height = static_cast<float>(Renderer::getScreenHeight());
	setSize(width, height);

	const std::string splash = SplashDir + game->getSourceFileData()->getSystem()->getName() + "/launching.png";
	mSplash = mMissing.empty() && Utils::FileSystem::exists(splash);
	if(mSplash)
	{
		mImage.setImage(splash);
		mImage.setOrigin(0.5f, 0.5f);
		mImage.setPosition(width / 2.0f, height / 2.0f);
		mImage.setMaxSize(width, height);
	}
	layoutText(mSplash);
	mTitle.setText(I18n::tr("LAUNCHING " + mGameName));

	const std::string command = GuiLaunchOptions::listCommand(game);
	mList = std::async(std::launch::async, [command] { return command.empty() ? std::string() : ConsoleTools::run(command); });
}

void GuiLaunchScreen::layoutText(bool onSplash)
{
	const float width = mSize.x();
	const float height = mSize.y();
	const float lineHeight = height * 0.06f;
	TextComponent* lines[] = { &mTitle, &mDetail, &mHint };
	for(int i = 0; i < 3; ++i)
	{
		if(onSplash)
		{
			lines[i]->setPosition(0.0f, height * (SplashFirstLine + i * 0.07f));
			lines[i]->setSize(width, lineHeight);
			lines[i]->setHorizontalAlignment(ALIGN_CENTER);
			lines[i]->setColor(i == 0 ? SplashBright : SplashDim);
		}
		else
		{
			lines[i]->setPosition(width * (PanelLeft + 0.024f), height * (FirstLine + i * LineStep));
			lines[i]->setSize(width * (PanelWidth - 0.048f), lineHeight);
		}
	}
}

void GuiLaunchScreen::showWaiting()
{
	mState = State::Waiting;
	mWaitMs = 0;
	if(mMissing.empty())
	{
		mLimitMs = Settings::getInstance()->getInt("LaunchScreenDelay") * 1000;
		std::string core = mChoices.defaultLabel;
		for(const auto& entry : mChoices.cores)
			if(entry.first == mChoices.current)
				core = entry.second;
		mTitle.setText(I18n::tr("LAUNCHING " + mGameName + " (" + core + ")"));
		mDetail.setText(I18n::tr("PRESS A BUTTON TO CONFIGURE"));
		mHint.setText(I18n::tr("B: CANCEL"));
		return;
	}
	mLimitMs = MissingLimitMs;
	mTitle.setText(I18n::tr("EMULATOR NOT INSTALLED"));
	mDetail.setText(I18n::tr("THIS GAME NEEDS " + baseName(mMissing) + " - INSTALL IT WITH HAKCHI"));
	mHint.setText(I18n::tr(mChoices.cores.empty() ? "PRESS A OR B TO GO BACK" : "A: USE ANOTHER EMULATOR   B: BACK"));
}

void GuiLaunchScreen::finish()
{
	const std::function<void()> launch = std::move(mLaunch);
	if(launch)
		launch();
	delete this;
}

void GuiLaunchScreen::openOptions()
{
	mState = State::Options;
	mWindow->pushGui(new GuiLaunchOptions(mWindow, mChoices, mMissing.empty(), [this](const std::string& id) {
		GuiLaunchOptions::save(mCode, mChoices, id);
		finish();
	}, [this] { delete this; }));
}

bool GuiLaunchScreen::input(InputConfig* config, Input input)
{
	if(input.value == 0 || mState != State::Waiting || mWaitMs < IgnoreInputMs)
		return true;

	const bool back = config->isMappedTo("b", input);
	if(mMissing.empty())
	{
		if(back)
			delete this;
		else
			openOptions();
	}
	else if(!back && config->isMappedTo("a", input) && !mChoices.cores.empty())
		openOptions();
	else if(back || config->isMappedTo("a", input))
		delete this;
	return true;
}

void GuiLaunchScreen::update(int deltaTime)
{
	GuiComponent::update(deltaTime);

	if(mState == State::Loading)
	{
		if(mList.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
			return;
		mChoices = GuiLaunchOptions::parse(mList.get());
		if(mMissing.empty() && mChoices.cores.empty())
		{
			finish();
			return;
		}
		showWaiting();
	}
	else if(mState == State::Waiting)
	{
		mWaitMs += deltaTime;
		if(mWaitMs < mLimitMs)
			return;
		if(mMissing.empty())
			finish();
		else
			delete this;
	}
}

void GuiLaunchScreen::render(const Transform4x4f& parentTrans)
{
	const Transform4x4f trans = parentTrans * getTransform();
	Renderer::setMatrix(trans);
	const float width = mSize.x();
	const float height = mSize.y();

	if(mSplash)
	{
		Renderer::drawRect(0.0f, 0.0f, width, height, 0x000000FF, 0x000000FF);
		mImage.render(trans);
		mTitle.render(trans);
		mDetail.render(trans);
		mHint.render(trans);
		return;
	}

	Renderer::drawRect(0.0f, 0.0f, width, height, Background, Background);

	const float x = width * PanelLeft;
	const float y = height * PanelTop;
	const float w = width * PanelWidth;
	const float h = height * PanelHeight;
	Renderer::drawRect(x, y, w, h, Panel, Panel);

	const float border = 2.0f;
	const float ix = x + width * Inset;
	const float iy = y + width * Inset;
	const float iw = w - 2.0f * width * Inset;
	const float ih = h - 2.0f * width * Inset;
	Renderer::drawRect(ix, iy, iw, border, Ink, Ink);
	Renderer::drawRect(ix, iy + ih - border, iw, border, Ink, Ink);
	Renderer::drawRect(ix, iy, border, ih, Ink, Ink);
	Renderer::drawRect(ix + iw - border, iy, border, ih, Ink, Ink);

	mTitle.render(trans);
	mDetail.render(trans);
	mHint.render(trans);
}

std::vector<HelpPrompt> GuiLaunchScreen::getHelpPrompts()
{
	return {};
}
