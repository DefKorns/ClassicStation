/**
  * Copyright (c) 2026 DefKorns (https://defkorns.github.io/LICENSE)
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  */
#pragma once
#ifndef ES_APP_COMPONENTS_DEMO_MODE_H
#define ES_APP_COMPONENTS_DEMO_MODE_H

#include "GuiComponent.h"
#include "components/ImageComponent.h"
#include <array>
#include <string>

class DemoMode : public GuiComponent
{
public:
	explicit DemoMode(Window* window);

	bool active() const { return mActive; }
	void cancel();
	void update(int deltaTime, bool ready, float coverTop);
	void render(const Transform4x4f& parentTrans) override;

private:
	struct Runner
	{
		Runner(Window* window, const std::string& prefix, int frameMs, int delayMs);

		void locate(const std::string& folder);
		bool available() const { return mAvailable; }
		void begin(float startX);
		void advance(int deltaTime, float direction, float step, float limitLow, float limitHigh);
		void draw(const Transform4x4f& parentTrans, float direction, float scale, float feetY);

		std::string prefix;
		std::array<std::string, 3> frames;
		ImageComponent sprite;
		int frameMs;
		int delayMs;
		int waitedMs = 0;
		int elapsedMs = 0;
		int frame = -1;
		float x = 0.0f;
		bool running = false;
		bool finished = false;
		bool mAvailable = false;
	};

	void startPass();

	Runner mMario;
	Runner mLuigi;
	bool mActive = false;
	int mPass = 0;
	int mCooldownMs = 0;
	unsigned int mLastIdle = 0;
	float mCoverTop = 0.0f;
};

#endif // ES_APP_COMPONENTS_DEMO_MODE_H
