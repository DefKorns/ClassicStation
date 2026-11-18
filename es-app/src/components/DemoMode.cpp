/**
  * Copyright (c) 2026 DefKorns (https://defkorns.github.io/LICENSE)
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  */
#include "components/DemoMode.h"

#include "renderers/Renderer.h"
#include "resources/ResourceManager.h"
#include "Settings.h"
#include "ThemeData.h"
#include "Window.h"

#include <algorithm>
#include <map>

namespace
{
	constexpr unsigned int StartMs = 60000;
	constexpr int CooldownMs = 20000;
	constexpr int LuigiDelayMs = 700;
	constexpr int MarioFrameMs = 50;
	constexpr int LuigiFrameMs = 50;
	constexpr float WalkSpeed = 350.0f;
	constexpr float SpriteWidth = 72.0f;
	constexpr float SpriteHeight = 96.0f;
	constexpr float GroundFeetY = 630.0f;
	constexpr float DesignHeight = 720.0f;

	std::string spriteFolder()
	{
		const std::map<std::string, ThemeSet> sets = ThemeData::getThemeSets();
		const auto set = sets.find(Settings::getInstance()->getString("ThemeSet"));
		return set == sets.end() ? std::string() : set->second.path + "/demo/";
	}

	float screenScale()
	{
		return static_cast<float>(Renderer::getScreenHeight()) / DesignHeight;
	}
}

DemoMode::Runner::Runner(Window* window, const std::string& folder, const std::string& prefix, int frameMs_, int delayMs_)
	: sprite(window), frameMs(frameMs_), delayMs(delayMs_)
{
	mAvailable = !folder.empty();
	for(std::size_t index = 0; index < frames.size(); ++index)
	{
		frames[index] = folder + prefix + "0" + std::to_string(index + 1) + ".png";
		mAvailable = mAvailable && ResourceManager::getInstance()->fileExists(frames[index]);
	}
	sprite.setOrigin(0.5f, 1.0f);
}

void DemoMode::Runner::begin(float startX)
{
	x = startX;
	waitedMs = 0;
	elapsedMs = 0;
	frame = -1;
	running = false;
	finished = false;
}

void DemoMode::Runner::advance(int deltaTime, float direction, float step, float limitLow, float limitHigh)
{
	if(finished)
		return;
	if(!running)
	{
		waitedMs += deltaTime;
		if(waitedMs < delayMs)
			return;
		running = true;
	}

	x += direction * step;
	elapsedMs += deltaTime;
	const int next = (elapsedMs / frameMs) % static_cast<int>(frames.size());
	if(next != frame)
	{
		frame = next;
		sprite.setImage(frames[static_cast<std::size_t>(frame)]);
	}
	if((direction > 0.0f && x > limitHigh) || (direction < 0.0f && x < limitLow))
		finished = true;
}

void DemoMode::Runner::draw(const Transform4x4f& parentTrans, float direction, float scale, float feetY)
{
	if(!running || finished || frame < 0)
		return;
	sprite.setResize(SpriteWidth * scale, SpriteHeight * scale);
	sprite.setFlipX(direction > 0.0f);
	sprite.setPosition(x, feetY);
	sprite.render(parentTrans);
}

DemoMode::DemoMode(Window* window)
	: GuiComponent(window), mMario(window, spriteFolder(), "mario_run", MarioFrameMs, 0), mLuigi(window, spriteFolder(), "luigi_run", LuigiFrameMs, LuigiDelayMs)
{
}

void DemoMode::cancel()
{
	mActive = false;
	mCooldownMs = 0;
}

void DemoMode::startPass()
{
	const float width = static_cast<float>(Renderer::getScreenWidth());
	const float margin = SpriteWidth * screenScale();
	const float startX = mPass % 2 == 0 ? -margin : width + margin;
	mMario.begin(startX);
	mLuigi.begin(startX);
}

void DemoMode::update(int deltaTime, bool ready, float coverTop)
{
	if(!mMario.available() || !mLuigi.available() || !Settings::getInstance()->getBool("DemoMode"))
	{
		cancel();
		return;
	}

	const unsigned int idle = mWindow->getTimeSinceLastInput();
	const bool interrupted = idle < mLastIdle;
	mLastIdle = idle;
	if(mActive && (interrupted || !ready))
		cancel();
	if(!mActive)
	{
		if(idle < StartMs || !ready)
			return;
		mCooldownMs -= deltaTime;
		if(mCooldownMs > 0)
			return;
		mActive = true;
		mPass = 0;
		startPass();
	}

	mCoverTop = coverTop;
	const float scale = screenScale();
	const float width = static_cast<float>(Renderer::getScreenWidth());
	const float margin = SpriteWidth * scale;
	const float direction = mPass % 2 == 0 ? 1.0f : -1.0f;
	const float step = WalkSpeed * scale * static_cast<float>(deltaTime) / 1000.0f;
	mMario.advance(deltaTime, direction, step, -margin, width + margin);
	mLuigi.advance(deltaTime, direction, step, -margin, width + margin);

	if(mMario.finished && mLuigi.finished)
	{
		++mPass;
		if(mPass >= 2)
		{
			mActive = false;
			mCooldownMs = CooldownMs;
			return;
		}
		startPass();
	}
}

void DemoMode::render(const Transform4x4f& parentTrans)
{
	if(!mActive)
		return;

	const float scale = screenScale();
	const float direction = mPass % 2 == 0 ? 1.0f : -1.0f;
	const float feetY = mPass % 2 == 0 ? GroundFeetY * scale : mCoverTop;
	mMario.draw(parentTrans, direction, scale, feetY);
	mLuigi.draw(parentTrans, direction, scale, feetY);
}
